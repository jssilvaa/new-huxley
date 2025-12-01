#!/usr/bin/env python3
"""Curses-based chat UI."""
from __future__ import annotations

import curses
import datetime
import errno
import select
import textwrap
import time
from collections import defaultdict
from typing import Any, Callable, Dict, Optional

from .protocol import ProtocolClient, MessageClient, JsonDict, _extract_command


class ConversationStore:
    """In-memory conversation cache with normalization and deduping."""

    def __init__(self, max_buffer: int) -> None:
        self.max_buffer = max_buffer
        self._conversations: dict[str, list[JsonDict]] = {}

    @staticmethod
    def _normalize_timestamp(ts: Any) -> str:
        if not isinstance(ts, str):
            return ""
        cleaned = ts.strip()
        if cleaned.endswith("Z"):
            cleaned = cleaned[:-1]
        cleaned = cleaned.replace("T", " ")
        if "." in cleaned:
            cleaned = cleaned.split(".", 1)[0]
        return cleaned

    def _base_key(self, message: JsonDict) -> tuple[Any, str, str, str, str]:
        msg_id = message.get("id")
        content = message.get("content", "")
        content_key = content.strip() if isinstance(content, str) else str(content)
        from_user = message.get("from", "") or ""
        to_user = message.get("to", "") or ""
        a, b = sorted([from_user, to_user])
        ts = self._normalize_timestamp(message.get("timestamp"))
        return (msg_id, a, b, content_key, ts)

    @staticmethod
    def _ts_score(ts: str) -> float:
        if not ts:
            return float("inf")
        try:
            return time.mktime(time.strptime(ts, "%Y-%m-%d %H:%M:%S"))
        except Exception:
            return float("inf")

    def get(self, peer: Optional[str]) -> list[JsonDict]:
        if not peer:
            return []
        return self._conversations.get(peer, [])

    def append(self, peer: str, message: JsonDict) -> bool:
        normalized = dict(message)
        normalized["timestamp"] = self._normalize_timestamp(normalized.get("timestamp"))
        convo = self._conversations.setdefault(peer, [])
        convo.append(normalized)
        changed = self._dedupe_and_clip(peer)
        return changed

    def merge_history(self, peer: str, messages: list[JsonDict]) -> None:
        normalized = []
        for msg in messages:
            normalized.append({
                "id": msg.get("id"),
                "from": msg.get("from", "?"),
                "to": msg.get("to", "?"),
                "content": msg.get("content", ""),
                "timestamp": self._normalize_timestamp(msg.get("timestamp")),
            })

        existing = list(self._conversations.get(peer, []))
        existing.extend(normalized)
        self._conversations[peer] = existing
        self._dedupe_and_clip(peer)

    def _dedupe_and_clip(self, peer: str) -> bool:
        convo = self._conversations.get(peer, [])
        if not convo:
            return False

        def better(a: JsonDict, b: JsonDict) -> JsonDict:
            ts_a = a.get("timestamp") or ""
            ts_b = b.get("timestamp") or ""
            score_a = self._ts_score(ts_a)
            score_b = self._ts_score(ts_b)
            if score_a != score_b:
                return a if score_a > score_b else b
            if len(ts_a) != len(ts_b):
                return a if len(ts_a) > len(ts_b) else b
            return a

        by_id: dict[Any, JsonDict] = {}
        fallback: dict[tuple[str, str, str], JsonDict] = {}
        for raw in convo:
            msg = dict(raw)
            msg["timestamp"] = self._normalize_timestamp(msg.get("timestamp"))
            msg_id = msg.get("id")
            base = self._base_key(msg)
            participants_key = (base[1], base[2], base[3])
            if msg_id is not None:
                if msg_id in by_id:
                    by_id[msg_id] = better(by_id[msg_id], msg)
                else:
                    by_id[msg_id] = msg
            else:
                if participants_key in fallback:
                    fallback[participants_key] = better(fallback[participants_key], msg)
                else:
                    fallback[participants_key] = msg

        for msg in by_id.values():
            base = self._base_key(msg)
            participants_key = (base[1], base[2], base[3])
            fallback.pop(participants_key, None)

        merged_msgs = list(by_id.values()) + list(fallback.values())
        merged_msgs = sorted(
            merged_msgs,
            key=lambda m: (
                self._normalize_timestamp(m.get("timestamp")),
                m.get("from") or "",
                m.get("to") or "",
                m.get("content") or "",
            ),
        )
        if len(merged_msgs) > self.max_buffer:
            merged_msgs = merged_msgs[-self.max_buffer:]
        changed = merged_msgs != convo
        self._conversations[peer] = merged_msgs
        return changed


