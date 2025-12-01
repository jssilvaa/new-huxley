#!/usr/bin/env python3
"""
Lightweight SQLite inspector for Huxley.

Usage:
  python scripts/db_inspect.py users [--db huxley.db]
  python scripts/db_inspect.py undelivered [--db huxley.db]
  python scripts/db_inspect.py logs [--db huxley.db] [--last N]
  python scripts/db_inspect.py messages --with USER [--db huxley.db] [--limit N] [--offset N]
"""
from __future__ import annotations

import argparse
import sqlite3
from typing import Any, Iterable


def connect(db_path: str) -> sqlite3.Connection:
    conn = sqlite3.connect(db_path)
    conn.row_factory = sqlite3.Row
    return conn


def print_rows(rows: Iterable[sqlite3.Row]) -> None:
    for row in rows:
        items = [f"{k}={row[k]!r}" for k in row.keys()]
        print(", ".join(items))


def cmd_users(conn: sqlite3.Connection) -> None:
    rows = conn.execute(
        "SELECT id, username, created_at FROM users ORDER BY username;"
    ).fetchall()
    print_rows(rows)


def cmd_undelivered(conn: sqlite3.Connection) -> None:
    rows = conn.execute(
        """
        SELECT m.id, u_from.username AS sender, u_to.username AS recipient,
               m.timestamp, m.delivered
        FROM messages m
        JOIN users u_from ON u_from.id = m.sender_id
        JOIN users u_to   ON u_to.id   = m.recipient_id
        WHERE m.delivered = 0
        ORDER BY m.timestamp ASC, m.id ASC;
        """
    ).fetchall()
    print_rows(rows)


def cmd_logs(conn: sqlite3.Connection, last: int) -> None:
    rows = conn.execute(
        "SELECT id, level, log, timestamp FROM logs ORDER BY id DESC LIMIT ?;",
        (last,),
    ).fetchall()
    print_rows(reversed(rows))


def cmd_messages(conn: sqlite3.Connection, with_user: str, limit: int, offset: int) -> None:
    rows = conn.execute(
        """
        SELECT m.id, u_from.username AS sender, u_to.username AS recipient,
               m.timestamp, m.delivered
        FROM messages m
        JOIN users u_from ON u_from.id = m.sender_id
        JOIN users u_to   ON u_to.id   = m.recipient_id
        WHERE (u_from.username = ? AND u_to.username = ?)
           OR (u_from.username = ? AND u_to.username = ?)
        ORDER BY m.timestamp ASC, m.id ASC
        LIMIT ? OFFSET ?;
        """,
        (with_user, current_user(conn), current_user(conn), with_user, limit, offset),
    ).fetchall()
    print_rows(rows)


def current_user(conn: sqlite3.Connection) -> str:
    # Placeholder: derive a "me" user from config if available; default to empty.
    return ""


def main() -> int:
    parser = argparse.ArgumentParser(description="SQLite inspector for Huxley DB")
    parser.add_argument("--db", default="huxley.db", help="Path to SQLite database")
    sub = parser.add_subparsers(dest="cmd", required=True)

    sub.add_parser("users")
    sub.add_parser("undelivered")
    p_logs = sub.add_parser("logs")
    p_logs.add_argument("--last", type=int, default=50, help="Number of log rows to show")

    p_msgs = sub.add_parser("messages")
    p_msgs.add_argument("--with", dest="with_user", required=True, help="Peer username")
    p_msgs.add_argument("--limit", type=int, default=50)
    p_msgs.add_argument("--offset", type=int, default=0)

    args = parser.parse_args()
    conn = connect(args.db)

    if args.cmd == "users":
        cmd_users(conn)
    elif args.cmd == "undelivered":
        cmd_undelivered(conn)
    elif args.cmd == "logs":
        cmd_logs(conn, args.last)
    elif args.cmd == "messages":
        cmd_messages(conn, args.with_user, args.limit, args.offset)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
