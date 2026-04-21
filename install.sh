#!/usr/bin/env bash

set -euo pipefail

usage() {
  cat <<'EOF'
Usage: ./install.sh [codex|gemini]

Installs all plugin assets found under ./plugins into the selected client's
user-wide configuration:
- skills are symlinked into the user's skills directory
- agents are symlinked into the user's agents directory
- MCP servers from each plugin's .mcp.json are registered with the client CLI
EOF
}

log() {
  printf '[install] %s\n' "$*"
}

die() {
  printf 'Error: %s\n' "$*" >&2
  exit 1
}

backup_existing() {
  local path="$1"
  local backup

  if [[ ! -e "$path" && ! -L "$path" ]]; then
    return
  fi

  if [[ -L "$path" ]]; then
    rm -f "$path"
    return
  fi

  backup="${path}.bak.$(date +%Y%m%d%H%M%S)"
  mv "$path" "$backup"
  log "Backed up existing path: $path -> $backup"
}

link_entry() {
  local src="$1"
  local dst="$2"

  backup_existing "$dst"
  ln -s "$src" "$dst"
  log "Linked $dst -> $src"
}

require_command() {
  command -v "$1" >/dev/null 2>&1 || die "Required command not found: $1"
}

install_symlinks() {
  local src_dir="$1"
  local dst_dir="$2"
  local entry

  mkdir -p "$dst_dir"

  for entry in "$src_dir"/*; do
    [[ -e "$entry" ]] || continue
    link_entry "$entry" "$dst_dir/$(basename "$entry")"
  done
}

install_codex_mcp_server() {
  local name="$1"
  local config_json="$2"
  local type url command bearer_env header_count
  local -a cmd env_args args

  type="$(jq -r '.type // (if has("url") then "http" else "stdio" end)' <<<"$config_json")"
  url="$(jq -r '.url // empty' <<<"$config_json")"
  command="$(jq -r '.command // empty' <<<"$config_json")"

  codex mcp remove "$name" >/dev/null 2>&1 || true

  if [[ -n "$url" ]]; then
    header_count="$(jq -r '.headers // {} | length' <<<"$config_json")"
    cmd=(codex mcp add "$name" --url "$url")

    if [[ "$header_count" -gt 0 ]]; then
      if [[ "$header_count" -ne 1 ]]; then
        die "Codex only supports bearer-token HTTP auth; unsupported headers for MCP server: $name"
      fi

      if [[ "$(jq -r '.headers | keys[0]' <<<"$config_json")" != "Authorization" ]]; then
        die "Codex only supports Authorization bearer headers for MCP server: $name"
      fi

      if [[ "$(jq -r '.headers.Authorization' <<<"$config_json")" =~ ^Bearer\ \$\{([A-Za-z_][A-Za-z0-9_]*)\}$ ]]; then
        bearer_env="${BASH_REMATCH[1]}"
        cmd+=(--bearer-token-env-var "$bearer_env")
      else
        die "Could not translate Authorization header to codex mcp add --bearer-token-env-var for server: $name"
      fi
    fi

    "${cmd[@]}"
  else
    [[ -n "$command" ]] || die "Missing MCP command for server: $name"

    env_args=()
    while IFS=$'\t' read -r key value; do
      [[ -n "$key" ]] || continue
      env_args+=(--env "$key=$value")
    done < <(jq -r '.env // {} | to_entries[]? | [.key, (.value | tostring)] | @tsv' <<<"$config_json")

    args=()
    while IFS= read -r value; do
      args+=("$value")
    done < <(jq -r '.args // [] | .[]' <<<"$config_json")

    codex mcp add "$name" "${env_args[@]}" -- "$command" "${args[@]}"
  fi

  log "Registered MCP server for Codex: $name"
}

install_gemini_mcp_server() {
  local name="$1"
  local config_json="$2"
  local type url command
  local -a cmd header_args env_args args

  type="$(jq -r '.type // (if has("url") then "http" else "stdio" end)' <<<"$config_json")"
  url="$(jq -r '.url // empty' <<<"$config_json")"
  command="$(jq -r '.command // empty' <<<"$config_json")"

  gemini mcp remove "$name" >/dev/null 2>&1 || true

  if [[ -n "$url" ]]; then
    header_args=()
    while IFS= read -r header; do
      [[ -n "$header" ]] || continue
      header_args+=(-H "$header")
    done < <(jq -r '.headers // {} | to_entries[]? | "\(.key): \(.value)"' <<<"$config_json")

    cmd=(gemini mcp add --scope user --transport "$type")
    cmd+=("${header_args[@]}")
    cmd+=("$name" "$url")
    "${cmd[@]}"
  else
    [[ -n "$command" ]] || die "Missing MCP command for server: $name"

    env_args=()
    while IFS=$'\t' read -r key value; do
      [[ -n "$key" ]] || continue
      env_args+=(-e "$key=$value")
    done < <(jq -r '.env // {} | to_entries[]? | [.key, (.value | tostring)] | @tsv' <<<"$config_json")

    args=()
    while IFS= read -r value; do
      args+=("$value")
    done < <(jq -r '.args // [] | .[]' <<<"$config_json")

    cmd=(gemini mcp add --scope user --transport stdio)
    cmd+=("${env_args[@]}")
    cmd+=("$name" "$command")
    cmd+=("${args[@]}")
    "${cmd[@]}"
  fi

  log "Registered MCP server for Gemini: $name"
}

install_mcp_servers() {
  local client="$1"
  local mcp_file="$2"
  local plugin_name="$3"
  local name config_json

  [[ -f "$mcp_file" ]] || return

  while IFS=$'\t' read -r name config_json; do
    [[ -n "$name" ]] || continue
    if [[ "$client" == "codex" ]]; then
      install_codex_mcp_server "$name" "$config_json"
    else
      install_gemini_mcp_server "$name" "$config_json"
    fi
  done < <(jq -r '.mcpServers // {} | to_entries[]? | [.key, (.value | tojson)] | @tsv' "$mcp_file")

  log "Processed MCP config for plugin: $plugin_name"
}

install_plugin() {
  local client="$1"
  local plugin_dir="$2"
  local skill_dst="$3"
  local agent_dst="$4"
  local plugin_name skill_src agent_src mcp_file

  plugin_name="$(basename "$plugin_dir")"
  skill_src="$plugin_dir/skills"
  agent_src="$plugin_dir/agents"
  mcp_file="$plugin_dir/.mcp.json"

  log "Installing plugin: $plugin_name"

  if [[ -d "$skill_src" ]]; then
    install_symlinks "$skill_src" "$skill_dst"
  fi

  if [[ -d "$agent_src" ]]; then
    install_symlinks "$agent_src" "$agent_dst"
  fi

  install_mcp_servers "$client" "$mcp_file" "$plugin_name"
}

install_all_plugins() {
  local client="$1"
  local repo_root="$2"
  local skill_dst="$3"
  local agent_dst="$4"
  local plugin_dir found_any=0

  for plugin_dir in "$repo_root"/plugins/*; do
    [[ -d "$plugin_dir" ]] || continue
    found_any=1
    install_plugin "$client" "$plugin_dir" "$skill_dst" "$agent_dst"
  done

  [[ "$found_any" -eq 1 ]] || die "No plugins found under $repo_root/plugins"
}

main() {
  local client="${1:-}"
  local script_dir repo_root
  local skill_dst agent_dst

  if [[ -z "$client" || "${2:-}" != "" ]]; then
    usage
    exit 1
  fi

  case "$client" in
    codex|gemini)
      ;;
    -h|--help)
      usage
      exit 0
      ;;
    *)
      usage
      die "Unsupported client: $client"
      ;;
  esac

  script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
  repo_root="$script_dir"
  [[ -d "$repo_root/plugins" ]] || die "Missing plugins directory: $repo_root/plugins"

  if [[ "$client" == "codex" ]]; then
    require_command codex
    require_command jq
    skill_dst="${CODEX_HOME:-$HOME/.codex}/skills"
    agent_dst="${CODEX_HOME:-$HOME/.codex}/agents"
    install_all_plugins "$client" "$repo_root" "$skill_dst" "$agent_dst"
  else
    require_command gemini
    require_command jq
    skill_dst="${GEMINI_HOME:-$HOME/.gemini}/skills"
    agent_dst="${GEMINI_HOME:-$HOME/.gemini}/agents"
    install_all_plugins "$client" "$repo_root" "$skill_dst" "$agent_dst"
  fi

  log "Installation complete for $client"
  log "Restart the client if it is already running."
}

main "$@"
