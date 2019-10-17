package specfile;

import java.io.FileNotFoundException;
import java.util.Collection;

import vmgen.type.TypeDefinition;
import vmgen.type.VMDataType;
import vmgen.type.VMRepType;

public class Datatypes {
    static public Datatypes loadFromFile(String fileName) throws FileNotFoundException {
        TypeDefinition.load(fileName);
        return new Datatypes();
    }
    String minimumRepresentation(Collection<VMRepType> dts, Collection<VMRepType> among) {
        /*
        if (!among.containsAll(dts))
            throw new Error("Internal error");

        if (dts.containsAll(among))
            return "1";

        ClassifiedVMRepTypes targetMap = new ClassifiedVMRepTypes(dts);
        ClassifiedVMRepTypes amongMap = new ClassifiedVMRepTypes(among);

        Collection<VMRepType.PT> unique = new HashSet<VMRepType.PT>();
        Collection<VMRepType.PT> common = new HashSet<VMRepType.PT>();
        Collection<VMRepType.HT> hts = new ArrayList<VMRepType.HT>();
        for (VMRepType.PT pt: targetMap.keySet()) {
            Set<VMRepType> targetSet = targetMap.get(pt);
            Set<VMRepType> amongSet = amongMap.get(pt);
            if (targetSet.containsAll(amongSet))
                unique.add(pt);
            else {
                common.add(pt);
                for (VMRepType rt: targetSet)
                    hts.add(rt.getHT());
            }
        }

        StringBuilder sb = new StringBuilder();
        sb.append("(((0");
        for (VMRepType.PT pt : common)
            sb.append(" || ")
            .append(String.format("(((x) & %s_MASK) == %s)", pt.getName(), pt.getName()));
        sb.append(") && (0");
        for (VMRepType.HT ht : hts)
            sb.append(" || ")
            .append(String.format("(obj_header_tag(x) == %s)", ht.getName()));
        sb.append("))");
        for (VMRepType.PT pt : unique)
            sb.append(" || ")
            .append(String.format("(((x) & %s_MASK) == %s)", pt.getName(), pt.getName()));
        sb.append(")");

        return sb.toString();
        */
        return "not implemented";
    }
    public String unparse() {
        StringBuffer sb = new StringBuffer();
        unparse(sb);
        return sb.toString();
    }
    public void unparse(StringBuffer sb) {
        sb.append("%% datatypes\n");

        for (VMDataType dt: VMDataType.all()) {
            if (dt.getVMRepTypes().isEmpty()) continue;

            sb.append(dt.getName())
            .append(" ")
            .append(minimumRepresentation(dt.getVMRepTypes(), VMRepType.all()))
            .append("\n");
        }

        return;
    }
}