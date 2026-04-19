# CISB Pattern Reference

Based on: "Silent Bugs Matter: A Study of Compiler-Introduced Security Bugs" (USENIX Security '23)

This reference catalogs the six classes of CISB from the paper's taxonomy, with source-level patterns
to look for, what to confirm in the compiled binary, and false-positive mitigations.

---

## Taxonomy Overview

```
Root Cause               Insecure Behavior              Security Consequence
─────────────────────────────────────────────────────────────────────────────
ISpec (Implicit Spec)
  ISpec1                 Eliminate security-related code  Check / memop removal
  ISpec2                 Reorder order-sensitive code     Misordered security ops
  ISpec3                 Introduce insecure instructions  Invalid/insecure instr
OSpec (Orthogonal Spec)
  OSpec1                 Make sensitive data out of bound Data lifetime/space viol
  OSpec2                 Break timing guarantees          Timing side channel
  OSpec3                 Introduce insecure micro-arch    Bounds-check bypass (Spectre)
```

---

## ISpec1 — Elimination of Security-Related Code

**What happens:** Compiler infers that a safety check or a critical memory operation
is redundant and removes it. The inference is based on assumptions the compiler may make
(no UB, no signed overflow, no null dereference, no strict-alias violation, no data race).

### ISpec1-A: Security check elimination via No-UB assumption

**Source pattern to find (clang LSP / grep):**

1. **Null check after deref** — a pointer is dereferenced before being checked:
   ```c
   int c = *ptr;      // dereference — compiler may infer ptr != NULL
   if (ptr) { ... }  // compiler may eliminate this check
   ```
   Look for: dereference of a pointer in the same function body, followed by a null check of the same pointer.

2. **Signed-overflow check** — using signed arithmetic to detect overflow:
   ```c
   if (x + 10 < x) { ... }   // compiler assumes signed overflow doesn't happen → eliminates
   if ((int)(x + y) < 0) ...  // similar
   ```
   Look for: comparisons of a signed addition against one of its operands.

3. **Infinite-loop guard elimination**:
   ```c
   for (...; i + 1 > i; ...) { ... }   // signed overflow: compiler removes guard
   ```

4. **Bounds check with signed arithmetic** used to protect array indexing.

**Binary confirmation:** In the optimized binary, the IR for the guarding branch/basic-block
should be absent. The analyze-agent should confirm the check's target block is unreachable.

---

### ISpec1-B: Critical memory operation elimination

**Source pattern:**

```c
memset(secret_buf, 0, sizeof(secret_buf));  // scrub before returning
return;
// If secret_buf is never read again, DSE eliminates the memset
```
Or:
```c
memset(&key, 0, keylen);  // key goes out of scope — compiler eliminates via DSE
```

Also watch for:
- Initialization of a variable that is then overwritten before its first read (compiler removes the init).
- `memset` / `bzero` before a pointer goes out of scope.

**Correct mitigations developers use (your job is to find code that does NOT use them):**
- `memzero_explicit()`, `explicit_bzero()`, `SecureZeroMemory()`
- `volatile` on the buffer
- `__asm__ __volatile__("")` memory barrier after the memset

---

## ISpec2 — Reordering of Order-Sensitive Security Code

**What happens:** Compiler reorders two operations that must occur in a fixed sequence
for security reasons, breaking the sequencing invariant.

### ISpec2-A: Security check / dangerous-operation reordering

**Source pattern:**
```c
if (arg == 0) error();          // check — if ereport() is declared noreturn-like,
result = a / arg;               // division — compiler may hoist this before the check
```
Also:
```c
check_permission();
do_dangerous_op();
// If compiler decides do_dangerous_op() has no data dep on check_permission(), it may reorder.
```

Look for: a function-call-based check immediately before an arithmetic or memory operation
where the compiler can prove the operation is "safe" under its assumptions.

### ISpec2-B: Memory-operation ordering (init before use)

```c
struct S s;
do_something(&s);    // uses s — but initializer was reordered after this
memset(&s, 0, ...);  // compiler moves this earlier/later based on alias analysis
```

---

## ISpec3 — Introduction of Insecure Instructions

**What happens:** Compiler introduces an instruction that is illegal or insecure on the
actual target environment, based on incorrect assumptions about alignment, calling convention,
or integer promotion rules.

### ISpec3-A: Unaligned access on align-required architecture

**Source pattern:**
```c
// struct contains a pointer to a mis-aligned region
uint32_t *p = (uint32_t *)((char *)base + 1);
uint32_t val = *p;   // compiler may use VLDR / aligned load instruction
```

Look for: casts of `char*` or `uint8_t*` to a wider integer pointer type, followed by a dereference.

### ISpec3-B: Standard-function signature mismatch / Environment assumption

```c
// Kernel redefines memset without a return value
void memset(void *s, int c, size_t n);  // no return value
// Compiler assumes POSIX memset returns void* and uses R0 after call
```

Look for: locally-defined versions of standard functions with different signatures.

### ISpec3-C: Integer promotion breaking security checks

```c
uint8_t u = 255;
if (u + 1 == 0) { handle_overflow(); }  // u+1 promotes to int → 256, never 0
```

Look for: comparisons of `uint8_t`/`uint16_t` arithmetic results against 0 or limits, where
the intent is to detect wrap-around.

### ISpec3-D: printf→puts replacement

```c
printf("%s", maybe_null);   // compiler may replace with puts(maybe_null)
                             // puts() crashes on NULL; printf() doesn't
```

Look for: `printf` calls with a single `%s` argument (no format args) where the string
may be NULL.

---

## OSpec1 — Making Sensitive Data Out of Bound

**What happens:** Compiler extends the lifetime or memory region of sensitive data beyond the
developer-intended boundary, enabling disclosure.

### OSpec1-A: Dead Store Elimination of secret scrubbing (living-time boundary)

Same source pattern as ISpec1-B, but here the concern is **disclosure** rather than just
"was the code eliminated". Specifically, the sensitive data remains in memory until the next
use of that memory (stack frame reuse, heap reuse).

**Key indicators:**
- Functions handling cryptographic keys, passwords, authentication tokens.
- `memset`/`bzero` calls at the end of such functions (before `return` or scope exit).
- The result of the memset is never read.

### OSpec1-B: Uninitialized structure padding / partial union initialization (space boundary)

```c
struct Msg { int type; char data[16]; };
struct Msg msg;
msg.type = TYPE_A;
// msg.data not initialized — compiler may leave padding bytes uninitialized
send_to_userspace(&msg, sizeof(msg));
```

Look for:
- Structs with padding (determined from `sizeof(struct) > sum of fields`).
- The struct is partially initialized then passed across a trust boundary (kernel→user: `copy_to_user`, socket `send`, file `write`).

---

## OSpec2 — Breaking Timing Guarantees

**What happens:** Compiler optimization changes the execution time of a code path,
enabling timing side-channel attacks.

### OSpec2-A: Early-return in comparison functions

```c
int my_memcmp(const void *a, const void *b, size_t n) {
    for (size_t i = 0; i < n; i++)
        if (((uint8_t*)a)[i] != ((uint8_t*)b)[i]) return 1;  // early return — timing leak
    return 0;
}
```
Compiler may further optimize the loop in ways that amplify the timing signal.

Look for: comparison loops that short-circuit on mismatch when the result is used
for authentication/MAC verification.

### OSpec2-B: Delay loop optimization (concurrency sequencing)

```c
for (volatile int i = 0; i < DELAY; i++);  // wait for hardware
use_hardware();
```
Compiler may eliminate the delay or reduce it.

Look for: delay loops whose `volatile` annotation was removed or whose iteration count
is modified by constant folding.

---

## OSpec3 — Introducing Insecure Micro-architectural Side Effects

**What happens:** Compiler emits code that is architecturally correct but introduces
speculative execution paths exploitable via cache side-channel attacks (Spectre).

### OSpec3-A: Bounds-check bypass (switch→indirect-branch transformation)

```c
switch (x) {
    case 0: return arr[0];
    case 1: return arr[1];
    default: return -1;
}
// Compiler may transform to: if (x > 2) return -1; goto case_table[x];
// The goto is speculative — arr[x] may be loaded speculatively even when x is out of bounds
```

Look for:
- `switch` statements over an attacker-influenced index that access an array.
- Large switch tables that the compiler might transform to indirect branches.

---

## Detection Priority (by source-level detectability)

| Class   | Source-level signal                              | Requires binary confirmation? |
|---------|--------------------------------------------------|-------------------------------|
| ISpec1-B| memset before scope end, no explicit_bzero       | Yes (DSE happened?)           |
| ISpec1-A| Deref before null check; signed overflow check   | Yes (check absent in IR?)     |
| OSpec1-A| As ISpec1-B but near trust boundary              | Yes                           |
| OSpec1-B| Partial struct init before send/copy_to_user     | No (static, always dangerous) |
| ISpec3-C| u8/u16 arithmetic compared to 0 or limit         | No (always latent)            |
| ISpec3-D| printf single %s argument                        | No (always latent)            |
| ISpec2-A| Check before op, function assumed noreturn-like  | Yes                           |
| OSpec2-A| Comparison loop with early return for auth       | Partial                       |
| OSpec3-A| switch on attacker-input into array              | Yes                           |
| ISpec2-B| Struct init ordering                             | Yes                           |
| ISpec3-A| Misaligned cast + deref                          | Architecture-dependent        |
| OSpec2-B| Delay loop for sequencing                        | Yes                           |
| ISpec3-B| Redefined standard function                      | Yes                           |
