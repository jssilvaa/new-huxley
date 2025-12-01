#!/usr/bin/env python3
"""Qt-friendly chat session thread."""
from __future__ import annotations

import select
import threading
import time
from typing import Any, Optional

from PyQt6.QtCore import QObject, QThread, pyqtSignal

try:
    from .protocol import ProtocolClient, MessageClient, JsonDict, _extract_command
    from .store import ConversationStore
except ImportError:
    from protocol import ProtocolClient, MessageClient, JsonDict, _extract_command  # type: ignore
    from store import ConversationStore  # type: ignore


class ChatSession(QThread):
    users_updated = pyqtSignal(list)
    conversation_updated = pyqtSignal(str, list)
    status_changed = pyqtSignal(str)
    error_occurred = pyqtSignal(str)
    disconnected = pyqtSignal()
    unread_updated = pyqtSignal(dict)
    login_result = pyqtSignal(bool, str)
    register_result = pyqtSignal(bool, str)

    def __init__(self, host: str, port: int, timeout: float, history_limit: int = 50) -> None:
        super().__init__()
        self.host = host
        self.port = port
        self.history_limit = max(1, history_limit)
        self.refresh_users_interval = 2.5
        self.refresh_history_interval = 1.5
        self.store = ConversationStore(max(self.history_limit * 4, 200))
        self.protocol = ProtocolClient(timeout)
        self.message_client = MessageClient(self.protocol, self._handle_async)
        self.running = True
        self.selected_peer: Optional[str] = None
        self.unread: dict[str, int] = {}
        self.last_users_refresh = 0.0
        self.last_history_refresh = 0.0
        self._cmd_queue: list[tuple[callable, tuple, dict]] = []
        self._cmd_lock = threading.Lock()

    def connect_socket(self) -> bool:
        try:
            self.protocol.connect(self.host, self.port)
            return True
        except Exception as exc:
            self.error_occurred.emit(f"Connect failed: {exc}")
            return False

    def stop(self) -> None:
        self.running = False
        self.quit()
        self.wait(2000)

    def run(self) -> None:
        while self.running:
            self._drain_commands()
            self._poll_socket()
            self._periodic_refresh()
            self.msleep(20)

    def _drain_commands(self) -> None:
        items: list[tuple[callable, tuple, dict]] = []
        with self._cmd_lock:
            if self._cmd_queue:
                items = list(self._cmd_queue)
                self._cmd_queue.clear()
        for fn, args, kwargs in items:
            try:
                fn(*args, **kwargs)
            except Exception as exc:
                self.error_occurred.emit(f"Command failed: {exc}")

    # Public commands -------------------------------------------------
    def register(self, username: str, password: str) -> None:
        resp = self.message_client.register(username, password)
        ok = bool(resp and resp.get("success"))
        msg = resp.get("message", "Registration failed") if resp else "No response"
        self.register_result.emit(ok, msg)

    def login(self, username: str, password: str) -> None:
        resp = self.message_client.login(username, password)
        ok = bool(resp and resp.get("success"))
        msg = resp.get("message", "Login failed") if resp else "No response"
        self.login_result.emit(ok, msg)
        if ok:
            self.status_changed.emit("Logged in")

    def logout(self) -> None:
        resp = self.message_client.logout()
        ok = bool(resp and resp.get("success"))
        if ok:
            self.status_changed.emit("Logged out")

    def select_peer(self, peer: str) -> None:
        self.selected_peer = peer
        self.unread[peer] = 0
        self.unread_updated.emit(dict(self.unread))
        self._enqueue(self._request_history, peer)

    def send_message(self, peer: str, content: str) -> None:
        if not content.strip():
            return
        ts = time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime())
        self._enqueue(self._send_message_internal, peer, content, ts)

    def _send_message_internal(self, peer: str, content: str, ts: str) -> None:
        resp = self.message_client.send_message(peer, content, ts)
        if resp and resp.get("success"):
            self.store.append(peer, {"from": self.message_client.username or "", "to": peer, "content": content, "timestamp": ts})
            self.conversation_updated.emit(peer, self.store.get(peer))
            if peer == self.selected_peer:
                self._enqueue(self._request_history, peer)
        else:
            msg = resp.get("message", "Send failed") if resp else "No response"
            self.error_occurred.emit(msg)

    def _enqueue(self, fn: callable, *args, **kwargs) -> None:
        with self._cmd_lock:
            self._cmd_queue.append((fn, args, kwargs))

    # Internals -------------------------------------------------------
    def _poll_socket(self) -> None:
        sock = self.protocol.raw_socket
        if not sock:
            return
        while True:
            ready, _, _ = select.select([sock], [], [], 0)
            if not ready:
                break
            response = self.protocol.recv_response()
            if response is None:
                self.disconnected.emit()
                self.running = False
                break
            self._dispatch_response(response)

    def _periodic_refresh(self) -> None:
        now = time.monotonic()
        if now - self.last_users_refresh >= self.refresh_users_interval:
            self.last_users_refresh = now
            self._refresh_users()
        if self.selected_peer and now - self.last_history_refresh >= self.refresh_history_interval:
            self.last_history_refresh = now
            self._request_history(self.selected_peer)

    def _dispatch_response(self, response: JsonDict) -> None:
        command = _extract_command(response) or ""
        base_command = command[:-9] if command.endswith("_response") else command
        if command in {"incoming_message", "incoming_message_response"}:
            self._handle_incoming(response)
            return
        # Command responses that do not go through send_and_wait
        if base_command in {"list_users", "get_history"}:
            # handled via pending handlers in TUI; here we call directly
            if base_command == "list_users":
                self._handle_users(response)
            elif base_command == "get_history":
                self._handle_history(response)
            return
        if not response.get("success", True):
            self.error_occurred.emit(response.get("message", "Command failed"))

    def _handle_async(self, response: JsonDict) -> None:
        cmd = _extract_command(response)
        if cmd in {"incoming_message", "incoming_message_response"}:
            self._handle_incoming(response)
        elif cmd == "timeout":
            self.error_occurred.emit(response.get("message", "Timeout"))

    def _handle_incoming(self, response: JsonDict) -> None:
        sender = response.get("sender", "?")
        content = response.get("content", "")
        ts = response.get("timestamp") or time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime())
        msg_id = response.get("id")
        peer = sender
        self.store.append(peer, {"id": msg_id, "from": sender, "to": self.message_client.username or "", "content": content, "timestamp": ts})
        if peer != self.selected_peer:
            self.unread[peer] = self.unread.get(peer, 0) + 1
            self.unread_updated.emit(dict(self.unread))
        self.conversation_updated.emit(peer, self.store.get(peer))
        if peer == self.selected_peer:
            self._enqueue(self._request_history, peer)

    def _refresh_users(self) -> None:
        resp = self.message_client.list_users()
        if not resp or not resp.get("success"):
            return
        payload = resp.get("payload") or {}
        users = payload.get("users", [])
        self.users_updated.emit(users)

    def _request_history(self, peer: str) -> None:
        resp = self.message_client.get_history(peer, self.history_limit, 0)
        self.last_history_refresh = time.monotonic()
        self._handle_history(resp)

    def _handle_history(self, resp: Optional[JsonDict]) -> None:
        if not resp or not resp.get("success"):
            return
        payload = resp.get("payload") or {}
        peer = payload.get("with") or self.selected_peer
        messages = payload.get("messages", [])
        if peer:
            self.store.merge_history(peer, messages)
            self.conversation_updated.emit(peer, self.store.get(peer))
