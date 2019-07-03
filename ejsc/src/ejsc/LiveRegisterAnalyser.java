/*
 * eJS Project
 * Kochi University of Technology
 * The University of Electro-communications
 *
 * The eJS Project is the successor of the SSJS Project at The University of
 * Electro-communications.
 */
package ejsc;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Set;

public class LiveRegisterAnalyser {
    HashMap<BCode, Set<Register>> inMap = new HashMap<BCode, Set<Register>>();
    HashMap<BCode, Set<Register>> outMap = new HashMap<BCode, Set<Register>>();

    public Set<Register> getLiveRegisters(BCode bc) {
        return inMap.get(bc);
    }

    public LiveRegisterAnalyser(List<BCode> bcodes) {
        ControlFlowGraph cfg = new ControlFlowGraph(bcodes);
        for (BCode bc: bcodes) {
            inMap.put(bc,  new HashSet<Register>());
            outMap.put(bc, new HashSet<Register>(bc.getSrcRegisters())); // gen
        }

        boolean fixPoint = false;
        while (!fixPoint) {
            fixPoint = true;
            for (ControlFlowGraph.CFGNode n: cfg.getNodes()) {
                BCode bc = n.getBCode();
                Set<Register> in = inMap.get(bc);
                Set<Register> out = outMap.get(bc);
                /* in += Union succ(out) */
                for (ControlFlowGraph.CFGNode succ: n.getSuccs()) {
                    BCode succBC = succ.getBCode();
                    if (in.addAll(outMap.get(succBC)))
                        fixPoint = false;
                }
                /* out += in - kill */
                for (Register r: in) {
                    if (r == bc.getDestRegister()) // kill
                        continue;
                    if (out.add(r))
                        fixPoint = false;
                }
            }
        }
    }

    public void print(List<BCode> bcodes) {
        System.out.println("----- Live Register Analyser begin -----");
        for (BCode bc: bcodes) {
            System.out.println(bc.number + ": "+bc+" "+showRegs(getLiveRegisters(bc)));
        }
        System.out.println("----- Live Register Analyser end -----");
    }

    static public String showRegs(Set<Register> regs) {
        String s = "{";
        for (Register r: regs)
            s += "r" + r + " ";
        return s + "}";
    }
}
