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
        Doc["error"] = "uri must resolve to an instruction";
        return Doc;
    }

    json::Array Defs;
    for (const Use &Op : Uri.instruction->operands()) {
        const Value *V = Op.get();
        json::Object Entry;

        if (const auto *DefInst = dyn_cast<Instruction>(V)) {
            const BasicBlock *DefBB = DefInst->getParent();
            const Function *DefFn = DefBB ? DefBB->getParent() : nullptr;

            Entry["kind"] = "instruction";
            Entry["uri"] = IRUri::buildURI(&M, DefFn, DefBB, DefInst);
            Entry["opcode"] = DefInst->getOpcodeName();

            if (DefBB) {
                std::size_t DefIdx = 0;
                if (IRUri::buildInstIdx(*DefBB, *DefInst, DefIdx)) {
                    Entry["index"] = static_cast<int64_t>(DefIdx);
                }
            }

            uint64_t Addr = 0;
            if (findInstrAddr(*DefInst, Addr)) {
                appendAsmLoc(Entry, Addr);
            }
        } else if (const auto *Arg = dyn_cast<Argument>(V)) {
            Entry["kind"] = "argument";
            Entry["uri"] = IRUri::buildURI(&M, Uri.function);
            Entry["name"] = Arg->getName().str();
        } else {
            Entry["kind"] = "constant";
            Entry["uri"] = "";
        }

        Defs.push_back(std::move(Entry));
    }

    Doc["found"] = true;
    Doc["uri"] = IRUri::buildURI(&M, Uri.function, Uri.block, Uri.instruction);
    Doc["kind"] = "instruction";
    Doc["definitions"] = std::move(Defs);
    return Doc;
}
