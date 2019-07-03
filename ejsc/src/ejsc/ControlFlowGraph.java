/*
 * eJS Project
 * Kochi University of Technology
 * The University of Electro-communications
 *
 * The eJS Project is the successor of the SSJS Project at The University of
 * Electro-communications.
 */
package ejsc;
import java.util.Collection;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;

public class ControlFlowGraph {
    static class CFGNode {
        private HashSet<CFGNode> pred = new HashSet<CFGNode>();
        private HashSet<CFGNode> succ = new HashSet<CFGNode>();
        private BCode bcode;
        CFGNode(BCode bcode) {
            this.bcode = bcode;
        }
        void addPred(CFGNode n) {
            pred.add(n);
        }
        void addSucc(CFGNode n) {
            succ.add(n);
        }
        public BCode getBCode() {
            return bcode;
        }
        public HashSet<CFGNode> getPreds() {
            return pred;
        }
        public HashSet<CFGNode> getSuccs() {
            return succ;
        }
    }
    private HashMap<BCode, CFGNode> cfg = new HashMap<BCode, CFGNode>();

    ControlFlowGraph(List<BCode> bcodes) {
        for (BCode bc: bcodes)
            cfg.put(bc, new CFGNode(bc));
        for (int i = 0; i < bcodes.size(); i++) {
            BCode bc = bcodes.get(i);
            CFGNode cfgNode = cfg.get(bc);
            if (bc.isFallThroughInstruction() && i + 1 < bcodes.size()) {
                BCode destBC = bcodes.get(i + 1);
                makeEdge(cfgNode, destBC);
            }
            BCode destBC = bc.getBranchTarget();
            if (destBC != null)
                makeEdge(cfgNode, destBC);
        }
    }

    private void makeEdge(CFGNode from, BCode toBC) {
        CFGNode to = cfg.get(toBC);
        from.addSucc(to);
        to.addPred(from);
    }

    public Collection<CFGNode> getNodes() {
        return cfg.values();
    }

    public CFGNode get(BCode bc) {
        return cfg.get(bc);
    }
}
