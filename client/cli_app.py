#!/usr/bin/env python3
from __future__ import annotations

import select
import sys
from datetime import datetime
from typing import Optional

from .protocol import MessageClient, ProtocolClient, JsonDict


class CliApp:
    """Line-based CLI client for the Huxley server."""

    def __init__(self, host: str, port: int, timeout: float = 5.0) -> None:
        self.host = host
        self.port = port
        self.protocol = ProtocolClient(timeout=timeout)
        self.client = MessageClient(self.protocol, notification_handler=self._handle_notification)
        self.running = True

    def run(self) -> None:
        self.protocol.connect(self.host, self.port)
        print(f"Connected to {self.host}:{self.port}")
        print("Type /help for available commands.")
        self._print_prompt()

        try:
            while self.running:
                inputs = [sys.stdin]
                raw_sock = self.protocol.raw_socket
                if raw_sock is not None:
                    inputs.append(raw_sock)

                readable, _, _ = select.select(inputs, [], [], 0.5)

                if raw_sock is not None and raw_sock in readable:
                    self._consume_socket_events(block=False)

                if sys.stdin in readable:
                    line = sys.stdin.readline()
                    if not line:
                        break
                    line = line.rstrip("\n")
                    if line.startswith("/"):
                        self._handle_command(line)
                    else:
                        print("Commands must start with '/'. Try /help.")
                    self._print_prompt()
        finally:
            self.protocol.close()

    def _handle_command(self, line: str) -> None:
        parts = line.split()
        cmd = parts[0][1:].lower()
        args = parts[1:]

        if cmd in {"quit", "q", "exit"}:
            print("Quitting...")
            self.running = False
            return

        if cmd == "help":
            self._print_help()
            return

        if cmd == "register":
            if len(args) != 2:
                print("Usage: /register <username> <password>")
                return
            username, password = args
            resp = self.client.register(username, password)
            self._display_response(resp)
            return

        if cmd == "login":
            if len(args) != 2:
                print("Usage: /login <username> <password>")
                return
            username, password = args
            resp = self.client.login(username, password)
            self._display_response(resp)
            return

        if cmd == "logout":
            resp = self.client.logout()
            self._display_response(resp)
            return

        if cmd == "users":
            resp = self.client.list_users()
            self._display_response(resp)
            return

        if cmd == "online":
            resp = self.client.list_online()
            self._display_response(resp)
            return

        if cmd == "history":
            if len(args) < 1:
                print("Usage: /history <username> [limit] [offset]")
                return
            peer = args[0]
            limit = int(args[1]) if len(args) > 1 else 50
            offset = int(args[2]) if len(args) > 2 else 0
            resp = self.client.get_history(peer, limit=limit, offset=offset)
            self._display_response(resp)
            return

        if cmd == "send":
            if len(args) < 2:
                print("Usage: /send <recipient> <message>")
                return
            recipient = args[0]
            content = " ".join(args[1:])
            ts = datetime.utcnow().isoformat(timespec="seconds") + "Z"
            resp = self.client.send_message(recipient, content, ts)
            self._display_response(resp)
            return

        if cmd == "whoami":
            if self.client.username:
                print(f"You are logged in as: {self.client.username}")
            else:
                print("You are not logged in.")
            return

        if cmd == "recv":
            self._consume_socket_events(block=True)
            return

        print(f"Unknown command: /{cmd}. Try /help.")

    def _consume_socket_events(self, block: bool = False, timeout: float = 0.5) -> None:
        raw_sock = self.protocol.raw_socket
        if raw_sock is None:
            return

        while True:
            r, _, _ = select.select([raw_sock], [], [], None if block else timeout)
            if raw_sock not in r:
                break
            response = self.protocol.recv_response()
            if response is None:
                print("Server closed connection.")
                self.running = False
                break
            self._display_response(response)
            if not block:
                break

    def _display_response(self, response: Optional[JsonDict]) -> None:
        if response is None:
            print("<no response>")
            return
        cmd = response.get("type") or response.get("command") or "(unknown)"
        success = response.get("success")

        if cmd == "INCOMING_MESSAGE":
            msg = response.get("message") or {}
            sender = msg.get("sender", "?")
            content = msg.get("content", "")
            ts = msg.get("timestamp", "")
            print(f"[{ts}] {sender}: {content}")
            return

        if success is True:
            print(f"[{cmd}] success: {response}")
        elif success is False:
            print(f"[{cmd}] ERROR: {response}")
        else:
            print(response)

    def _print_help(self) -> None:
        print("Available commands:")
        print("  /help                          Show this help message")
        print("  /register <u> <p>              Register a new user")
        print("  /login <u> <p>                 Login as user")
        print("  /logout                        Logout")
        print("  /users                         List all users")
        print("  /online                        List online users")
        print("  /history <u> [limit] [offset]  Show chat history with user")
        print("  /send <u> <msg>                Send message to user")
        print("  /whoami                        Show current username")
        print("  /recv                          Receive pending messages")
        print("  /quit                          Quit the client")

    def _print_prompt(self) -> None:
        sys.stdout.write("huxley> ")
        sys.stdout.flush()

    def _handle_notification(self, response: JsonDict) -> None:
        # For now, just print async events immediately.
        print("\n[async]", response)
        self._print_prompt()
