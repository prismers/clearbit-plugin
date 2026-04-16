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

We require the following environment variables to run the plugin:

```bash
# Generate a random API key for testing purposes.
export CLEARBIT_API_KEY=$(python -c "import uuid; print(uuid.uuid4())")
# Set your email for ClearBit.
export CLEARBIT_USER_EMAIL=<your_email>
# Set your ClearBit API URL. Forward ports if server is not public.
ssh -N -f -L 3664:localhost:3664 user@your_clearbit_server
export CLEARBIT_API_URL=<your_clearbit_api_url>
```

## Available plugin

- `binary-bug-detection`: Detect taint-style vulnerabilities (buffer overflow, command injection) in ELF binaries via ClearBit IR analysis. Includes client setup and multi-agent bug-finding workflow.

## What is ClearBit?

ClearBit is an MCP service for binary analysis. It lifts ELF binaries to LLVM IR so coding agents can inspect and reason about native code.
