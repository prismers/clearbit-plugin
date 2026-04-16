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

For the specification and available APIs, see the [custom-query-guide](ir://custom-query-guide) from the mcp resource. We show some examples covering common use cases below.

## Example 1 — Find all calls to `foo` within `bar`

[find-calls-to-foo-within-bar.cpp](references/find-calls-to-foo-within-bar.cpp)

## Example 2 — Find all users of an instruction

[find-users-of-instruction.cpp](references/find-users-of-instruction.cpp)

## Example 3 — Find all definitions reaching a given instruction

[find-definitions-reaching-instruction.cpp](references/find-definitions-reaching-instruction.cpp)

## Example 4 — Check if two pointers are aliasing

[check-base-pointer-aliases.cpp](references/check-base-pointer-aliases.cpp)
