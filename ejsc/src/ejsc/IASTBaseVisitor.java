/*
 * eJS Project
 * Kochi University of Technology
 * The University of Electro-communications
 *
 * The eJS Project is the successor of the SSJS Project at The University of
 * Electro-communications.
 */
package ejsc;

public class IASTBaseVisitor {
    public Object visitNode(IASTNode node) {
        return null;
    }
    public Object visitProgram(IASTProgram node) {
        for (IASTFunctionExpression toplevel: node.programs)
            toplevel.accept(this);
        return visitNode(node);
    }
    public Object visitLiteral(IASTLiteral node) {
        return visitNode(node);
    }
    public Object visitStringLiteral(IASTStringLiteral node) {
        return visitLiteral(node);
    }
    public Object visitNumericLiteral(IASTNumericLiteral node) {
        return visitLiteral(node);
    }
    public Object visitBooleanLiteral(IASTBooleanLiteral node) {
        return visitLiteral(node);
    }
    public Object visitRegExpLiteral(IASTRegExpLiteral node) {
        return visitLiteral(node);
    }
    public Object visitNullLiteral(IASTNullLiteral node) {
        return visitLiteral(node);
    }
    public Object visitStatement(IASTStatement node) {
        return visitNode(node);
    }
    public Object visitBlockStatement(IASTBlockStatement node) {
        for (IASTStatement s: node.stmts)
            s.accept(this);
        return visitStatement(node);
    }
    public Object visitExpressionStatement(IASTExpressionStatement node) {
        node.exp.accept(this);
        return visitStatement(node);
    }
    public Object visitReturnStatement(IASTReturnStatement node) {
        if (node.value != null)
            node.value.accept(this);
        return visitStatement(node);
    }
    public Object visitEmptyStatement(IASTEmptyStatement node) {
        return visitStatement(node);
    }
    public Object visitWithStatement(IASTWithStatement node) {
        node.object.accept(this);
        node.body.accept(this);
        return visitStatement(node);
    }
    public Object visitIfStatement(IASTIfStatement node) {
        node.test.accept(this);
        node.consequent.accept(this);
        if (node.alternate != null)
            node.alternate.accept(this);
        return visitStatement(node);
    }
    public Object visitSwitchStatement(IASTSwitchStatement node) {
        node.discriminant.accept(this);
        for (IASTSwitchStatement.CaseClause c: node.cases) {
            if (c.test != null)
                c.test.accept(this);
            c.consequent.accept(this);
        }
        return visitStatement(node);
    }
    public Object visitThrowStatement(IASTThrowStatement node) {
        node.value.accept(this);
        return visitStatement(node);
    }
    public Object visitTryCatchStatement(IASTTryCatchStatement node){
        node.body.accept(this);
        node.handler.accept(this);
        return visitStatement(node);
    }
    public Object visitTryFinallyStatement(IASTTryFinallyStatement node){
        node.body.accept(this);
        node.finaliser.accept(this);
        return visitStatement(node);
    }
    public Object visitForStatement(IASTForStatement node) {
        if (node.init != null)
            node.init.accept(this);
        if (node.test != null)
            node.test.accept(this);
        if (node.update != null)
            node.update.accept(this);
        node.body.accept(this);
        return visitStatement(node);
    }
    public Object visitWhileStatement(IASTWhileStatement node) {
        node.test.accept(this);
        node.body.accept(this);
        return visitStatement(node);
    }
    public Object visitDoWhileStatement(IASTDoWhileStatement node) {
        node.test.accept(this);
        node.body.accept(this);
        return visitStatement(node);
    }
    public Object visitForInStatement(IASTForInStatement node) {
        node.object.accept(this);
        node.body.accept(this);
        return visitStatement(node);
    }
    public Object visitBreakStatement(IASTBreakStatement node) {
        return visitStatement(node);
    }
    public Object visitContinueStatement(IASTContinueStatement node) {
        return visitStatement(node);
    }
    public Object visitExpression(IASTExpression node) {
        return visitNode(node);
    }
    public Object visitThisExpression(IASTThisExpression node) {
        return visitExpression(node);
    }
    public Object visitArrayExpression(IASTArrayExpression node) {
        for (IASTExpression e: node.elements)
            e.accept(this);
        return visitExpression(node);
    }
    public Object visitObjectExpression(IASTObjectExpression node) {
        for (IASTObjectExpression.Property p: node.properties) {
            p.key.accept(this);
            p.value.accept(this);
        }
        return visitExpression(node);
    }
    public Object visitFunctionExpression(IASTFunctionExpression node) {
        node.body.accept(this);
        return visitExpression(node);
    }
    public Object visitOperatorExpression(IASTOperatorExpression node) {
        for(IASTExpression e: node.operands)
            e.accept(this);
        return visitExpression(node);
    }
    public Object visitUnaryExpression(IASTUnaryExpression node) {
        return visitOperatorExpression(node);
    }

    public Object visitBinaryExpression(IASTBinaryExpression node) {
        return visitOperatorExpression(node);
    }
    public Object visitTernaryExpression(IASTTernaryExpression node) {
        return visitOperatorExpression(node);
    }
    public Object visitCallExpression(IASTCallExpression node) {
        node.callee.accept(this);
        for (IASTExpression e: node.arguments)
            e.accept(this);
        return visitExpression(node);
    }
    public Object visitNewExpression(IASTNewExpression node) {
        node.constructor.accept(this);
        for (IASTExpression e: node.arguments)
            e.accept(this);
        return visitExpression(node);
    }
    public Object visitMemberExpression(IASTMemberExpression node) {
        node.object.accept(this);
        node.property.accept(this);
        return visitExpression(node);
    }
    public Object visitIdentifier(IASTIdentifier node) {
        return visitExpression(node);
    }
    public Object visitSequenceExpression(IASTSequenceExpression node) {
        for (IASTExpression e : node.expressions) {
            e.accept(this);
        }
        return visitExpression(node);
    }
}
