void getQueryAnalysisUsage(AnalysisUsage &AU) {
    AU.addRequired<AliasAnalysis>();
}

bool runQueryBody(const Module &M, const IRUri &Q, json::Object &Response) {
    if (!Q.instruction || !Q.function) {
        Response["found"] = false;
        Response["uri"] = IRUri::buildURI(&M);
        Response["error"] = "uri must resolve to a load or store instruction";
        return false;
    }

    const Value *TargetPtr = nullptr;
    if (const auto *LI = dyn_cast<LoadInst>(Q.instruction)) {
        TargetPtr = LI->getPointerOperand();
    } else if (const auto *SI = dyn_cast<StoreInst>(Q.instruction)) {
        TargetPtr = SI->getPointerOperand();
    }
    if (!TargetPtr) {
        Response["found"] = false;
        Response["uri"] = IRUri::buildURI(&M, Q.function, Q.block, Q.instruction);
        Response["error"] = "target instruction is not a load or store";
        return false;
    }

    AliasAnalysis &AA = getFunctionAnalysis<AliasAnalysis>(*Q.function);
    AliasAnalysis::Location TargetLoc(TargetPtr, AliasAnalysis::Location::UnknownSize);

    json::Array Aliases;
    for (const BasicBlock &BB : *Q.function) {
        for (const Instruction &I : BB) {
            if (&I == Q.instruction) {
                continue;
            }

            const Value *Ptr = nullptr;
            if (const auto *LI = dyn_cast<LoadInst>(&I)) {
                Ptr = LI->getPointerOperand();
            } else if (const auto *SI = dyn_cast<StoreInst>(&I)) {
                Ptr = SI->getPointerOperand();
            }
            if (!Ptr) {
                continue;
            }

            AliasAnalysis::AliasResult R =
                AA.alias(TargetLoc, AliasAnalysis::Location(Ptr, AliasAnalysis::Location::UnknownSize));
            if (R == AliasAnalysis::NoAlias) {
                continue;
            }

            json::Object Entry;
            Entry["uri"] = IRUri::buildURI(&M, Q.function, &BB, &I);
            Entry["opcode"] = I.getOpcodeName();

            if (R == AliasAnalysis::MustAlias) {
                Entry["alias_result"] = "must";
            } else if (R == AliasAnalysis::PartialAlias) {
                Entry["alias_result"] = "partial";
            } else {
                Entry["alias_result"] = "may";
            }

            uint64_t Addr = 0;
            if (findInstrAddr(I, Addr)) {
                appendAsmLoc(Entry, Addr);
            }

            Aliases.push_back(std::move(Entry));
        }
    }

    Response["found"] = true;
    Response["uri"] = IRUri::buildURI(&M, Q.function, Q.block, Q.instruction);
    Response["kind"] = "instruction";
    Response["aliases"] = std::move(Aliases);
    return true;
}
