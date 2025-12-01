# Huxley Wire Protocol

JSON over TCP with a 4-byte big-endian length prefix framing the UTF-8 payload.

## Envelope and Routing

- Every packet MUST include either `type` or `command` (string). This is the routing key.
- Replies SHOULD reuse the request key exactly; `*_response` is tolerated by clients but not encouraged.
- Command replies MUST include:
  - `success` (bool)
  - `message` (string, human-readable)
  - Optional: `payload` (object), `id` (int), `timestamp` (string), `sender`, `recipient`, `content`
- Async notifications:
  - `incoming_message` with fields: `id` (int, DB message id), `sender`, `recipient` (optional), `content`, `timestamp`
  - `timeout` for session expiry

## Commands

| Command        | Request Fields                      | Response Payload                          |
|----------------|-------------------------------------|-------------------------------------------|
| REGISTER       | `username`, `password`              | none                                      |
| LOGIN          | `username`, `password`              | none                                      |
| LOGOUT         | –                                   | none                                      |
| SEND_MESSAGE   | `recipient`, `content`, `timestamp` | none                                      |
| LIST_USERS     | –                                   | `users`: `[{username, online}]`           |
| LIST_ONLINE    | –                                   | `users`: `[{username}]`                   |
| GET_HISTORY    | `with`, `limit`, `offset`           | `messages`: `[{id, from, to, content, timestamp}]` |

## Message Identity

- Every stored/delivered message SHOULD carry its database `id` in both realtime notifications and history responses.
- Clients dedupe primarily on `id`; if absent, they fall back to `(participants-as-set, content, timestamp)`.
- Timestamps SHOULD be `YYYY-MM-DDTHH:MM:SSZ`; fractional seconds are optional but discouraged for consistency.

## Error Semantics

- On protocol/validation errors, respond with `success: false`, a descriptive `message`, and the original routing key in `command`.
- On malformed JSON, clients synthesize an `{"type": "ERROR", "code": 400, "message": ...}` locally.

## Framing

- Frame: `[uint32 length][JSON bytes]`, length excludes the 4-byte header, network byte order (big-endian).
- Zero-length payload (`length == 0`) is treated as `{}`.

