/*
 * eJS Project
 * Kochi University of Technology
 * The University of Electro-communications
 *
 * The eJS Project is the successor of the SSJS Project at The University of
 * Electro-communications.
 */
package ejsc.ast_node;

import java.util.List;

import javax.json.Json;
import javax.json.JsonArrayBuilder;
import javax.json.JsonObject;
import javax.json.JsonObjectBuilder;

import ejsc.ast_node.Node.*;

public class SwitchStatement extends Node implements ISwitchStatement {

    IExpression discriminant;
    List<ISwitchCase> cases;

    public SwitchStatement(IExpression discriminant, List<ISwitchCase> cases) {
        type = SWITCH_STMT;
        this.discriminant = discriminant;
        this.cases = cases;
    }

    public IExpression getDiscriminant() {
        return discriminant;
    }

    public List<ISwitchCase> getCases() {
        return cases;
    }

    @Override
    public JsonObject getEsTree() {
        JsonArrayBuilder casesJb = Json.createArrayBuilder();
        for (ISwitchCase sc : cases) {
            casesJb.add(sc.getEsTree());
        }
        JsonObjectBuilder jb = Json.createObjectBuilder()
                .add(KEY_TYPE, "SwitchStatement")
                .add(KEY_DISCRIMINANT, discriminant.getEsTree())
                .add(KEY_CASES, casesJb);
        return jb.build();
    }

    @Override
    public <T> T accept(ESTreeBaseVisitor<T> visitor) {
        return visitor.visitSwitchStatement(this);
    }
}
