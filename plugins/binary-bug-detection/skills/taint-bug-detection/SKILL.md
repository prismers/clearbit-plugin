---
name: taint-bug-detection
description: >-
  Detect taint-style vulnerabilities in a single ELF binary using ClearBit IR analysis —
  buffer overflow, command injection, use-after-free, and any taint-reachable sink. Use this
  skill for standalone binaries from desktop software, servers, CTF challenges, or daemons.
  Trigger on: "analyze this binary for bugs", "check if ./server is vulnerable", "find CVEs
  in this ELF", "security audit this binary", "is there a buffer overflow in this program",
  "pentest this binary", "scan this for vulnerabilities", or just a path to an ELF with a
  request to check it. Orchestrates Plan, Analyze, and Validate agents to produce confirmed
  bugs with PoC scripts.
---

# Taint Bug Detection

Use this skill to find taint-style vulnerabilities in a **single ELF binary** — desktop
software, server daemons, CTF binaries, or any standalone compiled program.


## Inputs

- `binary` — ELF path or binary file reference (required)
- `bug_type` — e.g. `buffer-overflow`, `command-injection`, `use-after-free` (default: all taint-reachable sinks)
- `scope` — `source-scope` (start from source, find reachable sinks) or `sink-scope` (start from sink, find reachable sources); default: `sink-scope`


## File Layout

Agents write results to disk for persistence; the orchestrator consumes their in-conversation returns. `<binary>` is the base filename of the uploaded ELF (e.g. `httpd`).

- `<binary>-plan.json` ← plan-agent: `plans[]` with `plan_id`, `loc` (ir:// URI), `dir`, `targets`
- `<binary>-analysis/<plan-id>.json` ← analyze-agent: `reports[]` with `report_id`, `src`, `snk`, `PC`, `alpha`, `desc`
- `<binary>-validate/<report-id>.json` ← validate-agent: `report_id`, `exploitable`, `script`, `desc`

## Workflow

Agents write their results to disk (for persistence and resumption) and return the same content in-conversation. The orchestrator uses the in-conversation return to maintain a full picture — plans, reports, and PoC results — without re-reading files. Files are consulted only when resuming an interrupted session.

Proceed in depth-first order: fully validate all reports for one plan before moving to the next. This avoids spawning validate agents on plans that may yield nothing.

### Step 1 — Upload binary

Upload input `binary` to ClearBit using the upload base URL (`http://localhost:3664`) and obtain `binary_id`.
Poll until status is `ready` before proceeding.

Derive `<binary>` = base filename without directory path (e.g. `httpd`).

### Step 2 — Generate plans

Invoke `@agent-plan-agent` with `(binary_id, binary, bug_type, scope)`.

The agent returns the full plans list. If the list is empty, report that no candidates were found and stop.

### Step 3 — Analyze & Validate (depth-first)

For each plan:
a. Invoke `@agent-analyze-agent` with `(binary, plan_id)`. It returns reports with `alpha` values.
b. For each report where `alpha >= 0.8`, invoke `@agent-validate-agent` with `(binary, plan_id, report_id)`.
c. Discard reports where `exploitable` is false, then proceed to the next plan.

**Resuming an interrupted session**: check which files already exist under `<binary>-plan.json`, `<binary>-analysis/`, and `<binary>-validate/` to determine how far the previous run got, then read those files to reconstruct context and continue.

## Output

Report a table of confirmed, exploitable vulnerabilities (plan ID, bug class, source → sink, confidence, PoC trigger). Link each row to `<binary>-validate/<report-id>.json` for full detail. If none found, say so and list dismissed high-confidence reports with reasons.

## Companion Skills

- `iot-bug-detection` — multi-binary cross-process taint analysis for IoT firmware
- `write-custom-query` — write custom ClearBit IR queries during any analysis stage
