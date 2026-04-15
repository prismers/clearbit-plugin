# ClearBit Plugin Marketplace

Official Claude Code plugin marketplace for the ClearBit binary analysis service.

## Add this marketplace

```
/plugin marketplace add prismers/clearbit-plugin
```

## Available plugins

| Plugin | Description |
|--------|-------------|
| `clearbit-install` | Install and verify the ClearBit MCP service in a coding agent client |

## Install a plugin

```
/plugin install clearbit-install@clearbit-plugins
```

## What is ClearBit?

ClearBit is a binary analysis service that exposes an MCP server. It accepts ELF binaries, lifts them to LLVM IR, and makes the IR available to coding agents via the MCP protocol. Agents can read `ir://overview` and related resources to reason about binary code directly.

## Plugin overview

### clearbit-install

Guides the agent through connecting a client (Claude Code, Gemini, or Codex) to an already-running ClearBit server:

- Generates a UUID API key
- Writes the MCP config entry with `X-API-Key` and `X-User-Email` headers
- Verifies the connection by curling the server and reading `ir://overview`
- Optionally uploads an ELF binary and polls for analysis completion
