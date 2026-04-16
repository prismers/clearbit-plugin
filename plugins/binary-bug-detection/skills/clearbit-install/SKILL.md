---
name: clearbit-install
description: >-
  Install and verify the ClearBit MCP service in a coding agent client. Use whenever the user
  wants to set up or connect to ClearBit, even if they don't say "MCP" or "install". Trigger
  on: "set up ClearBit", "install ClearBit", "I have a ClearBit server running at...",
  "connect to ClearBit at <url>", "configure ClearBit MCP", "I want to start analyzing
  binaries with ClearBit". Also use this when binary-bug-detection fails or reports that
  ClearBit is not configured.
---

# ClearBit Client Setup

Use this skill to install ClearBit into a client that talks to an already running ClearBit server.

Do not use this skill for server deployment or LLVM/toolchain setup.

## Inputs

- Client: default to Claude Code unless the user says Gemini or Codex
- Base URL: for example `http://host:3664`
- User email for `X-User-Email` in MCP config
- Optional ELF path for upload verification

If one of these is missing and cannot be inferred safely, ask for it before editing client config.

## Contract

- MCP URL: `<base_url>/mcp`
- Upload URL: `<base_url>/binaries`
- The MCP config carries fixed `X-API-Key` and `X-User-Email` headers
- Send `X-API-Key` on upload requests

If the user gives a URL ending in `/mcp`, strip that suffix before building the upload URL.

If the server is not exposed to the public internet, tunnel it first and use `localhost` as the client host. For example:

```bash
ssh -L 3664:localhost:3664 <server-host>
```

Then use `http://localhost:3664` as the base URL in client config.

## Workflow

1. Install the MCP service in the client.

Prefer the client's native install command. If none exists, edit the client's MCP config directly.

2. Generate an API key UUID.

Do not require the user to provide one. Generate it locally with:

```bash
python -c "import uuid; print(uuid.uuid4())"
```

Store the generated value in `CLEARBIT_API_KEY` or write it directly into the client MCP config.

3. Add the MCP config entry.

Use this JSON shape when the client supports HTTP MCP config:

```json
{
  "mcpServers": {
    "clearbit": {
      "type": "http",
      "url": "${CLEARBIT_BASE_URL:-http://localhost:3664}/mcp",
      "headers": {
        "X-API-Key": "${CLEARBIT_API_KEY}",
        "X-User-Email": "${CLEARBIT_USER_EMAIL}"
      }
    }
  }
}
```

4. Verify the installation.

Check in this order:
- `curl <base_url>/`
- MCP can read `ir://overview`
- if a binary path is available, upload it:

```bash
curl -X POST <base_url>/binaries \
  -H "X-API-Key: <uuid-token>" \
  -F file=@/path/to/binary
```

- poll `check_binary_status(binary_id)` until ready

5. Restart the client.

Remind to restart Claude Code (or whichever client was configured) so the new MCP server is loaded. Configuration changes are not picked up by a running session.

6. Report the result.

State whether installation succeeded, where config changed, which headers were configured, and whether verification passed.

## Troubleshooting

- **Cannot connect to server** — verify the base URL and port are correct; if the server is behind a firewall, set up an SSH tunnel (`ssh -L 3664:localhost:3664 <server-host>`) and use `http://localhost:3664`
- **MCP tools not appearing after restart** — confirm the config was written to the correct client config file and that the client was fully restarted
- **Upload returns 401** — check that `X-API-Key` in the upload request matches the value in the MCP config headers

# Related External Plugins

ClearBit returns IR node indices rather than assembly or source code directly. To map IR back to source or assembly for deeper manual review, consider:

- https://github.com/HexRaysSA/ida-claude-plugins/ with the `ida-domain-scripting` skill


