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

    json::Array Users;
    for (const User *U : Uri.instruction->users()) {
        const auto *UserInst = dyn_cast<Instruction>(U);
        if (!UserInst) {
            continue;
        }

        const BasicBlock *UserBB = UserInst->getParent();
        const Function *UserFn = UserBB ? UserBB->getParent() : nullptr;
        if (!UserBB || !UserFn) {
            continue;
        }

        json::Object Entry;
        Entry["uri"] = IRUri::buildURI(&M, UserFn, UserBB, UserInst);
        Entry["opcode"] = UserInst->getOpcodeName();

        std::size_t UserIdx = 0;
        if (IRUri::buildInstIdx(*UserBB, *UserInst, UserIdx)) {
            Entry["index"] = static_cast<int64_t>(UserIdx);
        }

        uint64_t Addr = 0;
        if (findInstrAddr(*UserInst, Addr)) {
            appendAsmLoc(Entry, Addr);
        }

        Users.push_back(std::move(Entry));
    }

    Doc["found"] = true;
    Doc["uri"] = IRUri::buildURI(&M, Uri.function, Uri.block, Uri.instruction);
    Doc["kind"] = "instruction";
    Doc["users"] = std::move(Users);
    return Doc;
}
