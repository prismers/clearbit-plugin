# binary-bug-detection

A Claude Code plugin that detects taint-style vulnerabilities in ELF binaries using ClearBit IR analysis.

## What it does

Provides one skill:

- **`binary-bug-detection`** — multi-agent taint analysis that orchestrates Plan, Analyze, and Validate sub-agents to find and verify bugs such as buffer overflows and command injection.

## Prerequisites

- A running ClearBit server with MCP configured in the client

## Installation

```
/plugin install binary-bug-detection@clearbit-plugins
```

## Usage

```
Detect command injection vulnerabilities in ./httpd using ClearBit at http://localhost:3664
```

The agent will upload the binary, orchestrate the Plan → Analyze → Validate workflow, and report confirmed vulnerabilities with PoC scripts.
