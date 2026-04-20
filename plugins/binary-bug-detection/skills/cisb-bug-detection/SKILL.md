---
name: cisb-bug-detection
description: >-
  Detect compiler-introduced security bugs (CISB) in an ELF binary using ClearBit IR analysis.
  CISB are vulnerabilities created by correct compiler optimizations — dead store elimination
  removing secret scrubs, null-check elimination via no-UB assumptions, signed-overflow check
  removal, reordering of security-critical operations, uninitialized padding leaks, and
  Spectre-class speculative-execution hazards. Use this skill whenever the user asks to find
  CISB, compiler-introduced bugs, dead store elimination issues, sensitive data scrubbing
  problems, undefined behavior security issues, or asks "did the compiler introduce a bug here".
  Also trigger on: "check if memset was optimized away", "find DSE vulnerabilities", "look for
  null-check elimination", "scan for UB-related security bugs", or any mention of the CISB
  taxonomy (ISpec, OSpec, DSE, timing side-channel from compiler). Orchestrates Plan, Analyze,
  and Validate agents using the CISB taxonomy from the USENIX Security '23 paper.
---

# CISB Bug Detection

Use this skill to find **compiler-introduced security bugs** in an ELF binary — vulnerabilities
that exist in the optimized binary but not in the source code, produced by formally correct
compiler optimizations.

Read `references/cisb-patterns.md` before generating plans. It describes all six CISB classes
(ISpec1–3, OSpec1–3) with source-level patterns, binary confirmation criteria, and detection
priority.

## Inputs

- `binary` — ELF path (required)
- `cisb_class` — one or more of `ISpec1`, `ISpec2`, `ISpec3`, `OSpec1`, `OSpec2`, `OSpec3`,
  or `all` (default: `all`)
- `scope` — `source-scope` (start from CISB source, find insecure instructions/paths) or
  `sink-scope` (start from security-critical ops, find eliminated/reordered checks); default: `sink-scope`

## What Makes CISB Different from Taint Bugs

Taint bugs follow data flow: source → dangerous sink. CISB are structural: a security-relevant
operation is *absent*, *reordered*, or *replaced* by the compiler. The plan agent must therefore
look for **absence of expected IR** (a check that should be there but isn't), not just a path
from A to B. Keep this in mind when reviewing plans and reports.

Key IR signatures to look for:

| Class    | What to find in IR                                                      |
|----------|-------------------------------------------------------------------------|
| ISpec1-A | Branch/block present in unoptimized IR but absent in optimized IR       |
| ISpec1-B | `memset`/`bzero` call absent after function writing to a secret buffer  |
| ISpec2-A | IR instruction order differs from source-intended order (check after op)|
| ISpec3-C | Integer promotion: `u8`/`u16` arithmetic widened to `i32` before compare|
| ISpec3-D | `printf` replaced by `puts` — check call target in optimized IR         |
| OSpec1-B | Struct with padding bytes partially initialized then passed to send/copy |
| OSpec3-A | Switch table transformed to indirect branch over attacker-influenced index|

## File Layout

- `<binary>-cisb-plan.json` ← plan-agent output
- `<binary>-cisb-analysis/<plan-id>.json` ← analyze-agent output
- `<binary>-cisb-validate/<report-id>.json` ← validate-agent output

## Workflow

### Step 1 — Upload binary

Upload `binary` to ClearBit (`http://localhost:3664`) with `$CLARBIT_API_KEY` and obtain `binary_id`. Poll with `check_binary_status` until
status is `ready`. Derive `<binary>` = base filename (e.g. `httpd`).

### Step 2 — Generate CISB plans

Invoke `@agent-plan-agent` with `(binary_id, binary, cisb_class, scope)`.

Direct the plan agent to use `references/cisb-patterns.md` as its pattern guide. Plans should
target:
- Functions that handle secrets, authentication, or memory scrubbing (ISpec1-B, OSpec1)
- Functions with null checks, signed arithmetic checks, or bounds guards (ISpec1-A)
- Comparison functions used for authentication (OSpec2-A)
- Structs with padding sent across trust boundaries (OSpec1-B)
- Switch statements over attacker-controlled indices (OSpec3-A)

If the plans list is empty, report that no CISB candidates were found and stop.

### Step 3 — Analyze & Validate (depth-first)

For each plan:

a. Invoke `@agent-analyze-agent` with `(binary, plan_id)`. The agent returns reports with
   `alpha` confidence values and CISB class labels.

b. For each report where `alpha >= 0.8`, invoke `@agent-validate-agent` with
   `(binary, plan_id, report_id)`. The validator should confirm the bug is reachable
   and, where possible, produce a PoC or describe how to trigger it.

c. Discard reports where `exploitable` is false. Proceed to the next plan.

**Resuming an interrupted session:** check which files exist under `<binary>-cisb-plan.json`,
`<binary>-cisb-analysis/`, and `<binary>-cisb-validate/` to reconstruct prior progress.

## Output

Report a table of confirmed CISB vulnerabilities:

| Plan ID | CISB Class | Location | Description | Confidence | PoC |
|---------|------------|----------|-------------|------------|-----|
| P-001   | ISpec1-B   | `crypto_wipe()` | `memset` of key buffer eliminated by DSE | 0.94 | script |

After the table, note any high-confidence reports that were not confirmed exploitable, with
reasons (e.g., "check eliminated but path unreachable in practice").

## Companion Skills

- `write-custom-query` — write custom ClearBit IR queries for any CISB investigation
