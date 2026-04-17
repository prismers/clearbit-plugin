void getQueryAnalysisUsage(AnalysisUsage &AU) {}

bool runQueryBody(const Module &M, const IRUri &Q, json::Object &Response) {
    if (!Q.instruction) {
        Response["found"] = false;
        Response["uri"] = IRUri::buildURI(&M);
        Response["error"] = "uri must resolve to an instruction";
        return false;
    }

    json::Array Users;
    for (const User *U : Q.instruction->users()) {
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

    Response["found"] = true;
    Response["uri"] = IRUri::buildURI(&M, Q.function, Q.block, Q.instruction);
    Response["kind"] = "instruction";
    Response["users"] = std::move(Users);
    return true;
}
