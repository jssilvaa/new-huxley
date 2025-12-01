#!/usr/bin/env python3
"""Minimal CLI client for the Huxley messaging server."""
from __future__ import annotations

import argparse
import errno
import json
import select
import socket
import struct
import sys
from dataclasses import dataclass
from typing import Any, Callable, Dict, Optional
# timestamp 
import datetime

JsonDict = Dict[str, Any]

# simple type extraction 
def _extract_command(response: JsonDict) -> Optional[str]:
    """Return a lowercase command/type identifier if present."""
    for key in ("type", "command"):
        value = response.get(key)

        # check for value 
        if isinstance(value, str) and value:
            return value.strip().lower() # trim whitespaces, return lowecase command
    return None


class ProtocolClient:
    """Implements the length-prefixed JSON protocol over TCP."""

    # define init method: timeout default is 5.0 seconds
    def __init__(self, timeout: float = 5.0) -> None:
        self._timeout = timeout
        self._sock: Optional[socket.socket] = None

    # public method 
    def connect(self, host: str, port: int) -> None:
        self._sock = socket.create_connection((host, port), timeout=self._timeout)
        self._sock.settimeout(None)  # switch back to blocking I/O once connected

    # public method 
    def close(self) -> None:
        if self._sock:
            try:
                self._sock.close()
            finally:
                self._sock = None

    # getters for socket 
    @property # return socket object or error ; getter 1 
    def socket(self) -> socket.socket:
        if self._sock is None:
            raise RuntimeError("ProtocolClient is not connected")
        return self._sock

    @property # return raw socket object or None ; getter 2 
    def raw_socket(self) -> Optional[socket.socket]:
        """Returns the underlying socket for readiness checks."""
        return self._sock

    # public method
    def send_command(self, payload: JsonDict) -> None:
        sock = self.socket
        data = json.dumps(payload, separators=(",", ":"), ensure_ascii=True).encode("utf-8")
        header = struct.pack("!I", len(data))
        sock.sendall(header) # send data length  
        sock.sendall(data)   # send data payload

    # public method
    def recv_response(self) -> Optional[JsonDict]:
        sock = self.socket
        header = self._recv_exact(sock, 4) # read 4 bytes header
        if header is None:
            return None
        (length,) = struct.unpack("!I", header)  # unpack length and dummy identifier 
        if length == 0:
            return {}
        payload = self._recv_exact(sock, length) # read actual data 
        if payload is None:
            return None
        # decode JSON payload and handle exception
        try:
            return json.loads(payload.decode("utf-8"))
        except json.JSONDecodeError as exc:
            return JsonDict({
                "type": "ERROR",
                "code": 400,
                "message": f"Failed to parse server JSON: {exc}"
            })
    # private method
    @staticmethod 
    def _recv_exact(sock: socket.socket, count: int) -> Optional[bytes]:
        buf = bytearray()
        # continue receiving until we get 'count' bytes
        while len(buf) < count:
            chunk = sock.recv(count - len(buf))
            if not chunk:
                return None
            buf.extend(chunk)
        # return bytes object
        return bytes(buf)
    
@dataclass
class MessageClient:
    protocol: ProtocolClient
    notification_handler: Optional[Callable[[JsonDict], None]] = None
    username: Optional[str] = None

    # current async commands array 
    _ASYNC_COMMANDS = {
        "incoming_message",
        "incoming_message_response",
        "timeout",
    }

    # public method, register user, retrieve answer
    def register(self, username: str, password: str) -> Optional[JsonDict]:
        self.protocol.send_command({"type": "REGISTER", "username": username, "password": password})
        return self._recv_command_response()

    # public method, login user, retrieve answer 
    def login(self, username: str, password: str) -> Optional[JsonDict]:
        self.protocol.send_command({"type": "LOGIN", "username": username, "password": password})
        response = self._recv_command_response()
        if response and response.get("success"):
            self.username = username
        return response

    # public method
    def logout(self) -> Optional[JsonDict]:
        self.protocol.send_command({"type": "LOGOUT"})
        response = self._recv_command_response()
        if response and response.get("success"):
             self.username = None
        return response

    # public method
    def send_message(self, recipient: str, content: str) -> Optional[JsonDict]:
        if not self.username:
            raise RuntimeError("You must /login before sending messages")
        timestamp = datetime.datetime.utcnow().isoformat() + "Z"
        payload = {"type": "SEND_MESSAGE", "recipient": recipient, "content": content, "timestamp": timestamp}
        self.protocol.send_command(payload)
        return self._recv_command_response()

    # private method, handle handshake response
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

