# Multi-Client Chat Server (C) — `select()`-based

A TCP chat server written in C that handles multiple simultaneous clients using `select()` for I/O multiplexing (no threads or forked child processes on the server side). Each connecting client is assigned a numeric ID, receives the recent chat history on join, and can broadcast messages to everyone else connected.

## Features

- **Multi-client** via `select()` I/O multiplexing — up to `MAX_CLIENTS` (10) concurrent connections
- **Auto-assigned client ID** — each client gets a unique numeric ID on connect
- **Chat history replay** — new clients receive up to the last `MAX_HISTORY` (50) messages on join
- **Broadcast messaging** — any message sent by a client is relayed to all other connected clients
- **Client-side split process** — the client `fork()`s into a reader child (prints incoming messages) and a writer parent (reads stdin, sends messages)

## Build

```bash
gcc -o server server.c
gcc -o client client.c
```

## Usage

Start the server:

```bash
./server <portno>
```

Then connect one or more clients:

```bash
./client <Server_IP> <portno>
```

On connect, the client:
1. Receives and prints its assigned numeric ID
2. Prompts for a display name
3. Enters chat mode — type a message and hit Enter to broadcast it to everyone else connected
4. Type `bye` to disconnect

## Protocol Overview

Communication is a simple newline-terminated text protocol over TCP:

| Event                     | Direction         | Payload                                              |
|----------------------------|--------------------|-------------------------------------------------------|
| Client connects            | Server → Client    | Assigned numeric ID (as text)                          |
| History replay             | Server → Client    | Each stored message + `\n`, sent one at a time         |
| Chat message                | Client → Server    | `"<Name>: <message>"`                                  |
| Broadcast                   | Server → Clients   | Original message text + `\n`, relayed to all sockets   |
| Disconnect ("bye")          | Client → Server    | `"Client [ID: <id>, Name: <name>] disconnected."`, then socket closed |

## Server Design Notes

- Uses a single `select()` loop over the listening socket plus up to `MAX_CLIENTS` client sockets — no threading, so all I/O handling happens sequentially per event.
- Chat history is kept in a fixed in-memory buffer (`MAX_HISTORY = 50` messages); once full, the oldest message is shifted out to make room for new ones.
- New clients receive the full available history immediately after their ID, with a small `usleep()` delay between each line.

## Known Limitations / Next Steps

Course project — worth being aware of these for a defense/presentation, and good candidates to fix or discuss:

- **Buffer overflow risk in `chat_history`:** `buffer` is read up to 1024 bytes and null-terminated at index `valread` (up to index 1024), but each `chat_history[i]` entry is only `1024` bytes wide — a max-length message can write one byte past the end of the array. Fix: size `chat_history` entries at `1025` bytes, or cap reads at `1023` bytes.
- **No message framing/delimiters.** IDs, history lines, and chat messages are sent as raw `write()`s relying on separate TCP segments arriving separately (aided by `usleep()` in a few places). On a fast connection (e.g. localhost) these can coalesce or split unpredictably, corrupting the assigned-ID read or interleaving history/messages. Needs proper framing (length-prefixed or strict `\n`-delimited reads with buffering).
- **No handling when `MAX_CLIENTS` is reached.** If 10 clients are already connected, a new `accept()` still succeeds but the resulting socket is never registered in `client_socket[]` or watched by `select()` — the connection hangs silently instead of being rejected or closed.
- **Dead code:** `getpeername()` is called on disconnect but its result is unused.
- **No authentication/authorization** beyond the auto-assigned ID — anyone who can reach the port can join and claim any display name.
- **No encryption** — not intended for use over untrusted networks.

## Project Structure

```
.
├── client.c   # TCP chat client (fork: reader child + writer parent)
├── server.c   # select()-based multi-client chat server
└── README.md
```

## Requirements

- POSIX-compliant system (Linux/macOS)
- GCC or any C compiler with standard socket headers (`sys/socket.h`, `netinet/in.h`, `sys/select.h`)
