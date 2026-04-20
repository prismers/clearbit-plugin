---
name: iot-bug-detection
description: >-
  Detect taint-style vulnerabilities across multiple binaries in Linux-based IoT firmware
  using ClearBit IR analysis and cross-binary bridge point stitching. Use this skill for
  IoT firmware, router firmware, embedded Linux images, or any multi-binary system where
  taint flows between cooperating processes via IPC (NVRAM, /tmp files, env vars, Unix
  sockets). Trigger on: "analyze this firmware for bugs", "check this router firmware for
  vulnerabilities", "find command injections in this IoT device", "security audit this
  firmware image", "look for buffer overflows across these binaries", "scan httpd and nvramd
  for taint paths", or whenever a user provides multiple ELF binaries from a firmware
  filesystem and asks for a security review. Handles high-order taint chains
  (source → binary A → IPC bridge → binary B → sink) that single-binary tools miss.
---

# IoT Firmware Bug Detection

Use this skill to find taint-style vulnerabilities in **Linux-based IoT firmware** — router
firmware, embedded Linux images, or any system made of multiple cooperating processes linked
by IPC channels.

## Inputs

- `binaries` — List of ELF paths from the firmware filesystem (required). Common targets:
  `httpd`, `udhcpd`, `miniupnpd`, `nvramd`, `telnetd`, `ftpd`, CGI binaries under `www/cgi-bin/`
- `bug_type` — `buffer-overflow`, `command-injection`, `use-after-free`, or `all` (default: `all`)
- `order` — Maximum bridge hops to trace: `1` (single-binary only), `2` (one bridge), `3+` (default: `2`)


## Key Concepts

### Bridge Points
A **bridge point** is an IPC channel where taint crosses a process boundary:

| Bridge Type       | Writer API                        | Reader API                          |
|-------------------|-----------------------------------|-------------------------------------|
| Temp file         | `fopen("/tmp/...","w")`, `write`  | `fopen("/tmp/...","r")`, `read`     |
| NVRAM             | `nvram_set(key, val)`             | `nvram_get(key)`                    |
| Environment var   | `setenv(key, val)`, `putenv`      | `getenv(key)`                       |
| Unix socket       | `send`/`write` on bound socket    | `recv`/`read` on connected socket   |
| Named pipe (FIFO) | `write` on FIFO fd                | `read` on FIFO fd                   |
| Shared memory     | `shm_open`+`mmap` write           | `shm_open`+`mmap` read              |

Two binaries are **bridge-matched** when one writes and the other reads the same key or path.

### IoT Taint Sources
Prioritize these entry points for attacker-controlled data:
- `nvram_get()` — any call whose key is user-reachable via HTTP/NVRAM write
- `getenv("HTTP_*")`, `getenv("QUERY_STRING")`, `getenv("REQUEST_METHOD")` — CGI HTTP input
- `fgets`/`read` on stdin in CGI processes
- `recv`/`recvfrom` on network sockets (DHCP option fields, UPnP SSDP, TR-069)
- Vendor-specific: `acos_get_config()`, `bcm_nvram_get()`, `config_get()`, `ucix_get_option()`

### IoT Sink Priorities
- Command execution: `system()`, `popen()`, `execve()`, `execl()`, `execvp()`
- Memory corruption: `strcpy()`, `strcat()`, `sprintf()`, `vsprintf()`, `gets()`, `memcpy()`


## Workflow

Orchestrate agents across all binaries, then stitch results through bridge points.

### Step 1 — Upload all binaries

For each binary in `binaries`, upload to ClearBit (`http://localhost:3664`) with `$CLARBIT_API_KEY` and collect `binary_id`.
Poll each with `check_binary_status` until status is `ready`. Build a map: `{ filename → binary_id }`.

### Step 2 — Per-binary taint plans

Invoke `@agent-iot-plan-agent` for each binary with `(binary_id, filename, bug_type)`.

Each call returns an Analysis Plan list focused on IoT sources/sinks and bridge point writers/readers.

### Step 3 — Per-binary taint analysis

For each plan `p` from Step 2, invoke `@agent-analyze-agent` with `p`.

Collect all Bug Reports `r` and all **Bridge Point Records** `b`:
- A Bug Report is a complete source→sink path within one binary (confidence α ∈ [0,1])
- A Bridge Point Record is a partial path that reaches a bridge writer or starts from a bridge reader but does not yet reach a dangerous sink

### Step 4 — Cross-binary stitching

Invoke `@agent-iot-bridge-agent` with:
- All Bridge Point Records from Step 3
- The full `{ filename → binary_id }` map

The bridge agent matches writer records to reader records by key/path identity,
then constructs **High-Order Taint Chains**: sequences of (binary, partial-path) pairs
connected through matched bridge points, ending at a dangerous sink.

Returns: High-Order Bug Report list with the full cross-binary chain.

### Step 5 — Validate high-confidence findings

For each report `r` (single-binary or high-order) where `r.α ≥ 0.8`,
invoke `@agent-validate-agent` with `r`.

Discard reports where `exploitable` is false.

## Output

Report a table of confirmed vulnerabilities, clearly distinguishing single-binary from cross-binary chains:

| Chain | Bug Class | Full Taint Path | Order | Confidence | PoC |
|---|---|---|---|---|---|
| httpd → nvramd | command-injection | `getenv("HTTP_CMD")` → `nvram_set("cmd",…)` → `nvram_get("cmd")` → `system()` | 2nd | 0.93 | script |

**Order** = number of binaries in the chain (1 = single-binary, 2 = one bridge hop, etc.)

After the table, list any bridge points that carried taint but did not reach a confirmed
exploitable sink — these are useful for manual follow-up.

## Companion Skills

- `write-custom-query` — write custom ClearBit IR queries during any stage
