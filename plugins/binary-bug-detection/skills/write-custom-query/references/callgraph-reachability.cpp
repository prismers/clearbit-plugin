// Example 5 — Transitive callee reachability via CallGraphWrapperPass
//
// URI scope : ir://<module>/<function>  (the traversal root)
// Analyses  : CallGraphWrapperPass
//
// Performs a BFS over the module call graph starting from Q.function and
// returns every transitively reachable function that is *defined* in the module
// (declarations / external symbols are skipped).  Each entry carries its BFS
// depth, canonical URI, and best-effort assembly address and source location.
//
// Typical use: understand the full attack surface reachable from an entry
// point such as `main`, or check whether a dangerous callee is reachable
// from a given caller without running a full taint pass.

void getQueryAnalysisUsage(AnalysisUsage &AU) {
    AU.addRequired<CallGraphWrapperPass>();
    AU.setPreservesAll();
}

bool runQueryBody(const Module &M, const IRUri &Q, json::Object &Response) {
    if (!Q.function) {
        Response["found"] = false;
        Response["uri"]   = IRUri::buildURI(&M);
        Response["error"] = "uri must resolve to a function (the BFS root)";
        return false;
    }

    CallGraphWrapperPass &CGPass = getModuleAnalysis<CallGraphWrapperPass>();
    CallGraph &CG = CGPass.getCallGraph();
    CallGraphNode *RootNode = CG[Q.function];

    if (!RootNode) {
        Response["found"] = false;
        Response["uri"]   = IRUri::buildURI(&M, Q.function);
        Response["error"] = "function has no CallGraphNode";
        return false;
    }

    // BFS worklist: (CallGraphNode*, depth).
    // We advance a read index so we never need std::queue.
    SmallVector<std::pair<CallGraphNode *, unsigned>, 64> WorkList;
    SmallPtrSet<CallGraphNode *, 32> Visited;

    WorkList.push_back(std::make_pair(RootNode, 0u));
    Visited.insert(RootNode);

    json::Array Reachable;
    int64_t ReachableCount = 0;

    for (std::size_t Idx = 0; Idx < WorkList.size(); ++Idx) {
        CallGraphNode *Node  = WorkList[Idx].first;
        unsigned       Depth = WorkList[Idx].second;

        Function *Fn = Node->getFunction();
        if (Fn && Fn != Q.function && !Fn->isDeclaration()) {
            ++ReachableCount;
            json::Object Entry;
            Entry["name"]  = Fn->getName().str();
            Entry["depth"] = static_cast<int64_t>(Depth);
            Entry["uri"]   = IRUri::buildURI(&M, Fn);

            uint64_t Addr = 0;
            if (findFunctionAddr(*Fn, Addr)) {
                appendAsmLoc(Entry, Addr);
            }

            SourceLoc SL;
            if (findFunctionSrc(*Fn, SL)) {
                appendSrcLoc(Entry, SL);
            }

            Reachable.push_back(std::move(Entry));
        }

        // Expand all outgoing call edges one level deeper.
        for (auto &Edge : *Node) {
            CallGraphNode *Callee = Edge.second;
            if (Callee && !Visited.count(Callee)) {
                Visited.insert(Callee);
                WorkList.push_back(std::make_pair(Callee, Depth + 1));
            }
        }
    }

    Response["found"]           = true;
    Response["uri"]             = IRUri::buildURI(&M, Q.function);
    Response["kind"]            = "function";
    Response["root"]            = Q.function->getName().str();
    Response["reachable_count"] = ReachableCount;
    Response["reachable"]       = std::move(Reachable);
    return true;
}
