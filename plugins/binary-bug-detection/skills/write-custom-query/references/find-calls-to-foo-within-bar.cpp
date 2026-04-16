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
    if (!Uri.function) {
        Doc["found"] = false;
        Doc["uri"] = IRUri::buildURI(&M);
        Doc["error"] = "uri must resolve to a function";
        return Doc;
    }

    json::Array CallSites;
    for (const BasicBlock &BB : *Uri.function) {
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
            Site["uri"] = IRUri::buildURI(&M, Uri.function, &BB, &I);

            uint64_t Addr = 0;
            if (findInstrAddr(I, Addr)) {
                appendAsmLoc(Site, Addr);
            }

            CallSites.push_back(std::move(Site));
        }
    }

    Doc["found"] = true;
    Doc["uri"] = IRUri::buildURI(&M, Uri.function);
    Doc["kind"] = "function";
    Doc["call_sites"] = std::move(CallSites);
    return Doc;
}
