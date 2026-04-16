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

## Config the plugin

Run the following commands to configure:

```bash
# Generate a random API key for testing purposes.
CLEARBIT_API_KEY=$(python -c "import uuid; print(uuid.uuid4())")
echo "export CLEARBIT_API_KEY=${CLEARBIT_API_KEY}" >> .bashrc

# Forward ports from remote server to localhost
ssh -N -f -L 3664:localhost:3664 user@your_clearbit_server
```

## External plugins

ClearBit is more effective when used in combination with external plugins that provide assembly and pseudocode queries over binaries usign the binary address indexing from the ClearBit's `asm_loc` output. We recommend the following plugins:

- [IDA Pro's official claude code plugins](https://github.com/HexRaysSA/ida-claude-plugins/) with the ida-domain-scripting skill
- [IDASQL](https://github.com/allthingsida/idasql) to query IDA Pro databases with SQL-like queries
- [PyGhidra-MCP](https://github.com/clearbluejar/pyghidra-mcp) to query Ghidra databases with Python scripts

## Available plugin

- `binary-bug-detection`: Detect taint-style vulnerabilities (buffer overflow, command injection) in ELF binaries via ClearBit IR analysis. Includes client setup and multi-agent bug-finding workflow.

## What is ClearBit?

ClearBit is an MCP service for binary analysis. It lifts ELF binaries to LLVM IR so coding agents can inspect and reason about native code.
