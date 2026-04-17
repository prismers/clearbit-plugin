void getQueryAnalysisUsage(AnalysisUsage &AU) {
    AU.addRequired<AliasAnalysis>();
    AU.addRequired<MemoryDependenceAnalysis>();
}

bool runQueryBody(const Module &M, const IRUri &Q, json::Object &Response) {
    if (!Q.instruction || !Q.block || !Q.function) {
        Response["found"] = false;
        Response["uri"] = IRUri::buildURI(&M);
        Response["error"] = "uri must resolve to a load or store instruction";
        return false;
    }

    if (!isa<LoadInst>(Q.instruction) && !isa<StoreInst>(Q.instruction)) {
        Response["found"] = false;
        Response["uri"] = IRUri::buildURI(&M, Q.function, Q.block, Q.instruction);
        Response["error"] = "instruction is not a load or store";
        return false;
    }

    // getDependency requires a non-const Instruction*. Recover it by iterating
    // the block — comparing against Q.instruction (const Instruction*) is safe
    // because Instruction* implicitly converts to const Instruction* for ==.
    Instruction *QueryInst = nullptr;
    for (Instruction &I : *Q.block) {
        if (&I == Q.instruction) {
            QueryInst = &I;
            break;
        }
    }
    if (!QueryInst) {
        Response["found"] = false;
        Response["uri"] = IRUri::buildURI(&M, Q.function, Q.block, Q.instruction);
        Response["error"] = "could not locate instruction in block";
        return false;
    }

    MemoryDependenceAnalysis &MDA =
        getFunctionAnalysis<MemoryDependenceAnalysis>(*Q.function);
    MemDepResult Dep = MDA.getDependency(QueryInst);

    json::Object DepObj;
    if (Dep.isDef()) {
        DepObj["kind"] = "def";
    } else if (Dep.isClobber()) {
        DepObj["kind"] = "clobber";
    } else if (Dep.isNonLocal()) {
        DepObj["kind"] = "non_local";
    } else if (Dep.isNonFuncLocal()) {
        DepObj["kind"] = "non_func_local";
    } else {
        DepObj["kind"] = "unknown";
    }

    if (Dep.isDef() || Dep.isClobber()) {
        Instruction *DepI = Dep.getInst();
        if (DepI) {
            const BasicBlock *DepBB = DepI->getParent();
            const Function *DepFn = DepBB ? DepBB->getParent() : nullptr;
            DepObj["uri"] = IRUri::buildURI(&M, DepFn, DepBB, DepI);
            DepObj["opcode"] = DepI->getOpcodeName();
            uint64_t Addr = 0;
            if (findInstrAddr(*DepI, Addr)) {
                appendAsmLoc(DepObj, Addr);
            }
        }
    }

    Response["found"] = true;
    Response["uri"] = IRUri::buildURI(&M, Q.function, Q.block, Q.instruction);
    Response["kind"] = "instruction";
    Response["dependency"] = std::move(DepObj);
    return true;
}
