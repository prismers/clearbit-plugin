# LLVM 3.6 Analysis API Reference

Available analyses for use in `getQueryAnalysisUsage` / `getFunctionAnalysis` / `getModuleAnalysis`.
All entries target the **LLVM 3.6 legacy pass manager**.

---

## AliasAnalysis

```cpp
AU.addRequired<AliasAnalysis>();
AliasAnalysis &AA = getFunctionAnalysis<AliasAnalysis>(*Q.function);
```

Key types and methods:

```cpp
// Location — construct from a const Value*
AliasAnalysis::Location loc(ptr, AliasAnalysis::Location::UnknownSize);
AliasAnalysis::Location loc = AA.getLocation(loadOrStoreInst); // extracts size + AA metadata

// Query — method is non-const on AA object
AliasAnalysis::AliasResult R = AA.alias(locA, locB);
// R: NoAlias=0, MayAlias=1, PartialAlias=2, MustAlias=3

// Mod/ref for a call site against a memory location
AliasAnalysis::ModRefResult MR = AA.getModRefInfo(CS, loc);
// MR: NoModRef=0, Ref, Mod, ModRef
```

Gotchas: `alias()` and `getModRefInfo()` are non-const on the `AliasAnalysis` object (that's fine — `getFunctionAnalysis` returns a non-const reference).

---

## MemoryDependenceAnalysis

```cpp
AU.addRequired<AliasAnalysis>();               // MDA depends on AA
AU.addRequired<MemoryDependenceAnalysis>();
MemoryDependenceAnalysis &MDA =
    getFunctionAnalysis<MemoryDependenceAnalysis>(*Q.function);
```

Key methods:

```cpp
// getDependency takes Instruction* (non-const). Recover from Q.block by identity
// comparison since Q.instruction is const — see Example 3.
MemDepResult Dep = MDA.getDependency(queryInst);

bool Dep.isDef()         // instruction that defines the loaded value
bool Dep.isClobber()     // instruction that may overwrite the location
bool Dep.isNonLocal()    // dependency exits the block; use getNonLocalPointerDependency
bool Dep.isNonFuncLocal()// dependency exits the function (escaped pointer, etc.)
Instruction *Dep.getInst()  // valid when isDef() || isClobber(); returns non-const ptr

// Non-local pointer dependency (cross-block)
SmallVector<NonLocalDepResult, 8> Results;
MDA.getNonLocalPointerDependency(loc, /*isLoad=*/true, const_cast<BasicBlock*>(Q.block), Results);
for (auto &R : Results) {
    BasicBlock  *BB  = R.getBB();
    MemDepResult Res = R.getResult();
}
```

---

## DominatorTree

```cpp
AU.addRequired<DominatorTreeWrapperPass>();
DominatorTree &DT =
    getFunctionAnalysis<DominatorTreeWrapperPass>(*Q.function).getDomTree();
```

Key methods (all `const` on `DominatorTree`):

```cpp
bool DT.dominates(const Instruction *A, const Instruction *B);  // A dom B?
bool DT.dominates(const BasicBlock  *A, const BasicBlock  *B);
bool DT.properlyDominates(const BasicBlock *A, const BasicBlock *B);
bool DT.isReachableFromEntry(const BasicBlock *BB);
// Unreachable blocks have no DomTreeNode: DT.getNode(BB) == nullptr
```

---

## PostDominatorTree

```cpp
AU.addRequired<PostDominatorTree>();
PostDominatorTree &PDT = getFunctionAnalysis<PostDominatorTree>(*Q.function);
```

Key methods (block-level only — no instruction-level overload in 3.6):

```cpp
bool PDT.dominates(const BasicBlock *A, const BasicBlock *B);  // A post-dom B?
bool PDT.properlyDominates(const BasicBlock *A, const BasicBlock *B);
// Blocks not reaching any exit have no PDT node: PDT.getNode(BB) == nullptr
```

Gotcha: Some base-class methods on `DominatorTreeBase` take non-const `BasicBlock*` in 3.6. Prefer holding a non-const `PostDominatorTree &` reference.

---

## LoopInfo

**Note:** `LoopInfoWrapperPass` does not exist in LLVM 3.6 (added in 3.7). Use `LoopInfo` directly.

```cpp
AU.addRequired<DominatorTreeWrapperPass>(); // LoopInfo depends on DomTree
AU.addRequired<LoopInfo>();
LoopInfo &LI = getFunctionAnalysis<LoopInfo>(*Q.function);
```

Key methods:

```cpp
Loop   *LI.getLoopFor(const BasicBlock *BB);   // innermost Loop*, or nullptr
unsigned LI.getLoopDepth(const BasicBlock *BB); // 0 = not in loop, 1 = outermost, ...

// Loop* methods
BasicBlock *L->getHeader();
BasicBlock *L->getLoopPreheader();       // nullptr if no unique preheader
bool        L->contains(const BasicBlock *BB);
bool        L->contains(const Instruction *I);
unsigned    L->getLoopDepth();
// Sub-loops:
for (Loop *SubL : *L) { ... }           // Loop::iterator

// Iterate all top-level loops in a function:
for (Loop *L : LI) { ... }
```

Gotcha: `LI.isLoopHeader(BasicBlock*)` takes a non-const pointer in 3.6. Use `LI.getLoopFor(BB) && LI.getLoopFor(BB)->getHeader() == BB` instead when you only have `const BasicBlock*`.

---

## ScalarEvolution

**Note:** `ScalarEvolutionWrapperPass` does not exist in LLVM 3.6 (added in 3.8). Use `ScalarEvolution` directly.

```cpp
AU.addRequired<LoopInfo>();
AU.addRequired<ScalarEvolution>();
ScalarEvolution &SE = getFunctionAnalysis<ScalarEvolution>(*Q.function);
```

Key methods (nearly all non-const — SE caches results internally):

```cpp
const SCEV *SE.getSCEV(Value *V);                         // expression for a value
const SCEV *SE.getBackedgeTakenCount(const Loop *L);      // may be SCEVCouldNotCompute
const SCEV *SE.getMaxBackedgeTakenCount(const Loop *L);
bool        SE.isLoopInvariant(const SCEV *S, const Loop *L);
bool        SE.hasLoopInvariantBackedgeTakenCount(const Loop *L);

// Check if a result is unknown:
if (isa<SCEVCouldNotCompute>(SE.getBackedgeTakenCount(L))) { ... }

// Useful subclass casts (include ScalarEvolutionExpressions.h):
if (const SCEVConstant *C = dyn_cast<SCEVConstant>(scev)) {
    int64_t val = C->getValue()->getSExtValue();
}
if (const SCEVAddRecExpr *AR = dyn_cast<SCEVAddRecExpr>(scev)) {
    const SCEV *start = AR->getStart();
    const SCEV *step  = AR->getStepRecurrence(SE);
    bool isAffine     = AR->isAffine();
}
```

Gotcha: `getSCEV` takes `Value*` (non-const). Recover non-const pointer from `Q.block` by iteration as shown in Example 3.

---

## DataLayout

```cpp
AU.addRequired<DataLayoutPass>();
const DataLayout &DL = getModuleAnalysis<DataLayoutPass>().getDataLayout();
```

Key methods (all `const`):

```cpp
uint64_t DL.getTypeAllocSize(Type *Ty);      // bytes allocated (with padding)
uint64_t DL.getTypeSizeInBits(Type *Ty);     // exact bit width (no padding)
unsigned DL.getPointerSize(unsigned AS = 0); // pointer size in bytes
unsigned DL.getPointerSizeInBits(unsigned AS = 0);
unsigned DL.getABITypeAlignment(Type *Ty);
```

---

## CallGraph

CallGraph is a **module-level** analysis. Only use it when your query scope is `ir://<module>` (i.e. `Q.function == nullptr`).

```cpp
AU.addRequired<CallGraphWrapperPass>();
CallGraph &CG = getModuleAnalysis<CallGraphWrapperPass>().getCallGraph();
```

Key methods:

```cpp
CallGraphNode *CG[&F];  // node for function F

// Iterate callees of F:
for (auto &CR : *CG[&F]) {
    // CR.first  = WeakVH to the call instruction (may be null for implicit edges)
    // CR.second = CallGraphNode* of the callee
    Function *calleeF = CR.second->getFunction(); // null = calls-external node
}

// Special nodes:
CG.getExternalCallingNode(); // represents external callers of this module
CG.getCallsExternalNode();   // represents calls to unknown/external functions

// Reverse (callers): no direct API — scan all nodes manually.
```

Gotcha: Indirect calls appear as edges to `getCallsExternalNode()`, losing callee identity.
