---
name: write-custom-query
description: write and run custom ClearBit queries against LLVM IR during any analysis stage. Use when you want to inspect or analyze the binary in a way not covered by existing skills, e.g. custom taint rules, specific IR patterns, or ad-hoc queries during bug analysis.
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
