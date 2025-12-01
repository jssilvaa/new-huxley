#!/usr/bin/env python3
from __future__ import annotations

import json
import socket
import struct
from dataclasses import dataclass
from typing import Any, Callable, Dict, Optional

JsonDict = Dict[str, Any]


def _extract_command(response: JsonDict) -> Optional[str]:
    """Return a lowercase command/type identifier if present."""
    for key in ("type", "command"):
        value = response.get(key)
        if isinstance(value, str) and value:
            return value.strip().lower()
    return None


class ProtocolClient:
    """Implements the length-prefixed JSON protocol over TCP."""

    def __init__(self, timeout: float = 5.0) -> None:
        self._timeout = timeout
        self._sock: Optional[socket.socket] = None

    def connect(self, host: str, port: int) -> None:
        self._sock = socket.create_connection((host, port), timeout=self._timeout)
        self._sock.settimeout(None)

    def close(self) -> None:
        if self._sock:
            try:
                self._sock.close()
            finally:
                self._sock = None

    @property
    def socket(self) -> socket.socket:
        if self._sock is None:
            raise RuntimeError("ProtocolClient is not connected")
        return self._sock

    @property
    def raw_socket(self) -> Optional[socket.socket]:
        """Returns the underlying socket for readiness checks."""
        return self._sock

    def send_command(self, payload: JsonDict) -> None:
        sock = self.socket
        data = json.dumps(payload, separators=(",", ":"), ensure_ascii=True).encode("utf-8")
        header = struct.pack("!I", len(data))
        sock.sendall(header)
        sock.sendall(data)

    def recv_response(self) -> Optional[JsonDict]:
        sock = self.socket
        header = self._recv_exact(sock, 4)
        if header is None:
            return None
        (length,) = struct.unpack("!I", header)
        if length == 0:
            return {}
        payload = self._recv_exact(sock, length)
        if payload is None:
            return None
        try:
            return json.loads(payload.decode("utf-8"))
        except json.JSONDecodeError as exc:
            return {"type": "ERROR", "code": 400, "message": f"Failed to parse server JSON: {exc}"}

    @staticmethod
    def _recv_exact(sock: socket.socket, count: int) -> Optional[bytes]:
        buf = bytearray()
        while len(buf) < count:
            chunk = sock.recv(count - len(buf))
            if not chunk:
                return None
            buf.extend(chunk)
        return bytes(buf)


@dataclass
class MessageClient:
    protocol: ProtocolClient
    notification_handler: Optional[Callable[[JsonDict], None]] = None
    username: Optional[str] = None

    _ASYNC_COMMANDS = {"incoming_message", "incoming_message_response", "timeout"}

    def register(self, username: str, password: str) -> Optional[JsonDict]:
        self.protocol.send_command({"type": "REGISTER", "username": username, "password": password})
        return self._recv_command_response()

    def login(self, username: str, password: str) -> Optional[JsonDict]:
        self.protocol.send_command({"type": "LOGIN", "username": username, "password": password})
        response = self._recv_command_response()
        if response and response.get("success"):
            self.username = username
        return response

    def logout(self) -> Optional[JsonDict]:
        self.protocol.send_command({"type": "LOGOUT"})
        response = self._recv_command_response()
        if response and response.get("success"):
            self.username = None
        return response

    def send_message(self, recipient: str, content: str, timestamp: str) -> Optional[JsonDict]:
        if not self.username:
            raise RuntimeError("You must /login before sending messages")
        payload = {"type": "SEND_MESSAGE", "recipient": recipient, "content": content, "timestamp": timestamp}
        self.protocol.send_command(payload)
        return self._recv_command_response()

    def list_users(self) -> Optional[JsonDict]:
        self.protocol.send_command({"type": "LIST_USERS"})
        return self._recv_command_response()

    def list_online(self) -> Optional[JsonDict]:
        self.protocol.send_command({"type": "LIST_ONLINE"})
        return self._recv_command_response()

    def get_history(self, peer: str, limit: int = 50, offset: int = 0) -> Optional[JsonDict]:
        if not peer:
            raise RuntimeError("Peer username is required")
        payload = {"type": "GET_HISTORY", "with": peer, "limit": limit, "offset": offset}
        self.protocol.send_command(payload)
        return self._recv_command_response()

    def _recv_command_response(self) -> Optional[JsonDict]:
        while True:
            response = self.protocol.recv_response()
            if response is None:
                return None
            command = _extract_command(response)
            if command in self._ASYNC_COMMANDS or "success" not in response:
                if self.notification_handler:
                    self.notification_handler(response)
                continue
            return response
