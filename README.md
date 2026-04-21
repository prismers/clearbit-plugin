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

## Install into Codex or Gemini

From this repo checkout:

```bash
./install.sh codex
./install.sh gemini
```

The installer symlinks skills and agents from every plugin under `plugins/`
into the selected client's user-wide config and registers each plugin's MCP
servers with the client's `mcp add` command.

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

### `binary-bug-detection`

Multi-skill plugin for finding security vulnerabilities in ELF binaries via ClearBit IR analysis. Includes client setup and a multi-agent (Plan → Analyze → Validate) bug-finding workflow.

| Skill | What it finds | When to use |
|---|---|---|
| `taint-bug-detection` | Buffer overflows, command injection, use-after-free | Standalone binaries — desktop software, servers, CTF |
| `iot-bug-detection` | Cross-binary taint chains through IPC bridges (NVRAM, /tmp, sockets) | IoT / router firmware with multiple cooperating processes |
| `cisb-bug-detection` | Compiler-introduced bugs — dead store elimination of secret scrubs, null/overflow check removal, reordering, uninitialized padding leaks, Spectre gadgets | Any binary where the compiler may have optimized away security-critical code |
| `write-custom-query` | Custom ClearBit IR queries | Targeted follow-up analysis at any stage |

## What is ClearBit?

ClearBit is an MCP service for binary analysis. It lifts ELF binaries to LLVM IR so coding agents can inspect and reason about native code.
