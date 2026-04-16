---
name: binary-bug-detection
description: >-
  Detect taint-style vulnerabilities in an ELF binary using ClearBit IR analysis — buffer
  overflow, command injection, use-after-free, and any taint-reachable sink. Use this skill
  whenever the user wants to find security bugs or vulnerabilities in a compiled binary, even
  if they don't say "taint" or "ClearBit". Trigger on: "analyze this binary for bugs", "check
  if ./httpd is vulnerable", "find CVEs in this ELF", "security audit this binary", "is there
  a buffer overflow in this program", "pentest this binary", "scan this for vulnerabilities",
  or just a path to an ELF with a request to check it. Orchestrates Plan, Analyze, and
  Validate agents to produce confirmed bugs with PoC scripts.
---

# Binary Bug Detection

Use this skill to find taint-style vulnerabilities in an ELF binary.


## Inputs

- `binary` — ELF path or binary file reference (required)
- `bug_type` — e.g. `buffer-overflow`, `command-injection`, `use-after-free` (default: all taint-reachable sinks)
- `scope` — `source-scope` (start from source, find reachable sinks) or `sink-scope` (start from sink, find reachable sources); default: `sink-scope`


## Key Data Structures

- **Analysis Plan** `p = (loc, dir, targets)` — starting Binary Location as an `ir://` URI, tracking direction (`forward` or `backward`), target source/sink functions to track
- **Bug Report** `r = (src, snk, PC, α, desc)` — taint source instruction, sink instruction, path condition, confidence α ∈ [0,1], natural-language description
- **Binary Location** — MCP resource URI `ir://<binary_id>/<function>/<basicblock>/<instruction>`. Coarser granularity uses partial paths: `ir://<binary_id>/<function>` for function-level, `ir://<binary_id>` for binary-level.

## Workflow

Orchestrate three sub-agents in depth-first order — validate each report before advancing to the next plan.

### Step 1 — Upload binary

Upload input `binary` to ClearBit using the upload base URL (`http://localhost:3664`) and obtain `binary_id`.
Poll until status is `ready` before proceeding.

### Step 2 — Generate plans

Invoke `@agent-plan-agent` with `(binary_id, bug_type, scope)`.

Returns: Analysis Plan list `[p, ...]`

### Step 3 — Analyze each plan

For each plan `p`, invoke `@agent-analyze-agent` with `p`.

Returns: Bug Report list `[r, ...]` with confidence scores

### Step 4 — Validate each report

For each report `r` where `r.α ≥ 0.8`, invoke `@agent-validate-agent` with `r`.

Returns: `{ exploitable, script, desc }`

Discard reports where `exploitable` is false.

## Output

Report a table of confirmed vulnerabilities:

| Binary Location | Bug Class | Source → Sink | Confidence | PoC |
|---|---|---|---|---|

## Companion Skills

- `write-custom-query` — write and run custom ClearBit queries against LLVM IR during any analysis stage
