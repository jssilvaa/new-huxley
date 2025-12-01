#!/usr/bin/env python3
"""Minimal CLI client for the Huxley messaging server."""
from __future__ import annotations

import argparse
import curses
import errno
import json
import select
import socket
import struct
import sys
import time
from collections import defaultdict
from dataclasses import dataclass
from typing import Any, Callable, Dict, Optional
import datetime
import textwrap
import curses
import errno
import json
import select
import socket
import struct
import sys
import time
from collections import defaultdict
from dataclasses import dataclass
from typing import Any, Callable, Dict, Optional
import datetime
import textwrap

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
            return {
                "type": "ERROR",
                "code": 400,
                "message": f"Failed to parse server JSON: {exc}"
            }
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
        elif cmd == "/users":
            self._display_response(self.message_client.list_users())
        elif cmd == "/online":
            self._display_response(self.message_client.list_online())
        elif cmd == "/history":
            if len(tokens) < 2:
                print("[ERROR] Usage: /history <username> [limit]")
                return True
            peer = tokens[1]
            try:
                limit = int(tokens[2]) if len(tokens) > 2 else 50
            except ValueError:
                print("[ERROR] Limit must be an integer")
                return True
            self._display_response(self.message_client.get_history(peer, limit))
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
        elif command in {"list_users"}:
            if not success:
                print(f"[FAIL] List users failed: {message}")
                return
            payload = response.get("payload") or {}
            users = payload.get("users", [])
            if not users:
                print("[INFO] No users found.")
                return
            print("Users:")
            for user in users:
                name = user.get("username", "?")
                online = user.get("online") is True
                marker = "*" if online else " "
                print(f" {marker} {name}")
        elif command in {"list_online"}:
            if not success:
                print(f"[FAIL] Online list failed: {message}")
                return
            payload = response.get("payload") or {}
            online = payload.get("users", [])
            if not online:
                print("[INFO] No users online.")
                return
            print("Online users:")
            for user in online:
                print(f" * {user.get('username', '?')}")
        elif command in {"get_history"}:
            if not success:
                print(f"[FAIL] History failed: {message}")
                return
            payload = response.get("payload") or {}
            peer = payload.get("with", "(unknown)")
            messages = payload.get("messages", [])
            if not messages:
                print(f"[INFO] No messages with {peer}.")
                return
            print(f"History with {peer}:")
            for msg in messages:
                ts = msg.get("timestamp") or ""
                ts_prefix = f"[{ts}] " if ts else ""
                sender = msg.get("from", "?")
                content = msg.get("content", "")
                print(f"{ts_prefix}{sender}: {content}")
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
            "/users                           - List all users and online status\n"
            "/online                          - List online users\n"
            "/history <user> [limit]          - Fetch recent messages with user\n"
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
        # Prefer stable DB id when available; fall back to content-based key.
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
        """Return a sortable score for timestamps; lower is earlier."""
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
        """
        Consolidate messages for a peer.
        Rules:
        - If a DB id exists, it is the primary identity.
        - Otherwise dedupe on (sorted participants, content).
        - Among duplicates prefer entries with valid/later timestamps, then longer timestamp strings (higher precision).
        """
        convo = self._conversations.get(peer, [])
        if not convo:
            return False

        def better(a: JsonDict, b: JsonDict) -> JsonDict:
            """Choose the better of two representations of the same message."""
            ts_a = a.get("timestamp") or ""
            ts_b = b.get("timestamp") or ""
            score_a = self._ts_score(ts_a)
            score_b = self._ts_score(ts_b)
            if score_a != score_b:
                return a if score_a > score_b else b  # prefer later/valid timestamp
            if len(ts_a) != len(ts_b):
                return a if len(ts_a) > len(ts_b) else b  # prefer more precision
            # fall back to keep existing deterministically
            return a

        by_id: dict[Any, JsonDict] = {}
        fallback: dict[tuple[str, str, str], JsonDict] = {}
        for raw in convo:
            msg = dict(raw)
            msg["timestamp"] = self._normalize_timestamp(msg.get("timestamp"))
            msg_id = msg.get("id")
            base = self._base_key(msg)
            participants_key = (base[1], base[2], base[3])  # (a,b,content)
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

        # Drop fallback entries that correspond to any id-backed message with same participants/content
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
        self.status = "Arrows: select user | Enter: send | u: refresh users | r: reload chat | q: quit"
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
            # Color palette:
            # 1: peer messages (blue-ish)
            # 2: own messages (bright/white)
            curses.init_pair(1, curses.COLOR_CYAN, -1)
            curses.init_pair(2, curses.COLOR_WHITE, -1)
            curses.init_pair(3, curses.COLOR_YELLOW, -1)  # status/info
            curses.init_pair(4, curses.COLOR_RED, -1)     # errors
            curses.init_pair(5, curses.COLOR_BLACK, curses.COLOR_CYAN)   # (reserved)
            curses.init_pair(6, curses.COLOR_WHITE, -1)   # chat background
        except curses.error:
            # Some terminals disable colors; proceed without them.
            pass

    def _login_flow(self) -> bool:
        animated = False
        while self.running and not self.username:
            self.stdscr.erase()
            # Centered HUXLEY banner
            height, width = self.stdscr.getmaxyx()
            banner = [
                r' _   _ _   ___  ___     _______   __',
                r'| | | | | | \ \/ / |   |  ___\ \ / /',
                r'| |_| | | | |\  /| |   | |__  \ V / ',
                r'|  _  | | | |/  \| |   |  __|  \ /  ',
                r'| | | | |_| / /\ \ |___| |___  | |  ',
                r'\_| |_/\___/\/  \/\___/\____/  \_/  ',
            ]
            start_row = max(0, height // 2 - len(banner) - 3)

            # One-time simple animation: reveal banner character by character.
            if not animated:
                max_len = max(len(line) for line in banner)
                for col in range(max_len):
                    for i, line in enumerate(banner):
                        visible = line[: col + 1]
                        x = max(0, (width - len(line)) // 2)
                        self._safe_addstr(self.stdscr, start_row + i, x, visible.ljust(len(line)), curses.A_BOLD)

                    # Simple loading percentage under the banner.
                    progress = int(((col + 1) / max_len) * 100)
                    load_text = f"Loading {progress:3d}%"
                    load_y = start_row + len(banner) + 1
                    load_x = max(0, (width - len(load_text)) // 2)
                    self._safe_addstr(self.stdscr, load_y, load_x, load_text, curses.A_DIM)

                    self.stdscr.refresh()
                    for _ in range(2):
                        ch_ani = self.stdscr.getch()
                        if ch_ani != -1:
                            break
                        time.sleep(0.02)

                # Flash banner between white and red for a short time.
                try:
                    curses.init_pair(7, curses.COLOR_RED, -1)
                    flash_pairs = [curses.color_pair(7), curses.A_BOLD]
                except curses.error:
                    flash_pairs = [curses.A_BOLD]
                start_flash = time.monotonic()
                ch_ani = -1
                while time.monotonic() - start_flash < 1.5:
                    for attr in flash_pairs:
                        for i, line in enumerate(banner):
                            x = max(0, (width - len(line)) // 2)
                            self._safe_addstr(self.stdscr, start_row + i, x, line, attr)
                        # Keep final loading line at 100%.
                        load_text = "Loading 100%"
                        load_y = start_row + len(banner) + 1
                        load_x = max(0, (width - len(load_text)) // 2)
                        self._safe_addstr(self.stdscr, load_y, load_x, load_text, curses.A_DIM)
                        self.stdscr.refresh()
                        ch_ani = self.stdscr.getch()
                        if ch_ani != -1:
                            break
                        time.sleep(0.1)
                    if ch_ani != -1:
                        break

                # Final stable white banner and loading text.
                for i, line in enumerate(banner):
                    x = max(0, (width - len(line)) // 2)
                    self._safe_addstr(self.stdscr, start_row + i, x, line, curses.A_BOLD)
                load_text = "Loading 100%"
                load_y = start_row + len(banner) + 1
                load_x = max(0, (width - len(load_text)) // 2)
                self._safe_addstr(self.stdscr, load_y, load_x, load_text, curses.A_DIM)
                animated = True
            else:
                for i, line in enumerate(banner):
                    x = max(0, (width - len(line)) // 2)
                    self._safe_addstr(self.stdscr, start_row + i, x, line, curses.A_BOLD)

            # Subtle header with host:port (top-left) and AT-83 (top-right).
            host_label = f"{self.host}:{self.port}"
            self._safe_addstr(self.stdscr, 0, 1, host_label[: max(0, width - 2)], curses.A_DIM)
            tag = "AT-83"
            tag_x = max(1, width - len(tag) - 2)
            self._safe_addstr(self.stdscr, 0, tag_x, tag, curses.A_DIM)

            # Vertical menu under banner
            menu_items = ["[L]ogin", "[R]egister", "[Q]uit"]
            if not hasattr(self, "_login_menu_index"):
                self._login_menu_index = 0
            menu_start_row = start_row + len(banner) + 2
            for idx, label in enumerate(menu_items):
                attr = curses.A_BOLD | curses.A_REVERSE if idx == self._login_menu_index else curses.A_DIM
                x = max(0, (width - len(label)) // 2)
                self._safe_addstr(self.stdscr, menu_start_row + idx, x, label, attr)

            if self.error_message:
                self._draw_login_error(self.stdscr, self.error_message)
            self.stdscr.refresh()
            ch = self.stdscr.getch()
            if self.error_message and ch in (curses.KEY_ENTER, 10, 13, 27):
                self._clear_error()
                continue
            # Menu navigation
            if ch in (curses.KEY_UP, ord("k")):
                self._login_menu_index = (self._login_menu_index - 1) % 3
                continue
            if ch in (curses.KEY_DOWN, ord("j")):
                self._login_menu_index = (self._login_menu_index + 1) % 3
                continue
            if ch in (curses.KEY_ENTER, 10, 13):
                ch = [ord("l"), ord("r"), ord("q")][self._login_menu_index]
            if ch in (ord("q"), ord("Q")):
                self.running = False
                break
            if ch in (ord("l"), ord("L")):
                creds = self._credentials_form("Login")
                if creds:
                    self._handle_login(creds)
            elif ch in (ord("r"), ord("R")):
                creds = self._credentials_form("Register")
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

    def _credentials_form(self, title: str) -> Optional[tuple[str, str]]:
        curses.curs_set(1)
        self.stdscr.nodelay(False)
        height, width = self.stdscr.getmaxyx()
        box_width = min(50, max(30, width - 10))
        box_height = 9
        start_y = max(0, (height - box_height) // 2)
        start_x = max(0, (width - box_width) // 2)

        username: list[str] = []
        password: list[str] = []
        field_index = 0  # 0=username, 1=password, 2=buttons

        def draw() -> None:
            self.stdscr.erase()
            # Draw outer box
            for y in range(start_y, start_y + box_height):
                for x in range(start_x, start_x + box_width):
                    if y in (start_y, start_y + box_height - 1):
                        ch = "-" if x not in (start_x, start_x + box_width - 1) else "+"
                    elif x in (start_x, start_x + box_width - 1):
                        ch = "|"
                    else:
                        ch = " "
                    self._safe_addstr(self.stdscr, y, x, ch)

            title_str = f" {title} "
            title_x = start_x + max(1, (box_width - len(title_str)) // 2)
            self._safe_addstr(self.stdscr, start_y, title_x, title_str, curses.A_BOLD)

            label_y = start_y + 2
            self._safe_addstr(self.stdscr, label_y, start_x + 2, "Username:")
            self._safe_addstr(self.stdscr, label_y + 2, start_x + 2, "Password:")

            uname_val = "".join(username)
            pwd_val = "*" * len(password)
            field_width = box_width - 4 - len("Username:") - 2
            uname_x = start_x + 2 + len("Username:") + 2
            pwd_x = start_x + 2 + len("Password:") + 2

            attr_user = curses.A_REVERSE if field_index == 0 else curses.A_NORMAL
            attr_pass = curses.A_REVERSE if field_index == 1 else curses.A_NORMAL

            self._safe_addstr(self.stdscr, label_y, uname_x, uname_val.ljust(field_width)[:field_width], attr_user)
            self._safe_addstr(self.stdscr, label_y + 2, pwd_x, pwd_val.ljust(field_width)[:field_width], attr_pass)

            buttons_y = start_y + box_height - 3
            btn_ok = "[ OK ]"
            btn_cancel = "[ Cancel ]"
            total_btn_width = len(btn_ok) + 2 + len(btn_cancel)
            btn_start_x = start_x + (box_width - total_btn_width) // 2
            attr_ok = curses.A_REVERSE if field_index == 2 else curses.A_NORMAL
            attr_cancel = curses.A_REVERSE if field_index == 3 else curses.A_NORMAL

            self._safe_addstr(self.stdscr, buttons_y, btn_start_x, btn_ok, attr_ok)
            self._safe_addstr(self.stdscr, buttons_y, btn_start_x + len(btn_ok) + 2, btn_cancel, attr_cancel)

            # Place cursor
            if field_index == 0:
                cursor_x = min(uname_x + len(uname_val), uname_x + field_width - 1)
                cursor_y = label_y
            elif field_index == 1:
                cursor_x = min(pwd_x + len(pwd_val), pwd_x + field_width - 1)
                cursor_y = label_y + 2
            else:
                cursor_y = buttons_y
                cursor_x = btn_start_x if field_index == 2 else btn_start_x + len(btn_ok) + 2
            self.stdscr.move(cursor_y, cursor_x)
            self.stdscr.refresh()

        # Use 4 selectable indices: 0=username, 1=password, 2=OK, 3=Cancel
        field_index = 0
        draw()
        while True:
            ch = self.stdscr.getch()
            if ch in (27,):  # ESC
                self.stdscr.nodelay(True)
                return None
            if ch in (9, curses.KEY_DOWN):  # TAB or Down
                field_index = (field_index + 1) % 4
                draw()
                continue
            if ch == curses.KEY_UP:
                field_index = (field_index - 1) % 4
                draw()
                continue
            if ch in (curses.KEY_ENTER, 10, 13):
                if field_index == 0:
                    # Username -> Password
                    field_index = 1
                    draw()
                    continue
                if field_index == 1:
                    # Password -> OK button
                    field_index = 2
                    draw()
                    continue
                if field_index == 3:
                    # Cancel
                    self.stdscr.nodelay(True)
                    return None
                if field_index == 2:
                    # OK
                    self.stdscr.nodelay(True)
                    return ("".join(username), "".join(password))

            target = None
            if field_index == 0:
                target = username
            elif field_index == 1:
                target = password

            if target is not None:
                if ch in (curses.KEY_BACKSPACE, 127, 8):
                    if target:
                        target.pop()
                    draw()
                    continue
                if 32 <= ch <= 126 and len(target) < 64:
                    target.append(chr(ch))
                    draw()
                    continue

        self.stdscr.nodelay(True)
        return None

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
            if ch in (27,):  # ESC
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
                # Avoid flooding the server with the same request if one is already pending.
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
            except Exception as exc:  # pragma: no cover - defensive
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
            # While awaiting quit confirm, run dedicated logout screen.
            self._logout_screen()
            return

        if self.focus_chat:
            if ch in (27,):  # ESC exits chat focus
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
            # Immediately enter the logout confirmation screen.
            self._logout_screen()
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
        stdscr.refresh()

    def _draw_header(self, stdscr: Any, width: int) -> None:
        username = self.username or "(not logged in)"
        header = f"{username} @ {self.host}:{self.port}"
        hint = "Enter: chat  ESC: list  PgUp/PgDn: scroll  q: quit  u/r: refresh"
        header_text = header[: max(0, width - 1)]
        self._safe_addstr(stdscr, 0, 0, header_text, curses.A_BOLD)
        if len(header_text) + 2 < width:
            self._safe_addstr(
                stdscr,
                0,
                len(header_text) + 2,
                hint[: max(0, width - len(header_text) - 3)],
                curses.A_DIM,
            )

    def _draw_users(self, win: Any) -> None:
        win.erase()
        # Use a neutral background; rely on text attributes for emphasis.
        win.bkgd(" ", curses.A_NORMAL)
        win.border()
        self._safe_addstr(win, 0, 2, " Users ", curses.A_BOLD)
        if not self.users:
            self._safe_addstr(win, 1, 1, "No users yet.")
            return
        height, width = win.getmaxyx()
        max_name_len = max(8, width - 8)
        for idx, user in enumerate(self.users, start=0):
            name = user.get("username", "?")
            online = user.get("online") is True
            unread = self.unread_counts.get(name, 0)
            if len(name) > max_name_len:
                name = name[: max_name_len - 1] + "…"
            marker = "*" if online else " "
            badge = f" [{unread}]" if unread > 0 else ""
            line = f"{marker} {name}{badge}"

            if idx == self.selected_index:
                # Selected row: subtle reverse + bold, no cyan fill.
                attr = curses.A_REVERSE | curses.A_BOLD
            else:
                attr = curses.color_pair(1) if online else 0
                if unread > 0:
                    attr |= curses.A_BOLD

            self._safe_addstr(win, 1 + idx, 1, line[: max(0, width - 2)], attr)

    def _draw_chat(self, win: Any) -> None:
        win.erase()
        win.bkgd(" ", curses.color_pair(6))
        win.border()
        title_peer = self.selected_peer or "No peer"
        self._safe_addstr(win, 0, 2, f" Chat with {title_peer} ", curses.A_BOLD)
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
        row = 0
        for msg in messages[start:]:
            if row >= visible_lines:
                break
            timestamp = msg.get("timestamp") or ""
            sender = msg.get("from", "?")
            content = msg.get("content", "")
            prefix = f"[{timestamp}] {sender}: " if timestamp else f"{sender}: "
            avail = max(1, width - 2)
            wrapped = textwrap.wrap(content, max(1, avail - len(prefix))) or [content]
            attr = curses.color_pair(2) if sender == self.username else curses.color_pair(1)
            first = f"{prefix}{wrapped[0]}"
            self._safe_addstr(win, 1 + row, 1, first[:avail], attr)
            row += 1
            indent = " " * len(prefix)
            for cont_line in wrapped[1:]:
                if row >= visible_lines:
                    break
                self._safe_addstr(win, 1 + row, 1, f"{indent}{cont_line}"[:avail], attr)
                row += 1

    def _draw_input(self, stdscr: Any, row: int, width: int) -> None:
        target = self.selected_peer or "(no user selected)"
        label = f"To {target}: "
        stdscr.addstr(row, 0, " " * (width - 1))
        self._safe_addstr(stdscr, row, 1, label, curses.A_BOLD)
        avail = max(0, width - 2 - len(label))
        self._safe_addstr(stdscr, row, 1 + len(label), self.input_buffer[:avail])
        try:
            curses.curs_set(1)
        except curses.error:
            pass
        cursor_x = min(len(label) + len(self.input_buffer), width - 2)
        stdscr.move(row, cursor_x)

    def _draw_status(self, stdscr: Any, row: int, width: int) -> None:
        base = self.error_message or self.status
        if self.error_message:
            message = f"! {base}"
        elif base:
            message = f"· {base}"
        else:
            message = ""
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

    def _draw_login_error(self, stdscr: Any, message: str) -> None:
        height, width = stdscr.getmaxyx()
        box_width = min(width - 4, 70)
        wrapped = textwrap.wrap(message, box_width - 4) or [message]
        box_height = min(len(wrapped) + 2, max(3, height // 3))
        start_row = min(height - box_height - 1, 2)
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
            # Writing outside visible area; ignore.
            pass

    def _show_error(self, message: str, modal: bool = True) -> None:
        self.error_message = message
        self.status = ""
        self.error_modal = modal

    def _clear_error(self) -> None:
        self.error_message = None
        self.error_modal = False
        if not self.username:
            # keep login flow prompt clean
            self.status = ""

    def _logout_and_quit(self) -> None:
        self.awaiting_quit_confirm = False
        resp = self._send_and_wait({"type": "LOGOUT"}, "logout", timeout=2.0)
        if resp and resp.get("success"):
            self.status = "Logged out."
        self.running = False

    def _logout_screen(self) -> None:
        """Show a HUXLEY-branded logout confirmation screen with YES/NO."""
        if not self.stdscr:
            self._logout_and_quit()
            return

        height, width = self.stdscr.getmaxyx()
        banner = [
            r' _   _ _   ___  ___     _______   __',
            r'| | | | | | \ \/ / |   |  ___\ \ / /',
            r'| |_| | | | |\  /| |   | |__  \ V / ',
            r'|  _  | | | |/  \| |   |  __|  \ /  ',
            r'| | | | |_| / /\ \ |___| |___  | |  ',
            r'\_| |_/\___/\/  \/\___/\____/  \_/  ',
        ]

        if not hasattr(self, "_logout_menu_index"):
            self._logout_menu_index = 0

        while self.awaiting_quit_confirm and self.running:
            self.stdscr.erase()
            height, width = self.stdscr.getmaxyx()
            start_row = max(1, height // 2 - len(banner) - 3)

            # Draw banner (no animation here, just static).
            for i, line in enumerate(banner):
                x = max(0, (width - len(line)) // 2)
                self._safe_addstr(self.stdscr, start_row + i, x, line, curses.A_BOLD)

            # Subtle header like login screen.
            host_label = f"{self.host}:{self.port}"
            self._safe_addstr(self.stdscr, 0, 1, host_label[: max(0, width - 2)], curses.A_DIM)
            tag = "AT-83"
            tag_x = max(1, width - len(tag) - 2)
            self._safe_addstr(self.stdscr, 0, tag_x, tag, curses.A_DIM)

            # Message and YES/NO menu.
            message = "Logout and quit?"
            msg_y = start_row + len(banner) + 2
            msg_x = max(0, (width - len(message)) // 2)
            self._safe_addstr(self.stdscr, msg_y, msg_x, message, curses.A_BOLD)

            menu_items = ["[ Yes ]", "[ No ]"]
            menu_start_row = msg_y + 2
            for idx, label in enumerate(menu_items):
                attr = curses.A_BOLD | curses.A_REVERSE if idx == self._logout_menu_index else curses.A_DIM
                x = max(0, (width - len(label)) // 2)
                self._safe_addstr(self.stdscr, menu_start_row + idx, x, label, attr)

            self.stdscr.refresh()
            ch = self.stdscr.getch()
            if ch == -1:
                continue
            if ch in (curses.KEY_UP, ord("k")):
                self._logout_menu_index = (self._logout_menu_index - 1) % 2
                continue
            if ch in (curses.KEY_DOWN, ord("j")):
                self._logout_menu_index = (self._logout_menu_index + 1) % 2
                continue
            if ch in (27,):  # ESC cancels
                self.awaiting_quit_confirm = False
                self.status = "Quit canceled."
                break
            if ch in (curses.KEY_ENTER, 10, 13):
                if self._logout_menu_index == 0:
                    # Yes
                    self._logout_and_quit()
                else:
                    # No
                    self.awaiting_quit_confirm = False
                    self.status = "Quit canceled."
                break

    def _show_server_warning(self, command: str, response: JsonDict) -> None:
        msg = response.get("message") or "Command failed"
        label = command.upper() if command else "SERVER"
        self._show_error(f"{label}: {msg}")

    def _mark_read(self, peer: str) -> None:
        if peer in self.unread_counts:
            self.unread_counts[peer] = 0

    def _scroll_chat(self, delta: int) -> None:
        if not self.selected_peer:
            return
        current = self.chat_offset_by_peer.get(self.selected_peer, 0)
        target = max(0, current + delta)
        messages = self.store.get(self.selected_peer)
        max_offset = max(0, len(messages) - (self._last_chat_visible or 0))
        target = min(target, max_offset)
        self.chat_offset_by_peer[self.selected_peer] = target


def parse_args(argv: Optional[list[str]] = None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Minimal CLI client for HuxleyServer")
    parser.add_argument("host", type=str, help="Server hostname or IP")
    parser.add_argument("port", type=int, help="Server port")
    parser.add_argument("--timeout", type=float, default=5.0, help="Connection timeout in seconds")
    parser.add_argument("--tui", action="store_true", help="Launch curses-based chat UI")
    parser.add_argument("--history-limit", type=int, default=50, help="Messages to pull per /history request")
    return parser.parse_args(argv)


def main(argv: Optional[list[str]] = None) -> int:
    args = parse_args(argv)
    if args.tui:
        return CursesChatApp(args.host, args.port, args.timeout, history_limit=args.history_limit).run()
    CliApp(args.host, args.port, args.timeout).run()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())