---
name: write-custom-query
description: >-
  Write and run custom ClearBit queries against LLVM IR to answer specific questions about a
  binary's structure or data flow. Use whenever you need to go beyond standard taint analysis.
  Trigger on: "find all calls to X in function Y", "does this pointer alias", "trace how this
  value flows through IR", "check what reads or writes this memory", "write a custom query
  for...", or whenever binary-bug-detection needs to locate a specific IR pattern mid-analysis.
  Also call this proactively as a helper during any analysis stage.
---

# Write Custom Query

We can implement advanced pattern-based program analysis as reusable ClearBit queries, e.g. "find all calls to `system` with non-constant string arguments".

Always read `ir://custom-query-guide` first — it is the authoritative source for required signatures, available helpers, and active toolchain constraints.

## Workflow

1. Read `ir://overview`.
2. Inspect the relevant `ir://<module>/...` resources before writing code.
3. Copy exact module names, function names, block labels, and instruction indices from those resources.
4. Generate only the `query_body` fragment expected by ClearBit.

## Authoring Guidance

- The request URI is pre-resolved. Use `Q.module`, `Q.function`, `Q.block`, and `Q.instruction` directly — no manual resolution needed unless the query intentionally accepts a different scope.
- Prefer response keys `found`, `uri`, `kind`, and `summary`, then add query-specific fields.
- On failure, return `{"found": false, "uri": "...", "error": "..."}` and `return false`.
- Use `IRUri::buildURI(...)` when returning URIs derived from IR pointers.
- Use `IRUri::buildInstIdx(...)` when you need a stable instruction index.
- Call `appendAsmLoc(...)` and `appendSrcLoc(...)` opportunistically; tolerate missing address or debug info.
- Keep result payloads focused. Do not dump entire modules or functions when a summary or filtered list is enough.
- Declare any LLVM analyses you need in `getQueryAnalysisUsage(AU)` before accessing them via `getModuleAnalysis<T>()` or `getFunctionAnalysis<T>(...)`. See [llvm36-analyses.md](references/llvm36-analyses.md) for pass names, retrieval patterns, and key methods for `AliasAnalysis`, `MemoryDependenceAnalysis`, `DominatorTree`, `PostDominatorTree`, `LoopInfo`, `ScalarEvolution`, `DataLayout`, and `CallGraph`.

## Example 1 — Find all calls to `foo` within `bar`

[find-calls-to-foo-within-bar.cpp](references/find-calls-to-foo-within-bar.cpp)

## Example 2 — Find all users of an instruction

[find-users-of-instruction.cpp](references/find-users-of-instruction.cpp)

## Example 3 — Find the memory dependency of a load/store (`MemoryDependenceAnalysis`)

[find-definitions-reaching-instruction.cpp](references/find-definitions-reaching-instruction.cpp)

## Example 4 — Find aliasing accesses to a pointer (`AliasAnalysis`)

[check-base-pointer-aliases.cpp](references/check-base-pointer-aliases.cpp)

## Example 5 — Transitive callee reachability via the call graph (`CallGraphWrapperPass`)

[callgraph-reachability.cpp](references/callgraph-reachability.cpp)
