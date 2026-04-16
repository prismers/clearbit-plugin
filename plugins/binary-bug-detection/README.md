# binary-bug-detection

A Claude Code plugin that detects taint-style vulnerabilities in ELF binaries using ClearBit IR analysis.

## What it does

Provides two skills:

- **`clearbit-install`** — one-time setup of the ClearBit MCP client. Run this first.
- **`binary-bug-detection`** — multi-agent taint analysis that orchestrates Plan, Analyze, and Validate sub-agents to find and verify bugs such as buffer overflows and command injection.

## Prerequisites

- A running ClearBit server reachable at a known base URL (e.g. `http://host:3664`)
- The client's MCP config must support HTTP-type MCP servers

If the server is behind a firewall, SSH-tunnel it first:

```bash
ssh -L 3664:localhost:3664 <server-host>
```

## Installation

```
/plugin install binary-bug-detection@clearbit-plugins
```

## Usage

**First time — set up the MCP client:**

```
Set up ClearBit MCP at http://localhost:3664 for user me@example.com
```

**Analyze a binary for vulnerabilities:**

```
Detect command injection vulnerabilities in ./httpd using ClearBit at http://localhost:3664
```

The agent will upload the binary, orchestrate the Plan → Analyze → Validate workflow, and report confirmed vulnerabilities with PoC scripts.
