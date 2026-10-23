#ifndef AST_ASTVISITOR_HPP
#define AST_ASTVISITOR_HPP

#include "nodes.hpp"

namespace ast {

class ASTVisitor {
public:
    virtual ~ASTVisitor() = default;
    
    // Compilation Unit and Import Declarations
    virtual bool visit(CompilationUnitNode* node) { return true; }
    virtual void leave(CompilationUnitNode* node) {}
    
    virtual bool visit(PackageDeclarationNode* node) { return true; }
    virtual void leave(PackageDeclarationNode* node) {}
    
    virtual bool visit(ImportDeclarationNode* node) { return true; }
    virtual void leave(ImportDeclarationNode* node) {}
    
    virtual bool visit(TypeDeclarationNode* node) { return true; }
    virtual void leave(TypeDeclarationNode* node) {}
    
    virtual bool visit(BodyDeclarationNode* node) { return true; }
    virtual void leave(BodyDeclarationNode* node) {}
    
    virtual bool visit(FieldDeclarationNode* node) { return true; }
    virtual void leave(FieldDeclarationNode* node) {}
    
    virtual bool visit(MethodDeclarationNode* node) { return true; }
    virtual void leave(MethodDeclarationNode* node) {}
    
    virtual bool visit(ConstructorDeclarationNode* node) { return true; }
    virtual void leave(ConstructorDeclarationNode* node) {}
    
    // Variable Declarations
    virtual bool visit(VariableDeclarationNode* node) { return true; }
    virtual void leave(VariableDeclarationNode* node) {}
    
    virtual bool visit(SingleVariableDeclarationNode* node) { return true; }
    virtual void leave(SingleVariableDeclarationNode* node) {}
    
    virtual bool visit(VariableDeclarationFragmentNode* node) { return true; }
    virtual void leave(VariableDeclarationFragmentNode* node) {}
    
    // Statements
    virtual bool visit(StatementNode* node) { return true; }
    virtual void leave(StatementNode* node) {}
    
    virtual bool visit(BlockNode* node) { return true; }
    virtual void leave(BlockNode* node) {}
    
    virtual bool visit(EmptyStatementNode* node) { return true; }
    virtual void leave(EmptyStatementNode* node) {}
    
    virtual bool visit(ExpressionStatementNode* node) { return true; }
    virtual void leave(ExpressionStatementNode* node) {}
    
    virtual bool visit(ReturnStatementNode* node) { return true; }
    virtual void leave(ReturnStatementNode* node) {}
    
    virtual bool visit(IfStatementNode* node) { return true; }
    virtual void leave(IfStatementNode* node) {}
    
    virtual bool visit(WhileStatementNode* node) { return true; }
    virtual void leave(WhileStatementNode* node) {}
    
    virtual bool visit(ForStatementNode* node) { return true; }
    virtual void leave(ForStatementNode* node) {}
    
    virtual bool visit(VariableDeclarationStatementNode* node) { return true; }
    virtual void leave(VariableDeclarationStatementNode* node) {}
    
    // Expressions
    virtual bool visit(ExpressionNode* node) { return true; }
    virtual void leave(ExpressionNode* node) {}
    
    virtual bool visit(AssignmentExpressionNode* node) { return true; }
    virtual void leave(AssignmentExpressionNode* node) {}
    
    virtual bool visit(InfixExpressionNode* node) { return true; }
    virtual void leave(InfixExpressionNode* node) {}
    
    virtual bool visit(InstanceofExpressionNode* node) { return true; }
    virtual void leave(InstanceofExpressionNode* node) {}
    
    virtual bool visit(PrefixExpressionNode* node) { return true; }
    virtual void leave(PrefixExpressionNode* node) {}
    
    virtual bool visit(CastExpressionNode* node) { return true; }
    virtual void leave(CastExpressionNode* node) {}
    
    virtual bool visit(VariableDeclarationExpressionNode* node) { return true; }
    virtual void leave(VariableDeclarationExpressionNode* node) {}
    
    // Types
    virtual bool visit(TypeNode* node) { return true; }
    virtual void leave(TypeNode* node) {}
    
    virtual bool visit(PrimitiveTypeNode* node) { return true; }
    virtual void leave(PrimitiveTypeNode* node) {}
    
    virtual bool visit(ClassOrInterfaceTypeNode* node) { return true; }
    virtual void leave(ClassOrInterfaceTypeNode* node) {}
    
    virtual bool visit(ArrayTypeNode* node) { return true; }
    virtual void leave(ArrayTypeNode* node) {}
    
    // Literals and Other Expressions
    virtual bool visit(ArrayCreationExpressionNode* node) { return true; }
    virtual void leave(ArrayCreationExpressionNode* node) {}
    
    virtual bool visit(IntegerLiteralNode* node) { return true; }
    virtual void leave(IntegerLiteralNode* node) {}
    
    virtual bool visit(BooleanLiteralNode* node) { return true; }
    virtual void leave(BooleanLiteralNode* node) {}
    
    virtual bool visit(CharacterLiteralNode* node) { return true; }
    virtual void leave(CharacterLiteralNode* node) {}
    
    virtual bool visit(StringLiteralNode* node) { return true; }
    virtual void leave(StringLiteralNode* node) {}
    
    virtual bool visit(NullLiteralNode* node) { return true; }
    virtual void leave(NullLiteralNode* node) {}
    
    virtual bool visit(ThisExpressionNode* node) { return true; }
    virtual void leave(ThisExpressionNode* node) {}
    
    virtual bool visit(ParenthesizedExpressionNode* node) { return true; }
    virtual void leave(ParenthesizedExpressionNode* node) {}
    
    virtual bool visit(FieldAccessExpressionNode* node) { return true; }
    virtual void leave(FieldAccessExpressionNode* node) {}
    
    virtual bool visit(MethodInvocationExpressionNode* node) { return true; }
    virtual void leave(MethodInvocationExpressionNode* node) {}
    
    virtual bool visit(InstanceCreationExpressionNode* node) { return true; }
    virtual void leave(InstanceCreationExpressionNode* node) {}
    
    virtual bool visit(ArrayAccessExpressionNode* node) { return true; }
    virtual void leave(ArrayAccessExpressionNode* node) {}
    
    // Names
    virtual bool visit(NameNode* node) { return true; }
    virtual void leave(NameNode* node) {}
    
    virtual bool visit(SimpleNameNode* node) { return true; }
    virtual void leave(SimpleNameNode* node) {}
    
    virtual bool visit(QualifiedNameNode* node) { return true; }
    virtual void leave(QualifiedNameNode* node) {}
    
    // Modifiers
    virtual bool visit(ModifierNode* node) { return true; }
    virtual void leave(ModifierNode* node) {}
};

}

#endif // JOOSAST_ASTVISITOR_HPP