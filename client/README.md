# Huxley Minimal CLI Client

This Python script exercises the server's length-prefixed JSON protocol without any third-party dependencies. It is intended for quick manual tests (e.g., verifying `SingleServer`/`SingleWorker` flows) and for spawning multiple interactive shells to simulate concurrent users.

## Requirements

- Python 3.11+
- Access to a running Huxley server (host + port)

## Usage

```bash
python client.py <host> <port> [--timeout 5.0]
```

Example:

```bash
python client.py localhost 8080
```

Once connected, type commands prefixed with `/`:

- `/register <username> <password>` — create a new account
- `/login <username> <password>` — authenticate and store the session
- `/send <recipient> <message>` — send a message (requires login)
- `/logout` — terminate the current session
- `/recv [seconds]` — block until the server pushes at least one frame (handy for incoming messages)
- `/whoami` — display the logged-in user (if any)
- `/quit` — exit the client

Tips:

1. Open two terminals, run the client in both, and log in as different users to exchange `/send` commands.
2. Use `/recv 10` in one terminal to wait up to 10 seconds for inbound frames.
3. Any unsolicited server frames (e.g., `INCOMING_MESSAGE`) are printed automatically whenever you hit Enter; `/recv` forces a blocking wait if you expect a notification while idle.

## Implementation Notes

- Transport: classic `[4-byte NBO length][JSON payload]` framing.
- Architecture: `ProtocolClient` handles sockets/framing, `MessageClient` exposes high-level operations, and `CliApp` parses slash commands from stdin.
- Single-threaded by design; `select()` drains pending notifications without background workers.
