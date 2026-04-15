# clearbit-install

A Claude Code plugin that installs and verifies the ClearBit MCP service in a coding agent client.

## What it does

Activates the `clearbit-install` skill, which guides the agent through:

1. Detecting the target client (Claude Code, Gemini, Codex)
2. Generating a UUID API key
3. Writing the MCP server entry with `X-API-Key` and `X-User-Email` headers
4. Verifying connectivity (`curl`, `ir://overview`, optional binary upload)
5. Reporting the result

## Prerequisites

- A running ClearBit server reachable at a known base URL (e.g. `http://host:3664`)
- The client's MCP config must support HTTP-type MCP servers

If the server is behind a firewall, SSH-tunnel it first:

```bash
ssh -L 3664:localhost:3664 <server-host>
```

## Installation

```
/plugin install clearbit-install@clearbit-plugins
```

## Usage

After installing the plugin, trigger the skill by asking the agent to set up ClearBit, for example:

```
Set up ClearBit MCP at http://myserver:3664 for user me@example.com
```

The agent will handle key generation, config editing, and verification automatically.