class CliApp:
    def __init__(self, host: str, port: int, timeout: float) -> None:
        self.protocol = ProtocolClient(timeout)
        self.message_client = MessageClient(self.protocol, self._display_response)
        self.host = host
        self.port = port
        self._prompt_visible = False # track prompt visibility

    # public
    def run(self) -> None:
    
        # Attempt to connect to the user specified (host,port)
        try:
            self.protocol.connect(self.host, self.port) 
        except OSError as exc:
            print(f"[FATAL] Unable to connect to {self.host}:{self.port} — {exc}")
            sys.exit(1)

        print(f"Connected to {self.host}:{self.port}. Type /help for commands.")
        try:
            while True:
                self._print_prompt()

                # wait for input from stdin or socket
                # we'll be handling two types of inputs: user commands (inputs) and server messages (writes to the socket fds)
                inputs = [sys.stdin]

                # add socket to inputs if connected
                sock = self.protocol.raw_socket
                if sock is not None:
                    inputs.append(sock)
                try:
                    # wait for readiness (blocking, not using epoll for simplicity)
                    ready, _, _ = select.select(inputs, [], [])
                except OSError as exc:
                    if exc.errno == errno.EINTR:
                        continue
                    raise

                # consume socket events when ready, sock > inputs priority
                if sock is not None and sock in ready:
                    self._consume_socket_events(block=False)
                    continue

                # handle user input when ready
                if sys.stdin in ready:
                    line = sys.stdin.readline()
                    if line == "":
                        print()
                        break
                    line = line.strip()
                    if not line:
                        continue
                    if not line.startswith("/"):
                        print("Commands must start with '/'. Type /help for a list.")
                        continue
                    if not self._handle_command(line):
                        break
                    self._consume_socket_events(block=False)
        finally:
            self.protocol.close()

    #private 
    def _handle_command(self, line: str) -> bool:
        tokens = line.strip().split()
        if not tokens:
            return True
        # extract mnemonic command
        cmd = tokens[0].lower()
        if cmd == "/help":
            self._print_help()
        elif cmd == "/quit":
            return False
        elif cmd == "/register" and len(tokens) == 3:
            self._display_response(self.message_client.register(tokens[1], tokens[2]))
        elif cmd == "/login" and len(tokens) == 3:
            self._display_response(self.message_client.login(tokens[1], tokens[2]))
        elif cmd == "/logout":
            self._display_response(self.message_client.logout())
        elif cmd == "/send":
            parts = line.split(maxsplit=2)
            if len(parts) < 3:
                print("[ERROR] Usage: /send <recipient> <message>")
                return True
            recipient = parts[1]
            message = parts[2]
            try:
                response = self.message_client.send_message(recipient, message)
            except RuntimeError as exc:
                print(f"[ERROR] {exc}")
            else:
                self._display_response(response)
        elif cmd == "/whoami":
            current = self.message_client.username or "(not logged in)"
            print(f"Current user: {current}")
        elif cmd == "/recv":
            timeout = float(tokens[1]) if len(tokens) > 1 else None
            received = self._consume_socket_events(block=True, timeout=timeout)
            if not received:
                print("[INFO] No messages available.")
        else:
            print("[ERROR] Unknown or malformed command. Type /help for usage.")
        return True

    #private 
    def _consume_socket_events(self, block: bool, timeout: Optional[float] = None) -> bool:
        sock = self.protocol.raw_socket
        if sock is None:
            return False
        received_any = False
        first_wait = True
        while True:
            # handle timeout values 
            wait_timeout: Optional[float]
            # if defined block and first wait, use timeout; else no wait
            if block and first_wait:
                wait_timeout = timeout
            else:
                wait_timeout = 0.0
            first_wait = False
            # nullify negative timeouts
            if wait_timeout is not None and wait_timeout < 0:
                wait_timeout = 0.0

            # wait for socket readiness, use defined timeout
            try:
                ready, _, _ = select.select([sock], [], [], wait_timeout) # wait for socket readiness
            except OSError as exc:
                # interrupted system call, retry
                if exc.errno == errno.EINTR:
                    continue
                raise
            # no more data to read
            if not ready:
                break
            response = self.protocol.recv_response()

            # server closed connection
            if response is None:
                print("\n[INFO] Server closed the connection.")
                sys.exit(0)
            received_any = True
            # display response 
            self._display_response(response)
            block = False  # after first response, switch to non-blocking to flush rest
        return received_any

    #private
    def _display_response(self, response: Optional[JsonDict]) -> None:
        self._clear_prompt()
        if response is None:
            print("[ERROR] No response from server (connection closed?)")
            return
        msg_type = response.get("type")
        if msg_type == "ERROR":
            print(f"[ERROR {response.get('code', '?')}] {response.get('message', 'Unknown error')}")
            return

        command = _extract_command(response)
        success = response.get("success")
        message = response.get("message", "")

        if command in {"register", "register_response"}:
            print("[SUCCESS] User registered" if success else f"[FAIL] Registration failed: {message}")
        elif command in {"login", "login_response"}:
            print("[SUCCESS] Logged in" if success else f"[FAIL] Login failed: {message}")
        elif command in {"logout", "logout_response"}:
            print("[SUCCESS] Logged out" if success else f"[FAIL] Logout failed: {message}")
        elif command in {"send_message", "send_message_response"}:
            print("[SUCCESS] Message sent" if success else f"[FAIL] Send failed: {message}")
        elif command in {"incoming_message", "incoming_message_response"}:
            sender = response.get("sender", "?")
            content = response.get("content", "")
            timestamp = response.get("timestamp")
            suffix = f" ({timestamp})" if timestamp else ""
            print(f"[MSG] {sender}: {content}{suffix}")
        elif command in {"timeout"}:
            print(f"[INFO] Session timeout: {message}")
        else:
            print(f"[INFO] {json.dumps(response)}")

    #private
    def _print_help(self) -> None:
        print(
            "Available commands:\n"
            "/register <username> <password>  - Create a new account\n"
            "/login <username> <password>     - Authenticate and store session\n"
            "/send <recipient> <message>      - Send a message\n"
            "/logout                          - Logout current session\n"
            "/recv [timeout]                  - Wait for incoming messages (seconds)\n"
            "/whoami                          - Display current user\n"
            "/quit                            - Exit the client"
        )

    #private
    def _print_prompt(self) -> None:
        if not self._prompt_visible:
            print("> ", end="", flush=True)
            self._prompt_visible = True

    #private
    def _clear_prompt(self) -> None:
        if self._prompt_visible:
            print("\r", end="")
            self._prompt_visible = False


def parse_args(argv: Optional[list[str]] = None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Minimal CLI client for HuxleyServer")
    parser.add_argument("host", type=str, help="Server hostname or IP")
    parser.add_argument("port", type=int, help="Server port")
    parser.add_argument("--timeout", type=float, default=5.0, help="Connection timeout in seconds")
    return parser.parse_args(argv)


def main(argv: Optional[list[str]] = None) -> int:
    args = parse_args(argv)
    CliApp(args.host, args.port, args.timeout).run()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())