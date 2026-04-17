void getQueryAnalysisUsage(AnalysisUsage &AU) {}

bool runQueryBody(const Module &M, const IRUri &Q, json::Object &Response) {
    if (!Q.function) {
        Response["found"] = false;
        Response["uri"] = IRUri::buildURI(&M);
        Response["error"] = "uri must resolve to a function";
        return false;
    }

    json::Array CallSites;
    for (const BasicBlock &BB : *Q.function) {
        for (const Instruction &I : BB) {
            const auto *CI = dyn_cast<CallInst>(&I);
            if (!CI) {
                continue;
            }

            const Function *Callee = CI->getCalledFunction();
            if (!Callee || Callee->getName() != "foo") {
                continue;
            }

            json::Object Site;
            Site["uri"] = IRUri::buildURI(&M, Q.function, &BB, &I);

            uint64_t Addr = 0;
            if (findInstrAddr(I, Addr)) {
                appendAsmLoc(Site, Addr);
            }

            CallSites.push_back(std::move(Site));
        }
    }

    Response["found"] = true;
    Response["uri"] = IRUri::buildURI(&M, Q.function);
    Response["kind"] = "function";
    Response["call_sites"] = std::move(CallSites);
    return true;
}
