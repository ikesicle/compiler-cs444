#ifndef AST_ASTPRINTER_HPP
#define AST_ASTPRINTER_HPP

#include "ast_visitor.hpp"

namespace ast {

class ASTPrinter : public ASTVisitor {
private:
    int indentLevel = 0;
    void printIndent() const;
    
public:
    static ASTPrinter Instance;

    // Compilation Unit and Import Declarations
    bool visit(CompilationUnitNode* node) override;
    void leave(CompilationUnitNode* node) override;
    
    bool visit(PackageDeclarationNode* node) override;
    void leave(PackageDeclarationNode* node) override;
    
    bool visit(ImportDeclarationNode* node) override;
    void leave(ImportDeclarationNode* node) override;
    
    bool visit(TypeDeclarationNode* node) override;
    void leave(TypeDeclarationNode* node) override;
    
    bool visit(BodyDeclarationNode* node) override;
    void leave(BodyDeclarationNode* node) override;
    
    bool visit(FieldDeclarationNode* node) override;
    void leave(FieldDeclarationNode* node) override;
    
    bool visit(MethodDeclarationNode* node) override;
    void leave(MethodDeclarationNode* node) override;
    
    bool visit(ConstructorDeclarationNode* node) override;
    void leave(ConstructorDeclarationNode* node) override;
    
    // Variable Declarations
    bool visit(VariableDeclarationNode* node) override;
    void leave(VariableDeclarationNode* node) override;
    
    bool visit(SingleVariableDeclarationNode* node) override;
    void leave(SingleVariableDeclarationNode* node) override;
    
    bool visit(VariableDeclarationFragmentNode* node) override;
    void leave(VariableDeclarationFragmentNode* node) override;
    
    // Statements
    bool visit(StatementNode* node) override;
    void leave(StatementNode* node) override;
    
    bool visit(BlockNode* node) override;
    void leave(BlockNode* node) override;
    
    bool visit(EmptyStatementNode* node) override;
    void leave(EmptyStatementNode* node) override;
    
    bool visit(ExpressionStatementNode* node) override;
    void leave(ExpressionStatementNode* node) override;
    
    bool visit(ReturnStatementNode* node) override;
    void leave(ReturnStatementNode* node) override;
    
    bool visit(IfStatementNode* node) override;
    void leave(IfStatementNode* node) override;
    
    bool visit(WhileStatementNode* node) override;
    void leave(WhileStatementNode* node) override;
    
    bool visit(ForStatementNode* node) override;
    void leave(ForStatementNode* node) override;
    
    bool visit(VariableDeclarationStatementNode* node) override;
    void leave(VariableDeclarationStatementNode* node) override;
    
    // Expressions
    bool visit(ExpressionNode* node) override;
    void leave(ExpressionNode* node) override;
    
    bool visit(AssignmentExpressionNode* node) override;
    void leave(AssignmentExpressionNode* node) override;
    
    bool visit(InfixExpressionNode* node) override;
    void leave(InfixExpressionNode* node) override;
    
    bool visit(InstanceofExpressionNode* node) override;
    void leave(InstanceofExpressionNode* node) override;
    
    bool visit(PrefixExpressionNode* node) override;
    void leave(PrefixExpressionNode* node) override;
    
    bool visit(CastExpressionNode* node) override;
    void leave(CastExpressionNode* node) override;
    
    bool visit(VariableDeclarationExpressionNode* node) override;
    void leave(VariableDeclarationExpressionNode* node) override;
    
    // Types
    bool visit(TypeNode* node) override;
    void leave(TypeNode* node) override;
    
    bool visit(PrimitiveTypeNode* node) override;
    void leave(PrimitiveTypeNode* node) override;
    
    bool visit(ClassOrInterfaceTypeNode* node) override;
    void leave(ClassOrInterfaceTypeNode* node) override;
    
    bool visit(ArrayTypeNode* node) override;
    void leave(ArrayTypeNode* node) override;
    
    // Literals and Other Expressions
    bool visit(ArrayCreationExpressionNode* node) override;
    void leave(ArrayCreationExpressionNode* node) override;
    
    bool visit(IntegerLiteralNode* node) override;
    void leave(IntegerLiteralNode* node) override;
    
    bool visit(BooleanLiteralNode* node) override;
    void leave(BooleanLiteralNode* node) override;
    
    bool visit(CharacterLiteralNode* node) override;
    void leave(CharacterLiteralNode* node) override;
    
    bool visit(StringLiteralNode* node) override;
    void leave(StringLiteralNode* node) override;
    
    bool visit(NullLiteralNode* node) override;
    void leave(NullLiteralNode* node) override;
    
    bool visit(ThisExpressionNode* node) override;
    void leave(ThisExpressionNode* node) override;
    
    bool visit(ParenthesizedExpressionNode* node) override;
    void leave(ParenthesizedExpressionNode* node) override;
    
    bool visit(FieldAccessExpressionNode* node) override;
    void leave(FieldAccessExpressionNode* node) override;
    
    bool visit(MethodInvocationExpressionNode* node) override;
    void leave(MethodInvocationExpressionNode* node) override;
    
    bool visit(InstanceCreationExpressionNode* node) override;
    void leave(InstanceCreationExpressionNode* node) override;
    
    bool visit(ArrayAccessExpressionNode* node) override;
    void leave(ArrayAccessExpressionNode* node) override;
    
    // Names
    bool visit(NameNode* node) override;
    void leave(NameNode* node) override;
    
    bool visit(SimpleNameNode* node) override;
    void leave(SimpleNameNode* node) override;
    
    bool visit(QualifiedNameNode* node) override;
    void leave(QualifiedNameNode* node) override;
    
    // Modifiers
    bool visit(ModifierNode* node) override;
    void leave(ModifierNode* node) override;
};

}

#endif  // JOOSAST_ASTPRINTER_HPP