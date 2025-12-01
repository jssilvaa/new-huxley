#!/usr/bin/env python3
"""Conversation store with normalization and deduplication."""
from __future__ import annotations

import time
from typing import Any, Dict, Optional

JsonDict = Dict[str, Any]


class ConversationStore:
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
        return self._dedupe_and_clip(peer)

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
