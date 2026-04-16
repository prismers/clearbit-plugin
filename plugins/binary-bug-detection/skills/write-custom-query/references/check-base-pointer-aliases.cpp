static const Value *stripGEPsAndCasts(const Value *V) {
    while (true) {
        if (const auto *GEP = dyn_cast<GetElementPtrInst>(V)) {
            V = GEP->getPointerOperand();
        } else if (const auto *BC = dyn_cast<BitCastInst>(V)) {
            V = BC->getOperand(0);
        } else {
            break;
        }
    }
    return V;
}

json::Object runCustomQuery(Module &M, const json::Object &Args) {
    json::Object Doc;
    IRUri Uri;
    std::string ErrorURI, ErrorMsg;
    if (!resolveIRFromArgs(M, Args, Uri, ErrorURI, ErrorMsg)) {
        Doc["found"] = false;
        Doc["uri"] = ErrorURI;
        Doc["error"] = ErrorMsg;
        return Doc;
    }
    if (!Uri.instruction) {
        Doc["found"] = false;
        Doc["uri"] = IRUri::buildURI(&M);
        Doc["error"] = "uri must resolve to a load or store instruction";
        return Doc;
    }

    const Value *TargetPtr = nullptr;
    if (const auto *LI = dyn_cast<LoadInst>(Uri.instruction)) {
        TargetPtr = LI->getPointerOperand();
    } else if (const auto *SI = dyn_cast<StoreInst>(Uri.instruction)) {
        TargetPtr = SI->getPointerOperand();
    }
    if (!TargetPtr) {
        Doc["found"] = false;
        Doc["uri"] = IRUri::buildURI(&M, Uri.function, Uri.block, Uri.instruction);
        Doc["error"] = "target instruction is not a load or store";
        return Doc;
    }

    const Value *TargetBase = stripGEPsAndCasts(TargetPtr);

    json::Array Aliases;
    for (const BasicBlock &BB : *Uri.function) {
        for (const Instruction &I : BB) {
            const Value *Ptr = nullptr;
            if (const auto *LI = dyn_cast<LoadInst>(&I)) {
                Ptr = LI->getPointerOperand();
            } else if (const auto *SI = dyn_cast<StoreInst>(&I)) {
                Ptr = SI->getPointerOperand();
            }

            if (!Ptr || stripGEPsAndCasts(Ptr) != TargetBase || &I == Uri.instruction) {
                continue;
            }

            json::Object Entry;
            Entry["uri"] = IRUri::buildURI(&M, Uri.function, &BB, &I);
            Entry["opcode"] = I.getOpcodeName();

            uint64_t Addr = 0;
            if (findInstrAddr(I, Addr)) {
                appendAsmLoc(Entry, Addr);
            }

            Aliases.push_back(std::move(Entry));
        }
    }

    Doc["found"] = true;
    Doc["uri"] = IRUri::buildURI(&M, Uri.function, Uri.block, Uri.instruction);
    Doc["kind"] = "instruction";
    Doc["base_aliases"] = std::move(Aliases);
    return Doc;
}
