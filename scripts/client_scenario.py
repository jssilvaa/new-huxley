#!/usr/bin/env python3
"""Simulate conversational client scenarios against a running Huxley server."""
from __future__ import annotations

import argparse
import errno
import json
import random
import select
import string
import sys
import threading
import time
from dataclasses import dataclass
from pathlib import Path
from typing import Dict, List, Optional

REPO_ROOT = Path(__file__).resolve().parents[1]
if str(REPO_ROOT) not in sys.path:
    sys.path.insert(0, str(REPO_ROOT))

from client.client import MessageClient, ProtocolClient


@dataclass
class ScenarioUser:
    username: str
    password: str


class ScenarioSession:
    """Wraps a MessageClient with scripted behaviors."""

    def __init__(self, user: ScenarioUser, host: str, port: int, timeout: float) -> None:
        self.user = user
        self.host = host
        self.port = port
        self.protocol = ProtocolClient(timeout)
        self.client = MessageClient(self.protocol, self._handle_notification)
        self._lock = threading.Lock()
        self._connected = False

    @property
    def username(self) -> str:
        return self.user.username

    def connect(self) -> None:
        if self._connected:
            return
        self.protocol.connect(self.host, self.port)
        self._connected = True

    def register_and_login(self) -> None:
        self.connect()
        register_resp = self.client.register(self.user.username, self.user.password)
        if register_resp and register_resp.get("success"):
            print(f"[SCENARIO] Registered {self.username}")
        else:
            print(f"[SCENARIO] Registration skipped/failed for {self.username}: {register_resp}")
        login_resp = self.client.login(self.user.username, self.user.password)
        if not login_resp or not login_resp.get("success"):
            raise RuntimeError(f"Login failed for {self.username}: {login_resp}")
        print(f"[SCENARIO] {self.username} logged in")

    def send_message(self, recipient: str, content: str) -> None:
        response = self.client.send_message(recipient, content)
        if not response or not response.get("success"):
            raise RuntimeError(f"{self.username} failed to send to {recipient}: {response}")

    def logout(self) -> None:
        try:
            self.client.logout()
        finally:
            self.protocol.close()
            self._connected = False

    def drain_notifications(self, timeout: float = 0.0) -> None:
        sock = self.protocol.raw_socket
        if sock is None:
            return
        first_wait = True
        while True:
            wait_timeout: Optional[float]
            if first_wait and timeout > 0:
                wait_timeout = timeout
            else:
                wait_timeout = 0.0
            first_wait = False
            try:
                ready, _, _ = select.select([sock], [], [], wait_timeout)
            except OSError as exc:  # pragma: no cover - system dependent
                if exc.errno == errno.EINTR:
                    continue
                raise
            if not ready:
                break
            response = self.protocol.recv_response()
            if response is None:
                print(f"[SCENARIO] Server closed connection for {self.username}")
                break
            self._handle_notification(response)

    def _handle_notification(self, response: Optional[dict]) -> None:
        if response is None:
            return
        msg_type = response.get("type") or response.get("command")
        if msg_type in {"incoming_message", "incoming_message_response"}:
            sender = response.get("sender", "?")
            content = response.get("content", "")
            print(f"[MSG] {self.username} <- {sender}: {content}")
        elif msg_type == "timeout":
            print(f"[INFO] Session timeout for {self.username}: {response.get('message')}")
        elif msg_type == "ERROR":
            print(f"[ERROR] {self.username}: {response}")


def random_username(prefix: str, length: int = 6) -> str:
    suffix = ''.join(random.choices(string.ascii_lowercase + string.digits, k=length))
    return f"{prefix}{suffix}"


def random_password(length: int = 10) -> str:
    alphabet = string.ascii_letters + string.digits
    return ''.join(random.choices(alphabet, k=length))


def random_message(min_words: int = 3, max_words: int = 8) -> str:
    word_count = random.randint(min_words, max_words)
    words = [''.join(random.choices(string.ascii_lowercase, k=random.randint(3, 8))) for _ in range(word_count)]
    return ' '.join(words)


def build_users(count: int, prefix: str) -> List[ScenarioUser]:
    return [ScenarioUser(random_username(prefix), random_password()) for _ in range(count)]


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Automate conversational client scenarios against HuxleyServer")
    parser.add_argument("host", type=str, help="Server hostname or IP")
    parser.add_argument("port", type=int, help="Server port")
    parser.add_argument("--clients", type=int, default=2, help="Number of simulated clients (>=2)")
    parser.add_argument("--rounds", type=int, default=5, help="Total message rounds to run")
    parser.add_argument("--min-delay", type=float, default=0.5, help="Minimum delay between rounds (seconds)")
    parser.add_argument("--max-delay", type=float, default=1.5, help="Maximum delay between rounds (seconds)")
    parser.add_argument("--timeout", type=float, default=5.0, help="Socket timeout (seconds)")
    parser.add_argument("--user-prefix", type=str, default="scenario_", help="Prefix for generated usernames")
    parser.add_argument("--seed", type=int, default=None, help="Random seed for reproducibility")
    default_convo = Path(__file__).with_name("conversations.json")
    parser.add_argument(
        "--conversation-file",
        type=Path,
        default=default_convo,
        help=f"JSON file containing scripted conversations (default: {default_convo})",
    )
    return parser.parse_args()


