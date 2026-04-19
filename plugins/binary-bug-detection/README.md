# binary-bug-detection

A Claude Code plugin that finds security vulnerabilities in ELF binaries using ClearBit IR analysis. Provides four skills and five agents covering taint-style bugs, cross-binary IoT firmware analysis, and compiler-introduced security bugs.

## Skills

| Skill | What it finds | When to use |
|---|---|---|
| `taint-bug-detection` | Buffer overflows, command injection, use-after-free | Standalone binaries — desktop software, servers, CTF |
| `iot-bug-detection` | Cross-binary taint chains through IPC bridges (NVRAM, /tmp, sockets) | IoT / router firmware with multiple cooperating processes |
| `cisb-bug-detection` | Compiler-introduced bugs — dead store elimination of secret scrubs, null/overflow check removal, reordering, uninitialized padding leaks, Spectre gadgets | Any binary where the compiler may have optimized away security-critical code |
| `write-custom-query` | Custom ClearBit IR queries | Targeted follow-up analysis at any stage |

## Agents

| Agent | Role |
|---|---|
| `plan-agent` | Scans a binary for taint vulnerability candidates and CISB candidates; produces Analysis Plans |
| `analyze-agent` | Traces taint paths or confirms CISB structural patterns; produces Bug Reports with confidence scores |
| `validate-agent` | Verifies exploitability of a Bug Report and produces a PoC trigger script |
| `iot-plan-agent` | Per-binary IoT plan generation with bridge point writer/reader detection |
| `iot-bridge-agent` | Stitches per-binary bridge point records into high-order cross-binary taint chains |

## Prerequisites

- A running ClearBit server with MCP configured in the client

## Installation

```
/plugin install binary-bug-detection@clearbit-plugins
```

## Usage

```
Detect command injection vulnerabilities in ./httpd
Find compiler-introduced security bugs in ./openssl
Analyze this router firmware (httpd, nvramd, rc) for taint vulnerabilities
```

Each skill orchestrates the Plan → Analyze → Validate agent workflow and reports confirmed vulnerabilities with PoC scripts.