class CursesChatApp:
    def __init__(self, host: str, port: int, timeout: float, history_limit: int = 50) -> None:
        self.protocol = ProtocolClient(timeout)
        self.host = host
        self.port = port
        self.history_limit = max(1, history_limit)
        self.max_buffer = max(self.history_limit * 4, 200)
        self.store = ConversationStore(self.max_buffer)
        self.username: Optional[str] = None
        self.users: list[dict[str, Any]] = []
        self.selected_index = 0
        self.selected_peer: Optional[str] = None
        self.pending_handlers: defaultdict[str, list[Callable[[JsonDict], None]]] = defaultdict(list)
        self.status = ""
        self.error_message: Optional[str] = None
        self.error_modal = False
        self.input_buffer = ""
        self.running = True
        self.focus_chat = False
        self.awaiting_quit_confirm = False
        self.last_users_refresh = 0.0
        self.last_history_refresh = 0.0
        self.refresh_users_interval = 5.0
        self.refresh_history_interval = 4.0
        self.stdscr: Optional[Any] = None
        self.unread_counts: dict[str, int] = {}
        self.chat_offset_by_peer: dict[str, int] = {}
        self._last_chat_visible: int = 0

    def run(self) -> int:
        try:
            self.protocol.connect(self.host, self.port)
        except OSError as exc:
            print(f"[FATAL] Unable to connect to {self.host}:{self.port} — {exc}")
            return 1
        try:
            return curses.wrapper(self._run)
        finally:
            self.protocol.close()

    def _run(self, stdscr: Any) -> int:
        self.stdscr = stdscr
        self._setup_curses(stdscr)
        if not self._login_flow():
            return 0
        self._refresh_users()
        self.status = "Arrows: select user | Enter: chat | PgUp/PgDn scroll | q: quit"
        while self.running:
            self._poll_socket()
            self._periodic_refresh()
            self._render(stdscr)
            self._handle_key()
            time.sleep(0.05)
        return 0

    def _setup_curses(self, stdscr: Any) -> None:
        try:
            curses.curs_set(1)
        except curses.error:
            pass
        stdscr.nodelay(True)
        stdscr.keypad(True)
        try:
            curses.start_color()
            curses.use_default_colors()
            curses.init_pair(1, curses.COLOR_GREEN, -1)
            curses.init_pair(2, curses.COLOR_CYAN, -1)
            curses.init_pair(3, curses.COLOR_YELLOW, -1)
            curses.init_pair(4, curses.COLOR_RED, -1)
        except curses.error:
            pass

    def _login_flow(self) -> bool:
        while self.running and not self.username:
            self._draw_login_screen()
            ch = self.stdscr.getch()
            if self.error_message and ch in (curses.KEY_ENTER, 10, 13, 27):
                self._clear_error()
                continue
            if ch in (ord("q"), ord("Q")):
                self.running = False
                break
            if ch in (ord("l"), ord("L")):
                creds = self._prompt_credentials("Login")
                if creds:
                    self._handle_login(creds)
            elif ch in (ord("r"), ord("R")):
                creds = self._prompt_credentials("Register")
                if creds:
                    if self._handle_register(creds):
                        self._handle_login(creds)
            time.sleep(0.05)
        return self.username is not None

    def _handle_login(self, creds: tuple[str, str]) -> bool:
        username, password = creds
        resp = self._send_and_wait(
            {"type": "LOGIN", "username": username, "password": password},
            expected_command="login",
        )
        if resp and resp.get("success"):
            self.username = username
            self._clear_error()
            self.status = "Logged in"
            return True
        self._show_error(resp.get("message", "Login failed") if resp else "No login response", modal=False)
        return False

    def _handle_register(self, creds: tuple[str, str]) -> bool:
        username, password = creds
        resp = self._send_and_wait(
            {"type": "REGISTER", "username": username, "password": password},
            expected_command="register",
        )
        if resp and resp.get("success"):
            self._clear_error()
            self.status = "Registered. Logging in..."
            return True
        self._show_error(resp.get("message", "Registration failed") if resp else "No register response", modal=False)
        return False

    def _prompt_credentials(self, title: str) -> Optional[tuple[str, str]]:
        username = self._prompt_line(f"{title} username: ")
        if username is None:
            return None
        password = self._prompt_line(f"{title} password: ", hidden=True)
        if password is None:
            return None
        return username, password

    def _prompt_line(self, prompt: str, hidden: bool = False) -> Optional[str]:
        buffer: list[str] = []
        curses.curs_set(1)
        self.stdscr.nodelay(False)
        while True:
            self.stdscr.erase()
            self.stdscr.addstr(0, 0, prompt)
            display = "*" * len(buffer) if hidden else "".join(buffer)
            self.stdscr.addstr(1, 0, display)
            self.stdscr.refresh()
            ch = self.stdscr.getch()
            if ch in (curses.KEY_ENTER, 10, 13):
                self.stdscr.nodelay(True)
                return "".join(buffer)
            if ch in (27,):
                self.stdscr.nodelay(True)
                return None
            if ch in (curses.KEY_BACKSPACE, 127, 8):
                if buffer:
                    buffer.pop()
                continue
            if 32 <= ch <= 126 and len(buffer) < 80:
                buffer.append(chr(ch))

    def _send_and_wait(self, payload: JsonDict, expected_command: str, timeout: float = 5.0) -> Optional[JsonDict]:
        sock = self.protocol.raw_socket
        if sock is None:
            return None
        self.protocol.send_command(payload)
        deadline = time.monotonic() + timeout
        while time.monotonic() < deadline and self.running:
            remaining = max(0.0, deadline - time.monotonic())
            ready, _, _ = select.select([sock], [], [], remaining)
            if not ready:
                continue
            response = self.protocol.recv_response()
            if response is None:
                self.running = False
                return None
            cmd = _extract_command(response)
            if cmd == expected_command or cmd == f"{expected_command}_response":
                return response
            self._dispatch_response(response)
        return None

    def _send_request(self, payload: JsonDict, expected_command: str, handler: Optional[Callable[[JsonDict], None]] = None) -> bool:
        if handler:
            if self.pending_handlers[expected_command]:
                return False
            self.pending_handlers[expected_command].append(handler)
        self.protocol.send_command(payload)
        return True

    def _periodic_refresh(self) -> None:
        now = time.monotonic()
        if now - self.last_users_refresh >= self.refresh_users_interval:
            if self._refresh_users():
                self.last_users_refresh = now
        if self.selected_peer and now - self.last_history_refresh >= self.refresh_history_interval:
            if self._request_history(self.selected_peer):
                self.last_history_refresh = now

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
                self.status = "Server closed connection."
                self.running = False
                break
            self._dispatch_response(response)

    def _dispatch_response(self, response: JsonDict) -> None:
        command = _extract_command(response) or ""
        base_command = command[:-9] if command.endswith("_response") else command
        if command in {"incoming_message", "incoming_message_response"}:
            self._handle_incoming(response)
            return
        if base_command and self.pending_handlers.get(base_command):
            handler = self.pending_handlers[base_command].pop(0)
            try:
                handler(response)
            except Exception as exc:
                self.status = f"Handler error: {exc}"
            if not response.get("success", True):
                self._show_server_warning(command or base_command, response)
            return
        if command == "timeout":
            self.status = f"Session timeout: {response.get('message', '')}"
            return
        if not response.get("success", True):
            self._show_server_warning(command, response)

    def _handle_incoming(self, response: JsonDict) -> None:
        sender = response.get("sender", "?")
        content = response.get("content", "")
        timestamp = response.get("timestamp") or datetime.datetime.utcnow().isoformat() + "Z"
        msg_id = response.get("id")
        peer = sender
        self._append_message(peer, sender, self.username or "", content, timestamp, msg_id)
        if not any(user.get("username") == peer for user in self.users):
            self.users.append({"username": peer, "online": True})
        if peer != self.selected_peer or not self.focus_chat:
            self.unread_counts[peer] = self.unread_counts.get(peer, 0) + 1
        self.status = f"New message from {sender}"
        self._clear_error()

    def _refresh_users(self) -> bool:
        def handler(resp: JsonDict) -> None:
            if not resp or not resp.get("success"):
                msg = resp.get("message", "failed") if resp else "no response"
                self._show_error(f"User list failed: {msg}")
                return
            payload = resp.get("payload") or {}
            all_users = payload.get("users", [])
            self.users = [u for u in all_users if u.get("username") != self.username]
            names = [user.get("username", "") for user in self.users]
            self._clear_error()
            if not self.users:
                self.selected_peer = None
                self.selected_index = 0
                return
            if self.selected_peer not in names:
                self.selected_index = 0
                self.selected_peer = names[0]
                self._request_history(self.selected_peer)
            else:
                self.selected_index = names.index(self.selected_peer)
        return self._send_request({"type": "LIST_USERS"}, "list_users", handler)

    def _request_history(self, peer: str) -> bool:
        def handler(resp: JsonDict) -> None:
            if not resp or not resp.get("success"):
                msg = resp.get("message", "failed") if resp else "no response"
                self._show_error(f"History failed: {msg}")
                return
            payload = resp.get("payload") or {}
            messages = payload.get("messages", [])
            self.store.merge_history(peer, messages)
            self._mark_read(peer)
            self._clear_error()
        return self._send_request(
            {"type": "GET_HISTORY", "with": peer, "limit": self.history_limit, "offset": 0},
            "get_history",
            handler,
        )

    def _append_message(
        self,
        peer: str,
        sender: str,
        recipient: str,
        content: str,
        timestamp: str,
        msg_id: Any | None = None,
    ) -> None:
        self.store.append(peer, {
            "id": msg_id,
            "from": sender,
            "to": recipient,
            "content": content,
            "timestamp": timestamp,
        })

    def _handle_key(self) -> None:
        ch = self.stdscr.getch()
        if ch == -1:
            return
        if self.error_modal:
            if ch in (curses.KEY_ENTER, 10, 13, 27):
                self._clear_error()
            return
        if self.awaiting_quit_confirm:
            if ch in (ord("y"), ord("Y")):
                self._logout_and_quit()
            elif ch in (ord("n"), ord("N"), 27):
                self.awaiting_quit_confirm = False
                self.status = "Quit canceled."
            return

        if self.focus_chat:
            if ch in (27,):
                self.focus_chat = False
                self.status = "Back to user list."
                return
            if ch == curses.KEY_PPAGE:
                self._scroll_chat(self._last_chat_visible or 5)
                return
            if ch == curses.KEY_NPAGE:
                self._scroll_chat(-(self._last_chat_visible or 5))
                return
            if ch == curses.KEY_UP:
                self._scroll_chat(1)
                return
            if ch == curses.KEY_DOWN:
                self._scroll_chat(-1)
                return
            if ch in (curses.KEY_BACKSPACE, 127, 8):
                self.input_buffer = self.input_buffer[:-1]
                return
            if ch in (curses.KEY_ENTER, 10, 13):
                self._send_current_message()
                return
            if 32 <= ch <= 126:
                self.input_buffer += chr(ch)
            return

        if ch in (ord("q"), ord("Q")):
            self.awaiting_quit_confirm = True
            self.status = ""
            return
        if ch == curses.KEY_UP:
            self._move_selection(-1)
            return
        if ch == curses.KEY_DOWN:
            self._move_selection(1)
            return
        if ch in (ord("u"), ord("U")):
            if self._refresh_users():
                self.last_users_refresh = time.monotonic()
            return
        if ch in (ord("r"), ord("R")) and self.selected_peer:
            if self._request_history(self.selected_peer):
                self.last_history_refresh = time.monotonic()
            return
        if ch in (curses.KEY_ENTER, 10, 13) and self.selected_peer:
            self.focus_chat = True
            self.status = f"Chatting with {self.selected_peer}. ESC to go back."
            return

    def _move_selection(self, delta: int) -> None:
        if not self.users:
            return
        self.selected_index = (self.selected_index + delta) % len(self.users)
        self.selected_peer = self.users[self.selected_index].get("username")
        if self.selected_peer:
            self._request_history(self.selected_peer)
            self._mark_read(self.selected_peer)

    def _send_current_message(self) -> None:
        if not self.selected_peer:
            self.status = "Select a user before sending."
            return
        content = self.input_buffer.strip()
        if not content:
            return
        self.input_buffer = ""
        timestamp = datetime.datetime.utcnow().isoformat() + "Z"

        def handler(resp: JsonDict) -> None:
            if resp and resp.get("success"):
                self._append_message(self.selected_peer or "", self.username or "", self.selected_peer or "", content, timestamp)
                self.chat_offset_by_peer[self.selected_peer or ""] = 0
                self.status = "Message sent."
                self._clear_error()
            else:
                msg = resp.get("message", "Send failed") if resp else "No response"
                self._show_error(msg)

        self._send_request(
            {"type": "SEND_MESSAGE", "recipient": self.selected_peer, "content": content, "timestamp": timestamp},
            "send_message",
            handler,
        )

    def _render(self, stdscr: Any) -> None:
        stdscr.erase()
        height, width = stdscr.getmaxyx()
        left_width = max(20, int(width * 0.28))
        chat_height = height - 3

        self._draw_header(stdscr, width)

        user_win = stdscr.derwin(chat_height, left_width, 1, 0)
        chat_win = stdscr.derwin(chat_height, width - left_width, 1, left_width)
        self._draw_users(user_win)
        self._draw_chat(chat_win)
        self._draw_input(stdscr, height - 2, width)
        self._draw_status(stdscr, height - 1, width)
        if self.error_modal:
            self._draw_error_modal(stdscr)
        if self.awaiting_quit_confirm:
            self._draw_quit_modal(stdscr)
        stdscr.refresh()

    def _draw_header(self, stdscr: Any, width: int) -> None:
        username = self.username or "(not logged in)"
        header = f"{username} @ {self.host}:{self.port}"
        hint = "Enter: chat | ESC: list | PgUp/PgDn/up/down: scroll | q: quit | u/r: refresh"
        text = f"{header} — {hint}"
        self._safe_addstr(stdscr, 0, 0, text[: max(0, width - 1)], curses.A_BOLD)

    def _draw_users(self, win: Any) -> None:
        win.erase()
        win.border()
        if not self.users:
            self._safe_addstr(win, 1, 1, "No users yet.")
            return
        for idx, user in enumerate(self.users, start=0):
            name = user.get("username", "?")
            online = user.get("online") is True
            unread = self.unread_counts.get(name, 0)
            marker = "*" if online else " "
            badge = f" [{unread}]" if unread > 0 else ""
            line = f"{marker} {name}{badge}"
            attr = curses.color_pair(1) if online else 0
            if idx == self.selected_index:
                attr |= curses.A_REVERSE | curses.color_pair(2)
            self._safe_addstr(win, 1 + idx, 1, line, attr)

    def _draw_chat(self, win: Any) -> None:
        win.erase()
        win.border()
        if not self.selected_peer:
            self._safe_addstr(win, 1, 1, "Select a user to start chatting.")
            return
        messages = self.store.get(self.selected_peer)
        height, width = win.getmaxyx()
        visible_lines = height - 2
        self._last_chat_visible = visible_lines
        offset = self.chat_offset_by_peer.get(self.selected_peer, 0)
        max_offset = max(0, len(messages) - visible_lines)
        offset = min(max_offset, max(0, offset))
        self.chat_offset_by_peer[self.selected_peer] = offset
        start = max(0, len(messages) - visible_lines - offset)
        for idx, msg in enumerate(messages[start:], start=0):
            timestamp = msg.get("timestamp") or ""
            sender = msg.get("from", "?")
            content = msg.get("content", "")
            prefix = f"[{timestamp}] " if timestamp else ""
            line = f"{prefix}{sender}: {content}"
            self._safe_addstr(win, 1 + idx, 1, line[: max(0, width - 2)])

    def _draw_input(self, stdscr: Any, row: int, width: int) -> None:
        target = self.selected_peer or "(no user selected)"
        prompt = f"To {target}: {self.input_buffer}"
        padded = prompt[: max(0, width - 2)]
        stdscr.addstr(row, 0, " " * (width - 1))
        self._safe_addstr(stdscr, row, 1, padded)
        try:
            curses.curs_set(1)
        except curses.error:
            pass
        cursor_x = min(len(prompt), width - 2)
        stdscr.move(row, cursor_x)

    def _draw_status(self, stdscr: Any, row: int, width: int) -> None:
        message = self.error_message or self.status
        msg = message[: max(0, width - 2)]
        attr = curses.color_pair(4) if self.error_message else curses.color_pair(3)
        stdscr.addstr(row, 0, " " * (width - 1))
        self._safe_addstr(stdscr, row, 1, msg, attr)

    def _draw_error_modal(self, stdscr: Any) -> None:
        if not self.error_message:
            return
        height, width = stdscr.getmaxyx()
        modal_width = min(width - 4, 70)
        if modal_width < 20 or height < 6:
            return
        wrapped = textwrap.wrap(self.error_message, modal_width - 4) or [self.error_message]
        modal_height = min(len(wrapped) + 4, height - 2)
        top = (height - modal_height) // 2
        left = (width - modal_width) // 2
        try:
            win = stdscr.derwin(modal_height, modal_width, top, left)
            win.bkgd(" ", curses.color_pair(4) | curses.A_REVERSE)
            win.box()
            title = " ERROR "
            win.addstr(0, max(1, (modal_width - len(title)) // 2), title, curses.A_BOLD | curses.color_pair(4))
            for idx, line in enumerate(wrapped[: modal_height - 2]):
                win.addstr(1 + idx, 2, line)
            hint = "[Enter/ESC to dismiss]"
            win.addstr(modal_height - 1, max(1, modal_width - len(hint) - 2), hint, curses.A_DIM)
        except curses.error:
            pass

    def _draw_login_screen(self) -> None:
        stdscr = self.stdscr
        if stdscr is None:
            return
        stdscr.erase()
        height, width = stdscr.getmaxyx()
        art = [
            " _   _           _            ",
            "| | | |___  _ __| |_____  ___ ",
            "| |_| / _ \\| '__| / / _ \\/ __|",
            "|  _  | (_) | |  | |  __/\\__ \\",
            "|_| |_|\\___/|_|  |_|\\___||___/",
        ]
        art_width = max(len(line) for line in art)
        art_left = max(0, (width - art_width) // 2)
        for i, line in enumerate(art):
            self._safe_addstr(stdscr, 1 + i, art_left, line, curses.A_BOLD | curses.color_pair(2))

        prompt = "Press [l]ogin  [r]egister  [q]uit"
        self._safe_addstr(stdscr, len(art) + 3, max(0, (width - len(prompt)) // 2), prompt, curses.A_BOLD)

        if self.error_message:
            self._draw_login_error_box(stdscr, self.error_message)

        stdscr.refresh()

    def _draw_login_error_box(self, stdscr: Any, message: str) -> None:
        height, width = stdscr.getmaxyx()
        box_width = min(width - 4, 70)
        wrapped = textwrap.wrap(message, box_width - 4) or [message]
        box_height = min(len(wrapped) + 2, max(3, height // 3))
        start_row = min(height - box_height - 1, 8)
        start_col = (width - box_width) // 2
        try:
            win = stdscr.derwin(box_height, box_width, start_row, start_col)
            win.bkgd(" ", curses.color_pair(4) | curses.A_REVERSE)
            win.box()
            for idx, line in enumerate(wrapped[: box_height - 2]):
                win.addstr(1 + idx, 2, line)
        except curses.error:
            pass

    @staticmethod
    def _safe_addstr(win: Any, y: int, x: int, text: str, attr: int = 0) -> None:
        try:
            win.addstr(y, x, text, attr)
        except curses.error:
            pass

    def _show_error(self, message: str, modal: bool = True) -> None:
        self.error_message = message
        self.status = ""
        self.error_modal = modal

    def _clear_error(self) -> None:
        self.error_message = None
        self.error_modal = False
        if not self.username:
            self.status = ""

    def _logout_and_quit(self) -> None:
        self.awaiting_quit_confirm = False
        resp = self._send_and_wait({"type": "LOGOUT"}, "logout", timeout=2.0)
        if resp and resp.get("success"):
            self.status = "Logged out."
        self.running = False

    def _show_server_warning(self, command: str, response: JsonDict) -> None:
        msg = response.get("message") or "Command failed"
        label = command.upper() if command else "SERVER"
        self._show_error(f"{label}: {msg}")

    def _mark_read(self, peer: str) -> None:
        if peer in self.unread_counts:
            self.unread_counts[peer] = 0
        self.chat_offset_by_peer[peer] = self.chat_offset_by_peer.get(peer, 0)

    def _scroll_chat(self, delta: int) -> None:
        if not self.selected_peer:
            return
        current = self.chat_offset_by_peer.get(self.selected_peer, 0)
        target = max(0, current + delta)
        messages = self.store.get(self.selected_peer)
        max_offset = max(0, len(messages) - (self._last_chat_visible or 0))
        target = min(target, max_offset)
        self.chat_offset_by_peer[self.selected_peer] = target
