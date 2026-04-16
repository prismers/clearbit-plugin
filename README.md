# ClearBit Plugin Marketplace

Plugin marketplace for the ClearBit binary analysis service.

## Add the marketplace

```text
/plugin marketplace add prismers/clearbit-plugin
```

## Install the plugin

```text
/plugin install binary-bug-detection@clearbit-plugins
```

## Available plugin

- `binary-bug-detection`: Detect taint-style vulnerabilities (buffer overflow, command injection) in ELF binaries via ClearBit IR analysis. Includes client setup and multi-agent bug-finding workflow.

## What is ClearBit?

ClearBit is an MCP service for binary analysis. It lifts ELF binaries to LLVM IR so coding agents can inspect and reason about native code.
