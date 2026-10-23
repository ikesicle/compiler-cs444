#include "ast/ast_printer.hpp"
#include "ast/java_type.hpp"
#include <iostream>

namespace ast {

void ASTPrinter::printIndent() const {
    for (int i = 0; i < indentLevel; ++i) {
        std::cout << "  ";
    }
}

// Compilation Unit and Import Declarations

bool ASTPrinter::visit(CompilationUnitNode* node) {
    printIndent();
    std::cout << node->getNodeType() << "\n";
    ++indentLevel;
    return true;
}

void ASTPrinter::leave(CompilationUnitNode* node) {
    --indentLevel;
}

bool ASTPrinter::visit(PackageDeclarationNode* node) {
    printIndent();
    std::cout << node->getNodeType() << "\n";
    ++indentLevel;
    return true;
}

void ASTPrinter::leave(PackageDeclarationNode* node) {
    --indentLevel;
}

bool ASTPrinter::visit(ImportDeclarationNode* node) {
    printIndent();
    std::cout << node->getNodeType() << " - isWild: " << node->isWildcard << "\n";
    ++indentLevel;
    return true;
}

void ASTPrinter::leave(ImportDeclarationNode* node) {
    --indentLevel;
}

bool ASTPrinter::visit(TypeDeclarationNode* node) {
    printIndent();
    std::cout << node->getNodeType() << " - isInterface: " << (node->isInterface ? "true" : "false") << "\n";
    ++indentLevel;
    return true;
}

void ASTPrinter::leave(TypeDeclarationNode* node) {
    --indentLevel;
}

bool ASTPrinter::visit(BodyDeclarationNode* node) {
    printIndent();
    std::cout << node->getNodeType() << "\n";
    ++indentLevel;
    return true;
}

void ASTPrinter::leave(BodyDeclarationNode* node) {
    --indentLevel;
}

bool ASTPrinter::visit(FieldDeclarationNode* node) {
    printIndent();
    std::cout << node->getNodeType() << "\n";
    ++indentLevel;
    return true;
}

void ASTPrinter::leave(FieldDeclarationNode* node) {
    --indentLevel;
}

bool ASTPrinter::visit(MethodDeclarationNode* node) {
    printIndent();
    std::cout << node->getNodeType() << "\n";
    ++indentLevel;
    return true;
}

void ASTPrinter::leave(MethodDeclarationNode* node) {
    --indentLevel;
}

bool ASTPrinter::visit(ConstructorDeclarationNode* node) {
    printIndent();
    std::cout << node->getNodeType() << "\n";
    ++indentLevel;
    return true;
}

void ASTPrinter::leave(ConstructorDeclarationNode* node) {
    --indentLevel;
}

// Variable Declarations

bool ASTPrinter::visit(VariableDeclarationNode* node) {
    printIndent();
    std::cout << node->getNodeType() << "\n";
    ++indentLevel;
    return true;
}

void ASTPrinter::leave(VariableDeclarationNode* node) {
    --indentLevel;
}

bool ASTPrinter::visit(SingleVariableDeclarationNode* node) {
    printIndent();
    std::cout << node->getNodeType() << "\n";
    ++indentLevel;
    return true;
}

void ASTPrinter::leave(SingleVariableDeclarationNode* node) {
    --indentLevel;
}

bool ASTPrinter::visit(VariableDeclarationFragmentNode* node) {
    printIndent();
    std::cout << node->getNodeType() << "\n";
    ++indentLevel;
    return true;
}

void ASTPrinter::leave(VariableDeclarationFragmentNode* node) {
    --indentLevel;
}

// Statements

bool ASTPrinter::visit(StatementNode* node) {
    printIndent();
    std::cout << node->getNodeType() << "\n";
    ++indentLevel;
    return true;
}

void ASTPrinter::leave(StatementNode* node) {
    --indentLevel;
}

bool ASTPrinter::visit(BlockNode* node) {
    printIndent();
    std::cout << node->getNodeType() << "\n";
    ++indentLevel;
    return true;
}

void ASTPrinter::leave(BlockNode* node) {
    --indentLevel;
}

bool ASTPrinter::visit(EmptyStatementNode* node) {
    printIndent();
    std::cout << node->getNodeType() << "\n";
    ++indentLevel;
    return true;
}

void ASTPrinter::leave(EmptyStatementNode* node) {
    --indentLevel;
}

bool ASTPrinter::visit(ExpressionStatementNode* node) {
    printIndent();
    std::cout << node->getNodeType() << "\n";
    ++indentLevel;
    return true;
}

void ASTPrinter::leave(ExpressionStatementNode* node) {
    --indentLevel;
}

bool ASTPrinter::visit(ReturnStatementNode* node) {
    printIndent();
    std::cout << node->getNodeType() << "\n";
    ++indentLevel;
    return true;
}

void ASTPrinter::leave(ReturnStatementNode* node) {
    --indentLevel;
}

bool ASTPrinter::visit(IfStatementNode* node) {
    printIndent();
    std::cout << node->getNodeType() << "\n";
    ++indentLevel;
    return true;
}

void ASTPrinter::leave(IfStatementNode* node) {
    --indentLevel;
}

bool ASTPrinter::visit(WhileStatementNode* node) {
    printIndent();
    std::cout << node->getNodeType() << "\n";
    ++indentLevel;
    return true;
}

void ASTPrinter::leave(WhileStatementNode* node) {
    --indentLevel;
}

bool ASTPrinter::visit(ForStatementNode* node) {
    printIndent();
    std::cout << node->getNodeType() << "\n";
    ++indentLevel;
    return true;
}

void ASTPrinter::leave(ForStatementNode* node) {
    --indentLevel;
}

bool ASTPrinter::visit(VariableDeclarationStatementNode* node) {
    printIndent();
    std::cout << node->getNodeType() << "\n";
    ++indentLevel;
    return true;
}

void ASTPrinter::leave(VariableDeclarationStatementNode* node) {
    --indentLevel;
}

// Expressions

bool ASTPrinter::visit(ExpressionNode* node) {
    printIndent();
    std::cout << node->getNodeType() << "\n";
    ++indentLevel;
    return true;
}

void ASTPrinter::leave(ExpressionNode* node) {
    --indentLevel;
}

bool ASTPrinter::visit(AssignmentExpressionNode* node) {
    printIndent();
    std::cout << node->getNodeType() << "\n";
    ++indentLevel;
    return true;
}

void ASTPrinter::leave(AssignmentExpressionNode* node) {
    --indentLevel;
}

bool ASTPrinter::visit(InfixExpressionNode* node) {
    printIndent();
    std::cout << node->getNodeType() << " - " << node->op <<  "\n";
    ++indentLevel;
    return true;
}

void ASTPrinter::leave(InfixExpressionNode* node) {
    --indentLevel;
}

bool ASTPrinter::visit(InstanceofExpressionNode* node) {
    printIndent();
    std::cout << node->getNodeType() << "\n";
    ++indentLevel;
    return true;
}

void ASTPrinter::leave(InstanceofExpressionNode* node) {
    --indentLevel;
}

bool ASTPrinter::visit(PrefixExpressionNode* node) {
    printIndent();
    std::cout << node->getNodeType() << " - " << node->op << "\n";
    ++indentLevel;
    return true;
}

void ASTPrinter::leave(PrefixExpressionNode* node) {
    --indentLevel;
}

bool ASTPrinter::visit(CastExpressionNode* node) {
    printIndent();
    std::cout << node->getNodeType() << "\n";
    ++indentLevel;
    return true;
}

void ASTPrinter::leave(CastExpressionNode* node) {
    --indentLevel;
}

bool ASTPrinter::visit(VariableDeclarationExpressionNode* node) {
    printIndent();
    std::cout << node->getNodeType() << "\n";
    ++indentLevel;
    return true;
}

void ASTPrinter::leave(VariableDeclarationExpressionNode* node) {
    --indentLevel;
}

// Types

bool ASTPrinter::visit(TypeNode* node) {
    printIndent();
    std::cout << node->getNodeType() << "\n";
    ++indentLevel;
    return true;
}

void ASTPrinter::leave(TypeNode* node) {
    --indentLevel;
}

bool ASTPrinter::visit(PrimitiveTypeNode* node) {
    printIndent();
    std::cout << node->getNodeType() << " - kind: " << node->kind << "\n";
    ++indentLevel;
    return true;
}

void ASTPrinter::leave(PrimitiveTypeNode* node) {
    --indentLevel;
}

bool ASTPrinter::visit(ClassOrInterfaceTypeNode* node) {
    printIndent();
    std::cout << node->getNodeType() << "\n";
    ++indentLevel;
    return true;
}

void ASTPrinter::leave(ClassOrInterfaceTypeNode* node) {
    --indentLevel;
}

bool ASTPrinter::visit(ArrayTypeNode* node) {
    printIndent();
    std::cout << node->getNodeType() << "\n";
    ++indentLevel;
    return true;
}

void ASTPrinter::leave(ArrayTypeNode* node) {
    --indentLevel;
}

// Literals and Other Expressions

bool ASTPrinter::visit(ArrayCreationExpressionNode* node) {
    printIndent();
    std::cout << node->getNodeType() << "\n";
    ++indentLevel;
    return true;
}

void ASTPrinter::leave(ArrayCreationExpressionNode* node) {
    --indentLevel;
}

bool ASTPrinter::visit(IntegerLiteralNode* node) {
    printIndent();
    std::cout << node->getNodeType() << " - value: " << node->value << "\n";
    ++indentLevel;
    return true;
}

void ASTPrinter::leave(IntegerLiteralNode* node) {
    --indentLevel;
}

bool ASTPrinter::visit(BooleanLiteralNode* node) {
    printIndent();
    std::cout << node->getNodeType() << " - value: " << node->value << "\n";
    ++indentLevel;
    return true;
}

void ASTPrinter::leave(BooleanLiteralNode* node) {
    --indentLevel;
}

bool ASTPrinter::visit(CharacterLiteralNode* node) {
    printIndent();
    std::cout << node->getNodeType() << " - value: " << node->value << "\n";
    ++indentLevel;
    return true;
}

void ASTPrinter::leave(CharacterLiteralNode* node) {
    --indentLevel;
}

bool ASTPrinter::visit(StringLiteralNode* node) {
    printIndent();
    std::cout << node->getNodeType() << " - value: ";
    for (unsigned char c : node->value) {
        std::cout << c;
    }
    std::cout << "\n";
    ++indentLevel;
    return true;
}

void ASTPrinter::leave(StringLiteralNode* node) {
    --indentLevel;
}

bool ASTPrinter::visit(NullLiteralNode* node) {
    printIndent();
    std::cout << node->getNodeType() << "\n";
    ++indentLevel;
    return true;
}

void ASTPrinter::leave(NullLiteralNode* node) {
    --indentLevel;
}

bool ASTPrinter::visit(ThisExpressionNode* node) {
    printIndent();
    std::cout << node->getNodeType() << "\n";
    ++indentLevel;
    return true;
}

void ASTPrinter::leave(ThisExpressionNode* node) {
    --indentLevel;
}

bool ASTPrinter::visit(ParenthesizedExpressionNode* node) {
    printIndent();
    std::cout << node->getNodeType() << "\n";
    ++indentLevel;
    return true;
}

void ASTPrinter::leave(ParenthesizedExpressionNode* node) {
    --indentLevel;
}

bool ASTPrinter::visit(FieldAccessExpressionNode* node) {
    printIndent();
    std::cout << node->getNodeType() << "\n";
    ++indentLevel;
    return true;
}

void ASTPrinter::leave(FieldAccessExpressionNode* node) {
    --indentLevel;
}

bool ASTPrinter::visit(MethodInvocationExpressionNode* node) {
    printIndent();
    std::cout << node->getNodeType() << "\n";
    if (node->resolvedType) {
        printIndent();
        std::cout << " ^- (resolved type: " << node->resolvedType->name << ")\n";
    }
    if (node->resolvedDeclaration) {
        printIndent();
        std::cout << " ^- (resolved declaration: " << node->resolvedDeclaration << ")\n";
    }
    ++indentLevel;
    return true;
}

void ASTPrinter::leave(MethodInvocationExpressionNode* node) {
    --indentLevel;
}

bool ASTPrinter::visit(InstanceCreationExpressionNode* node) {
    printIndent();
    std::cout << node->getNodeType() << "\n";
    ++indentLevel;
    return true;
}

void ASTPrinter::leave(InstanceCreationExpressionNode* node) {
    --indentLevel;
}

bool ASTPrinter::visit(ArrayAccessExpressionNode* node) {
    printIndent();
    std::cout << node->getNodeType() << "\n";
    ++indentLevel;
    return true;
}

void ASTPrinter::leave(ArrayAccessExpressionNode* node) {
    --indentLevel;
}

// Names

bool ASTPrinter::visit(NameNode* node) {
    printIndent();
    std::cout << node->getNodeType() << " - Variant:" << node->variant << "\n";
    ++indentLevel;
    return true;
}

void ASTPrinter::leave(NameNode* node) {
    --indentLevel;
}

bool ASTPrinter::visit(SimpleNameNode* node) {
    printIndent();
    std::cout << "SimpleName '" << node->value << "' - variant: " << node->variant << "\n";
    if (node->resolvedReference) {
        printIndent();
        std::cout << " ^- (resolved reference: " << node->resolvedReference->getNodeType() << ")\n";
    }
    if (node->resolvedType) {
        printIndent();
        std::cout << " ^- (resolved type: " << node->resolvedType->name << ")\n";
    }
    ++indentLevel;
    return true;
}

void ASTPrinter::leave(SimpleNameNode* node) {
    --indentLevel;
}

bool ASTPrinter::visit(QualifiedNameNode* node) {
    printIndent();
    std::cout << node->getNodeType() << " '" << node->getSimpleName() << "' - variant: " << node->variant << "\n";
    if (node->resolvedReference) {
        printIndent();
        std::cout << " ^- (resolved reference: " << node->resolvedReference->getNodeType() << ")\n";
    }
    if (node->resolvedType) {
        printIndent();
        std::cout << " ^- (resolved type: " << node->resolvedType->name << ")\n";
    }
    ++indentLevel;
    return true;
}

void ASTPrinter::leave(QualifiedNameNode* node) {
    --indentLevel;
}

// Modifiers

bool ASTPrinter::visit(ModifierNode* node) {
    printIndent();
    std::cout << node->getNodeType() << " - Kind: " << node->kind << "\n";
    ++indentLevel;
    return true;
}

void ASTPrinter::leave(ModifierNode* node) {
    --indentLevel;
}

ASTPrinter ASTPrinter::Instance{};

}  // namespace ast