def validate_args(args: argparse.Namespace) -> None:
    if args.clients < 2:
        raise SystemExit("--clients must be >= 2")
    if args.min_delay < 0 or args.max_delay < 0:
        raise SystemExit("Delays must be non-negative")
    if args.min_delay > args.max_delay:
        raise SystemExit("--min-delay must be <= --max-delay")


def load_conversations(path: Path) -> List[Dict[str, object]]:
    if not path.exists():
        print(f"[WARN] Conversation file {path} not found; falling back to random chatter")
        return []
    try:
        with path.open("r", encoding="utf-8") as handle:
            data = json.load(handle)
    except json.JSONDecodeError as exc:  # pragma: no cover - invalid data is user provided
        print(f"[WARN] Failed to parse conversation file {path}: {exc}; using random chatter")
        return []
    if not isinstance(data, list):
        print(f"[WARN] Conversation file {path} must contain a list; using random chatter")
        return []
    valid = []
    for convo in data:
        if not isinstance(convo, dict):
            continue
        messages = convo.get("messages")
        if not isinstance(messages, list) or not messages:
            continue
        valid.append(convo)
    if not valid:
        print(f"[WARN] No valid conversations in {path}; using random chatter")
    return valid


def assign_roles(conversation: Dict[str, object], sessions: List[ScenarioSession]) -> Dict[str, ScenarioSession]:
    role_names = []
    for message in conversation["messages"]:  # type: ignore[index]
        role_names.extend([message.get("from"), message.get("to")])
    unique_roles = sorted({role for role in role_names if isinstance(role, str)})
    if len(unique_roles) > len(sessions):
        raise SystemExit(
            f"Conversation '{conversation.get('title', 'Unnamed')}' needs {len(unique_roles)} participants but only {len(sessions)} clients provided"
        )
    selected_sessions = random.sample(sessions, len(unique_roles))
    return dict(zip(unique_roles, selected_sessions))


def replay_conversation(
    conversation: Dict[str, object],
    sessions: List[ScenarioSession],
    min_delay: float,
    max_delay: float,
) -> None:
    role_map = assign_roles(conversation, sessions)
    title = conversation.get("title", "Untitled Scenario")
    description = conversation.get("description")
    print(f"[SCENARIO] Replaying conversation: {title}")
    if description:
        print(f"[SCENARIO] {description}")
    for idx, message in enumerate(conversation["messages"], start=1):  # type: ignore[index]
        sender_role = message.get("from")
        recipient_role = message.get("to")
        text = message.get("text", "")
        if not isinstance(sender_role, str) or not isinstance(recipient_role, str):
            continue
        sender_session = role_map[sender_role]
        recipient_session = role_map[recipient_role]
        print(f"[TURN {idx}] {sender_session.username} -> {recipient_session.username}: {text}")
        sender_session.send_message(recipient_session.username, text)
        for session in sessions:
            session.drain_notifications(timeout=0.2)
        delay = random.uniform(min_delay, max_delay)
        time.sleep(delay)


def main() -> int:
    args = parse_args()
    validate_args(args)
    random.seed(args.seed)

    users = build_users(args.clients, args.user_prefix)
    sessions = [ScenarioSession(user, args.host, args.port, args.timeout) for user in users]
    conversations = load_conversations(args.conversation_file)
    scripted_available = bool(conversations)

    try:
        for session in sessions:
            session.register_and_login()
            time.sleep(random.uniform(args.min_delay, args.max_delay))
        if scripted_available:
            conversation = random.choice(conversations)
            replay_conversation(conversation, sessions, args.min_delay, args.max_delay)
        else:
            for round_idx in range(1, args.rounds + 1):
                sender = random.choice(sessions)
                possible_receivers = [s for s in sessions if s is not sender]
                recipient_session = random.choice(possible_receivers)
                message = random_message()
                print(f"[ROUND {round_idx}] {sender.username} -> {recipient_session.username}: {message}")
                sender.send_message(recipient_session.username, message)
                for session in sessions:
                    session.drain_notifications(timeout=0.2)
                delay = random.uniform(args.min_delay, args.max_delay)
                time.sleep(delay)

    finally:
        for session in sessions:
            try:
                session.logout()
            except Exception as exc:  # pragma: no cover - cleanup path
                print(f"[WARN] Failed to logout {session.username}: {exc}")

    return 0


if __name__ == "__main__":
    sys.exit(main())
