#include "semantics/ast_parser.hpp"
#include "common/parse_tree.hpp"
#include "ast/ast_node.hpp"
#include "ast/nodes.hpp"
#include <iostream>
#include <sstream>
#include <stdexcept>

namespace semantics {
// TODO: Add variant handling to all calls to parseName

CompilationUnitNode* ASTParser::parseCompilationUnit(const common::ParseTree& node) {
    if (node.kind != "CompilationUnit") {
        node.print();
        throw std::runtime_error("Expected CompilationUnit node");
    }
    CompilationUnitNode* compilationUnit = ast->create<CompilationUnitNode>();

    if (auto packageDeclNode = node.children[0]->findOne("PackageDecl")) {
        // PackageDecl
        compilationUnit->packageDeclaration = parsePackageDeclaration(*packageDeclNode);
        compilationUnit->packageDeclaration->parent = compilationUnit;
        enterNamedGlobalScope(compilationUnit->packageDeclaration->name);
        // std::cerr << "In package: " << compilationUnit->packageDeclaration->name->toString() << " with scope " << currentScope << "\n";
    } else {
        enterNamedGlobalScope(nullptr); // Default unnamed global scope for classes without package
        // std::cerr << "In package: " << "default" << " with scope " << currentScope << "\n";
    }

    enterScope();
    compilationUnit->scope = currentScope;

    for (auto importDecl : node.children[1]->findAll("ImportDecl", "", false)) {
        // ImportDecl
        compilationUnit->importDeclarations.push_back(parseImportDeclaration(*importDecl));
        compilationUnit->importDeclarations.back()->parent = compilationUnit;
    }
    for (auto typeDecl : node.children[2]->findAll("TypeDecl", "", false)) {
        // TypeDecl
        compilationUnit->typeDeclarations.push_back(parseTypeDeclaration(*typeDecl));
        compilationUnit->typeDeclarations.back()->parent = compilationUnit;
    }
    
    exitScope();

    return compilationUnit;
}

PackageDeclarationNode* ASTParser::parsePackageDeclaration(const common::ParseTree& node) {
    if (node.kind != "PackageDecl") {
        throw std::runtime_error("Expected PackageDecl node");
    }
    auto nameNode = node.findOne("Name");
    if (!nameNode) {
        node.print();
        throw std::runtime_error("PackageDecl node missing Name child");
    }
    PackageDeclarationNode* packageDecl = ast->create<PackageDeclarationNode>();
    packageDecl->name = parseName(*nameNode, NameNode::Package);
    packageDecl->name->parent = packageDecl;
    // PackageDeclarationNode has no associated scope because it doesn't really make sense

    return packageDecl;
}

ImportDeclarationNode* ASTParser::parseImportDeclaration(const common::ParseTree& node) {
    if (node.kind != "ImportDecl") {
        node.print();
        throw std::runtime_error("Expected ImportDecl node");
    }
    auto nameNode = node.findOne("Name");
    if (!nameNode) {
        node.print();
        throw std::runtime_error("ImportDecl node missing Name child");
    }
    ImportDeclarationNode* importDecl = ast->create<ImportDeclarationNode>();
    importDecl->isWildcard = node.children.size() == 5 && node.children[3]->kind == "STAR"; // Check if it ends with *
    importDecl->name = parseName(*nameNode, importDecl->isWildcard ? NameNode::Package : NameNode::Type);
    importDecl->name->parent = importDecl;

    // Declaration of the types within the scopes is left for later
    // We leave that for linking to account for wildcards

    importDecl->scope = currentScope;

    return importDecl;
}

TypeDeclarationNode* ASTParser::parseTypeDeclaration(const common::ParseTree& node) {
    if (node.kind != "TypeDecl") {
        node.print();
        throw std::runtime_error("Expected TypeDecl node");
    }
    TypeDeclarationNode* typeDecl = ast->create<TypeDeclarationNode>();
    // Class or Interface
    if (auto classNode = node.findOne("ClassDecl")) {
        typeDecl->isInterface = false;
        for (auto modifierNode : classNode->children[0]->findAll("Modifier", "", false)) {
            typeDecl->modifiers.push_back(parseModifier(*modifierNode));
            typeDecl->modifiers.back()->parent = typeDecl;
        }
        auto simpleNameNode = classNode->children[2].get();
        if (!simpleNameNode) {
            classNode->print();
            throw std::runtime_error("ClassDecl node missing SimpleName child");
        }
        typeDecl->name = parseSimpleName(*simpleNameNode);
        typeDecl->name->variant = NameNode::RawIdentifier; // Set variant to RawIdentifier for class names
        typeDecl->name->parent = typeDecl;

        // Within package - currently in the import scope, but should declare in package scope
        // STAGE 1: No class name collisions allowed, so declare in current scope and check for duplicates
        if (!currentScope->parent->declare(typeDecl->name->getSimpleName(), typeDecl)) {
            std::cerr << "Error: Duplicate class declaration " << typeDecl->name->getSimpleName() << "\n";
            std::exit(42); // Joos requirement for error
        }
        // std::cerr << "Declared type: " << typeDecl->name->toString() << "(" << typeDecl << ")" << " in scope " << currentScope->parent << "\n";
        
        typeDecl->scope = currentScope;

        // Extends
        for (auto type : classNode->children[3]->findAll("ClassType", "", false)) {
            typeDecl->extends.push_back(parseClassOrInterfaceType(*type));
            typeDecl->extends.back()->parent = typeDecl;
        }

        // Implements
        for (auto type : classNode->children[4]->findAll("InterfaceType", "", false)) {
            typeDecl->implements.push_back(parseClassOrInterfaceType(*type));
            typeDecl->implements.back()->parent = typeDecl;
        }

        enterScope(); // Enter class scope for body declarations
        // Body Declarations
        typeDecl->bodyScope = currentScope;
        for (auto bodyDecl : classNode->children[5]->findAll("ClassBodyDecl", "", false)) {
            typeDecl->bodyDeclarations.push_back(parseBodyDeclaration(*bodyDecl));
            typeDecl->bodyDeclarations.back()->parent = typeDecl;
        }
        exitScope(); // Exit class scope after parsing body declarations

    } else if (auto interfaceNode = node.findOne("InterfaceDecl")) {
        typeDecl->isInterface = true;
        // children[0] is ModifiersOpt
        for (auto modifierNode : interfaceNode->children[0]->findAll("Modifier", "", false)) {
            typeDecl->modifiers.push_back(parseModifier(*modifierNode));
            typeDecl->modifiers.back()->parent = typeDecl;
        }
        auto simpleNameNode = interfaceNode->children[2].get();
        if (!simpleNameNode) {
            interfaceNode->print();
            throw std::runtime_error("InterfaceDecl node missing SimpleName child");
        }
        typeDecl->name = parseSimpleName(*simpleNameNode);
        typeDecl->name->variant = NameNode::RawIdentifier; // Set variant to Type for interface names
        typeDecl->name->parent = typeDecl;

        // Within package - currently in the import scope, but should declare in package scope
        currentScope->parent->declare(typeDecl->name->getSimpleName(), typeDecl);
        typeDecl->scope = currentScope;

        // Extends - children[3] is ExtendsIntOpt
        for (auto type : interfaceNode->children[3]->findAll("InterfaceType", "", false)) {
            typeDecl->extends.push_back(parseClassOrInterfaceType(*type));
            typeDecl->extends.back()->parent = typeDecl;
        }

        enterScope(); // Enter interface scope for body declarations
        // Body Declarations - children[4] is InterfaceBody
        typeDecl->bodyScope = currentScope; // Set body scope for resolving member accesses
        for (auto bodyDecl : interfaceNode->children[4]->findAll("InterfaceBodyDecl", "", false)) {
            typeDecl->bodyDeclarations.push_back(parseBodyDeclaration(*bodyDecl));
            typeDecl->bodyDeclarations.back()->parent = typeDecl;
        }
        exitScope(); // Exit interface scope after parsing body declarations

    } else {
        node.print();
        throw std::runtime_error("TypeDecl node missing ClassDecl or InterfaceDecl child");
    }
    return typeDecl;
}

BodyDeclarationNode* ASTParser::parseBodyDeclaration(const common::ParseTree& node) {
    if (node.kind != "ClassBodyDecl" && node.kind != "InterfaceBodyDecl") {
        node.print();
        throw std::runtime_error("Expected ClassBodyDecl or InterfaceBodyDecl node");
    }
    if (node.kind == "ClassBodyDecl") {
        if (node.children[0]->kind == "ClassMemberDecl") {
            auto memberDeclNode = node.children[0].get();
            if (memberDeclNode->children[0]->kind == "FieldDecl") {
                return parseFieldDeclaration(*memberDeclNode->children[0]);
            } else if (memberDeclNode->children[0]->kind == "MethodDecl") {
                return parseMethodDeclaration(*memberDeclNode->children[0]);
            } else {
                node.print();
                throw std::runtime_error("ClassMemberDecl node has unrecognized child");
            }
        } else if (node.children[0]->kind == "ConstructorDecl") {
            return parseConstructorDeclaration(*node.children[0]);
        } else {
            node.print();
            throw std::runtime_error("ClassBodyDecl node has unrecognized child");
        }
    } else if (node.kind == "InterfaceBodyDecl") {
        // children[0] is AbstractMethodDecl
        if (node.children[0]->kind == "AbstractMethodDecl") {
            return parseMethodDeclaration(*node.children[0]);
        } else {
            node.print();
            throw std::runtime_error("InterfaceBodyDecl node has unrecognized child");
        }
    } else {
        node.print();
        throw std::runtime_error("Expected ClassBodyDecl or InterfaceBodyDecl node");
    }
}

FieldDeclarationNode* ASTParser::parseFieldDeclaration(const common::ParseTree& node) {
    if (node.kind != "FieldDecl") {
        node.print();
        throw std::runtime_error("Expected FieldDecl node");
    }
    FieldDeclarationNode* fieldDecl = ast->create<FieldDeclarationNode>();
    // Modifiers
    for (auto modifierNode : node.children[0]->findAll("Modifier", "", false)) {
        fieldDecl->modifiers.push_back(parseModifier(*modifierNode));
        fieldDecl->modifiers.back()->parent = fieldDecl;
    }

    fieldDecl->type = parseType(*node.children[1]);
    fieldDecl->type->parent = fieldDecl;
    fieldDecl->scope = currentScope; // Field declarations are in the class scope, not the package scope

    // VariableDeclarators
    for (auto varDecl : node.children[2]->findAll("VarDeclarator", "", false)) {
        fieldDecl->fragments.push_back(parseVariableDeclarationFragment(*varDecl));
        fieldDecl->fragments.back()->parent = fieldDecl;
    }
    return fieldDecl;
}

MethodDeclarationNode* ASTParser::parseMethodDeclaration(const common::ParseTree& node) {
    if (node.kind != "MethodDecl" && node.kind != "AbstractMethodDecl") {
        node.print();
        throw std::runtime_error("Expected MethodDecl or AbstractMethodDecl node");
    }

    MethodDeclarationNode* methodDecl = ast->create<MethodDeclarationNode>();
    
    const common::ParseTree* headerNode = node.children[0].get();
    const common::ParseTree* bodyNode = nullptr;
    if (node.kind == "MethodDecl" && node.children.size() > 1) {
        bodyNode = node.children[1].get();
    }

    // Modifiers
    for (auto modifierNode : headerNode->children[0]->findAll("Modifier", "", false)) {
        methodDecl->modifiers.push_back(parseModifier(*modifierNode));
        methodDecl->modifiers.back()->parent = methodDecl;
    }

    if (headerNode->children[1]->kind == "Type") {
        methodDecl->returnType = parseType(*headerNode->children[1]);
        methodDecl->returnType->parent = methodDecl;
    } else {
        methodDecl->returnType = nullptr; // void return type
    }

    const common::ParseTree* methodDeclaratorNode = headerNode->children[2].get();
    if (!methodDeclaratorNode) {
        node.print();
        throw std::runtime_error("MethodDecl node missing MethodDeclarator child");
    }

    const common::ParseTree* simpleNameNode = methodDeclaratorNode->children[0].get();
    if (!simpleNameNode) {
        node.print();
        throw std::runtime_error("MethodDeclarator node missing SimpleName child");
    }

    methodDecl->name = parseSimpleName(*simpleNameNode);
    methodDecl->name->variant = NameNode::RawIdentifier; // Set variant to RawIdentifier for method names
    methodDecl->name->parent = methodDecl;
    methodDecl->scope = currentScope; // Method declarations are in the class scope, not the package scope
    
    currentScope->declare(methodDecl->name->getSimpleName(), methodDecl);

    enterScope(); // Enter method scope for parameters and body
    // Parameters - children[2] should be FormalParaListOpt
    for (auto param : methodDeclaratorNode->children[2]->findAll("FormalPara", "", false)) {
        methodDecl->parameters.push_back(parseSingleVariableDeclaration(*param));
        methodDecl->parameters.back()->parent = methodDecl;
    }

    // Body - only for MethodDecl, not AbstractMethodDecl
    if (node.kind == "MethodDecl") {
        if (bodyNode->children[0]->kind == "Block") {
            methodDecl->body = parseBlock(*bodyNode->children[0]);
            methodDecl->body->parent = methodDecl;
        } else {
            // Methodbody resolves to SEMI
            methodDecl->body = nullptr; // Bodyless method (e.g., abstract method)
        }
    } else {
        methodDecl->body = nullptr; // Abstract method has no body
    }
    exitScope(); // Exit method scope after parsing body
    return methodDecl;
}

ConstructorDeclarationNode* ASTParser::parseConstructorDeclaration(const common::ParseTree& node) {
    if (node.kind != "ConstructorDecl") {
        node.print();
        throw std::runtime_error("Expected ConstructorDecl node");
    }
    ConstructorDeclarationNode* constructorDecl = ast->create<ConstructorDeclarationNode>();
    // Modifiers
    for (auto modifierNode : node.children[0]->findAll("Modifier", "", false)) {
        constructorDecl->modifiers.push_back(parseModifier(*modifierNode));
        constructorDecl->modifiers.back()->parent = constructorDecl;
    }

    const common::ParseTree* constructorDeclaratorNode = node.children[1].get();
    
    if (!constructorDeclaratorNode) {
        node.print();
        throw std::runtime_error("ConstructorDecl node missing ConstructorDeclarator child");
    }

    const common::ParseTree* simpleNameNode = constructorDeclaratorNode->children[0].get();
    if (!simpleNameNode) {
        node.print();
        throw std::runtime_error("ConstructorDeclarator node missing SimpleName child");
    }

    constructorDecl->name = parseSimpleName(*simpleNameNode);
    constructorDecl->name->variant = NameNode::RawIdentifier; // Set variant to RawIdentifier for constructor names
    constructorDecl->name->parent = constructorDecl;
    constructorDecl->scope = currentScope; // Constructor declarations are in the class scope, not the package scope

    enterScope(); // Enter constructor scope for parameters and body
    // Parameters - children[2] should be FormalParaListOpt
    for (auto param : constructorDeclaratorNode->children[2]->findAll("FormalPara", "", false)) {
        constructorDecl->parameters.push_back(parseSingleVariableDeclaration(*param));
        constructorDecl->parameters.back()->parent = constructorDecl;
    }

    const common::ParseTree* bodyNode = node.children[2].get();
    if (!bodyNode || bodyNode->kind != "ConstructorBody") {
        node.print();
        throw std::runtime_error("ConstructorDecl node missing ConstructorBody child");
    }
    // Body - children[1] should be Block
    if (bodyNode->children[0]->kind == "Block") {
        constructorDecl->body = parseBlock(*bodyNode->children[0]);
        constructorDecl->body->parent = constructorDecl;
    } else {
        node.print();
        throw std::runtime_error("ConstructorBody node missing Block child");
    }
    exitScope(); // Exit constructor scope after parsing body
    return constructorDecl;
}

SingleVariableDeclarationNode* ASTParser::parseSingleVariableDeclaration(const common::ParseTree& node) {
    if (node.kind != "FormalPara") {
        node.print();
        throw std::runtime_error("Expected FormalPara node");
    }
    SingleVariableDeclarationNode* varDecl = ast->create<SingleVariableDeclarationNode>();
    
    // Name
    auto vdi = node.children[1].get();
    if (!vdi || vdi->kind != "VarDeclaratorId") {
        node.print();
        throw std::runtime_error("FormalPara node missing VarDeclaratorId child");
    }

    auto simpleNameNode = vdi->children[0].get();
    if (!simpleNameNode || simpleNameNode->kind != "IDENTIFIER") {
        node.print();
        throw std::runtime_error("VarDeclaratorId node missing IDENTIFIER child");
    }

    varDecl->name = parseSimpleName(*simpleNameNode);
    varDecl->name->parent = varDecl;
    varDecl->name->variant = NameNode::RawIdentifier; // Set variant to RawIdentifier for parameter names
    // Type
    varDecl->type = parseType(*node.children[0]);
    varDecl->type->parent = varDecl;

    // Formal parameters cannot have initializers
    varDecl->initializer = nullptr; 
    varDecl->scope = currentScope;
    if (!currentScope->declare(varDecl->name->getSimpleName(), varDecl)) {
        throw std::runtime_error("Error: Duplicate variable declaration " + varDecl->name->getSimpleName());
    }
    return varDecl;
}

VariableDeclarationFragmentNode* ASTParser::parseVariableDeclarationFragment(const common::ParseTree& node, bool inMethod) {
    if (node.kind != "VarDeclarator") {
        node.print();
        throw std::runtime_error("Expected VarDeclarator node");
    }
    // node.print();
    VariableDeclarationFragmentNode* varDecl = ast->create<VariableDeclarationFragmentNode>();
    // Name
    auto simpleNameNode = node.children[0].get();
    if (!simpleNameNode || simpleNameNode->kind != "VarDeclaratorId") {
        node.print();
        throw std::runtime_error("VarDeclarator node missing VarDeclaratorId child");
    }
    varDecl->name = parseSimpleName(*simpleNameNode->children[0]);
    varDecl->name->variant = NameNode::RawIdentifier; // Set variant to RawIdentifier for variable names
    varDecl->name->parent = varDecl;
    if (inMethod) { // Being done inside a method or constructor header, so need to create a new scope for the variable declaration
        // Also need to check for duplicate localvar declarations
        enterScope(); // Create a new scope
    }
    varDecl->scope = currentScope; // Set scope before parsing initializer

    // STAGE 1: No two local variables with overlapping scopes can have the same name. This search doesn't include parameters since we use VariableDeclarationFragmentNode
    if (inMethod) {
        for (auto node : currentScope->lookup(varDecl->name->getSimpleName(), NodeType::VariableDeclarationFragment)) {
            if (node->parent->getNodeType() == NodeType::FieldDeclaration) {
                continue; // those can be shadowed
            }
            std::cerr << "Error: Duplicate local variable declaration " << varDecl->name->getSimpleName() << "\n";
            std::exit(42); // Joos requirement for error
        }
        if (!currentScope->lookup(varDecl->name->getSimpleName(), NodeType::SingleVariableDeclaration).empty()) {
            std::cerr << "Error: Local variable declaration for " << varDecl->name->getSimpleName() << " shadows a parameter\n";
            std::exit(42); // Joos requirement for error
        }
    }

    // STAGE 1: No two fields in the same class can have the same name. Since declare only checks the current scope, this applies to both fields and locals
    if (!currentScope->declare(varDecl->name->getSimpleName(), varDecl)) {
        std::cerr << "Error: Duplicate field declaration " << varDecl->name->getSimpleName() << "\n";
        std::exit(42); // Joos requirement for error
    } else {
        // std::cerr << "Declared variable: " << varDecl->name->toString() << " in scope " << currentScope << "\n";
    }


    // Initializer
    if (node.children.size() > 1 && node.children[1]->kind == "ASSIGN") {
        auto initializerNode = node.children[2].get();
        if (!initializerNode) {
            node.print();
            throw std::runtime_error("VarDeclarator node has ASSIGN but missing initializer expression");
        }
        varDecl->initializer = parseExpression(*initializerNode->children[0]);
        varDecl->initializer->parent = varDecl;
    } else {
        varDecl->initializer = nullptr;
    }
    varDecl->scope = currentScope;
    return varDecl;
}

StatementNode* ASTParser::parseStatement(const common::ParseTree& node, int& newScopeCount) {
    if (node.kind != "Statement" && node.kind != "LocalVarDeclStat") {
        node.print();
        throw std::runtime_error("Expected Statement node");
    }
    newScopeCount = 0; // Initialize newScopeCount to 0 for non-variable declaration statements
    if (node.kind == "LocalVarDeclStat") {
        return parseVariableDeclarationStatement(node, newScopeCount);
    }

    const common::ParseTree* statementNode = node.children[0].get();
    std::string statementKind = statementNode->kind;
    if (statementKind == "IfThenStat" || statementKind == "IfThenElseStat") {
        return parseIfStatement(*statementNode);
    } else if (statementKind == "WhileStat") {
        return parseWhileStatement(*statementNode);
    } else if (statementKind == "ForStat") {
        return parseForStatement(*statementNode);
    } else if (statementKind == "StatementNoTrail") {
        const common::ParseTree* noTrailNode = statementNode->children[0].get();
        std::string noTrailKind = noTrailNode->kind;
        if (noTrailKind == "Block") {
            return parseBlock(*statementNode->children[0]);
        } else if (noTrailKind == "EmptyStat") {
            return parseEmptyStatement(*statementNode->children[0]);
        } else if (noTrailKind == "ExprStat") {
            return parseExpressionStatement(*statementNode->children[0]);
        } else if (noTrailKind == "ReturnStat") {
            return parseReturnStatement(*statementNode->children[0]);
        } else {
            node.print();
            throw std::runtime_error("Unrecognized StatementNoTrail kind: " + noTrailKind);
        }
    } else {
        node.print();
        throw std::runtime_error("Unrecognized Statement kind: " + statementKind);
    }
}

StatementNode* ASTParser::parseStatementNoShortIf(const common::ParseTree& node) {
    if (node.kind != "StatementNoShortIf") {
        node.print();
        throw std::runtime_error("Expected StatementNoShortIf node");
    }
    const common::ParseTree* statementNode = node.children[0].get();
    std::string statementKind = statementNode->kind;
    if (statementKind == "IfThenElseStatNoShortIf") {
        return parseIfStatement(*statementNode);
    } else if (statementKind == "WhileStatNoShortIf") {
        return parseWhileStatement(*statementNode);
    } else if (statementKind == "ForStatNoShortIf") {
        return parseForStatement(*statementNode);
    } else if (statementKind == "StatementNoTrail") {
        const common::ParseTree* noTrailNode = statementNode->children[0].get();
        std::string noTrailKind = noTrailNode->kind;
        if (noTrailKind == "Block") {
            return parseBlock(*noTrailNode);
        } else if (noTrailKind == "EmptyStat") {
            return parseEmptyStatement(*noTrailNode);
        } else if (noTrailKind == "ExprStat") {
            return parseExpressionStatement(*noTrailNode);
        } else if (noTrailKind == "ReturnStat") {
            return parseReturnStatement(*noTrailNode);
        } else {
            node.print();
            throw std::runtime_error("Unrecognized StatementNoTrail kind: " + noTrailKind);
        }
    } else {
        node.print();
        throw std::runtime_error("Unrecognized StatementNoShortIf kind: " + statementKind);
    }
}

BlockNode* ASTParser::parseBlock(const common::ParseTree& node) {
    if (node.kind != "Block") {
        node.print();
        throw std::runtime_error("Expected Block node");
    }
    BlockNode* block = ast->create<BlockNode>();
    block->scope = currentScope; // Block shares the same scope as its parent statement
    enterScope(); // Enter block scope for local variable declarations
    int localVarDecls = 0;
    // std::cerr << "Block\n";
    for (auto child : node.findAll("BlockStatOne", "", false)) {
        // child->print();
        int thisStatementVarDecls = 0;
        block->statements.push_back(parseStatement(*child->children[0], thisStatementVarDecls));
        localVarDecls += thisStatementVarDecls;
        block->statements.back()->parent = block;
    }
    for (int i = 0; i < localVarDecls; i++) {
        exitScope(); // Exit block scope for each local variable declaration to ensure correct scoping rules
    }
    exitScope();
    return block;
}

EmptyStatementNode* ASTParser::parseEmptyStatement(const common::ParseTree& node) {
    if (node.kind != "EmptyStat") {
        node.print();
        throw std::runtime_error("Expected EmptyStat node");
    }
    auto ret =  ast->create<EmptyStatementNode>();
    ret->scope = currentScope;
    return ret;
}

ExpressionStatementNode* ASTParser::parseExpressionStatement(const common::ParseTree& node) {
    if (node.kind != "ExprStat") {
        node.print();
        throw std::runtime_error("Expected ExprStat node");
    }
    ExpressionStatementNode* stmt = ast->create<ExpressionStatementNode>();
    stmt->scope = currentScope;
    
    const common::ParseTree* statexpr = node.children[0].get();
    if (!statexpr || statexpr->kind != "StatExpr") {
        node.print();
        throw std::runtime_error("ExprStat node missing StatExpr child");
    }

    const common::ParseTree* exprNode = statexpr->children[0].get();
    if (!exprNode) {
        node.print();
        throw std::runtime_error("StatExpr node missing expression child");
    }

    if (exprNode->kind == "Assign") {
        stmt->expression = parseAssignmentExpression(*exprNode);
    } else if (exprNode->kind == "MethodCall") {
        stmt->expression = parseMethodInvocationExpression(*exprNode);
    } else if (exprNode->kind == "ClassCreate") {
        stmt->expression = parseInstanceCreationExpression(*exprNode);
    } else {
        node.print();
        throw std::runtime_error("Unrecognized StatExpr child kind: " + exprNode->kind);
    }
    stmt->expression->parent = stmt;
    return stmt;
}

ReturnStatementNode* ASTParser::parseReturnStatement(const common::ParseTree& node) {
    if (node.kind != "ReturnStat") {
        node.print();
        throw std::runtime_error("Expected ReturnStat node");
    }
    ReturnStatementNode* stmt = ast->create<ReturnStatementNode>();
    stmt->scope = currentScope;
    if (node.children.size() > 2) {
        stmt->expression = parseExpression(*node.children[1]);
        stmt->expression->parent = stmt;
    } else {
        stmt->expression = nullptr; // return with no expression
    }
    return stmt;
}

IfStatementNode* ASTParser::parseIfStatement(const common::ParseTree& node) {
    if (node.kind != "IfThenStat" && node.kind != "IfThenElseStat" && node.kind != "IfThenElseStatNoShortIf") {
        node.print();
        throw std::runtime_error("Expected IfThenStat, IfThenElseStat node");
    }
    int newScopeCount = 0; // Track new scopes created within the if statement for correct scope exiting

    IfStatementNode* stmt = ast->create<IfStatementNode>();
    stmt->scope = currentScope;
    stmt->condition = parseExpression(*node.children[2]);
    if (node.kind == "IfThenStat") {
        enterScope();
        stmt->thenBranch = parseStatement(*node.children[4], newScopeCount);
        for (int i = 0; i < newScopeCount; i++) { exitScope(); }
        exitScope();
        stmt->elseBranch = nullptr;
    } else if (node.kind == "IfThenElseStat") {
        enterScope();
        stmt->thenBranch = parseStatementNoShortIf(*node.children[4]);
        exitScope();
        enterScope();
        stmt->elseBranch = parseStatement(*node.children[6], newScopeCount);
        for (int i = 0; i < newScopeCount; i++) { exitScope(); }
        exitScope();
    } else if (node.kind == "IfThenElseStatNoShortIf") {
        enterScope();
        stmt->thenBranch = parseStatementNoShortIf(*node.children[4]);
        exitScope();
        enterScope();
        stmt->elseBranch = parseStatementNoShortIf(*node.children[6]);
        exitScope();
    } else {
        node.print();
        throw std::runtime_error("Unrecognized IfStatement kind: " + node.kind);
    }
    stmt->condition->parent = stmt;
    stmt->thenBranch->parent = stmt;
    if (stmt->elseBranch) {
        stmt->elseBranch->parent = stmt;
    }
    return stmt;
}

WhileStatementNode* ASTParser::parseWhileStatement(const common::ParseTree& node) {
    if (node.kind != "WhileStat" && node.kind != "WhileStatNoShortIf") {
        node.print();
        throw std::runtime_error("Expected WhileStat or WhileStatNoShortIf node");
    }
    WhileStatementNode* stmt = ast->create<WhileStatementNode>();
    stmt->scope = currentScope;
    stmt->condition = parseExpression(*node.children[2]);
    int newScopeCount = 0; // Track new scopes created within the while statement for correct scope exiting
    enterScope();
    if (node.kind == "WhileStat") {
        stmt->body = parseStatement(*node.children[4], newScopeCount);
    } else if (node.kind == "WhileStatNoShortIf") {
        stmt->body = parseStatementNoShortIf(*node.children[4]);
    } else {
        node.print();
        throw std::runtime_error("Unrecognized WhileStatement kind: " + node.kind);
    }
    for (int i = 0; i < newScopeCount; i++) { exitScope(); }
    exitScope();
    stmt->condition->parent = stmt;
    stmt->body->parent = stmt;
    return stmt;
}

ForStatementNode* ASTParser::parseForStatement(const common::ParseTree& node) {
    if (node.kind != "ForStat" && node.kind != "ForStatNoShortIf") {
        throw std::runtime_error("Expected ForStat or ForStatNoShortIf node");
    }
    
    ForStatementNode* stmt = ast->create<ForStatementNode>();
    stmt->scope = currentScope;
    // ForInit
    int initScopeCount = 0;
    int bodyScopeCount = 0; // Track new scopes created within the for statement body for correct scope exiting
    const common::ParseTree* forInitNode = node.children[2]->findOne("ForInit");
    if (forInitNode) {
        if (forInitNode->children[0]->kind == "StatExprList") {
            // Expression list
            for (auto expr : forInitNode->children[0]->findAll("StatExpr", "", false)) {
                expr = expr->children[0].get(); // StatExpr has a single child which is the actual expression
                if (expr->kind == "Assign") {
                    stmt->initializers.push_back(parseAssignmentExpression(*expr));
                } else if (expr->kind == "MethodCall") {
                    stmt->initializers.push_back(parseMethodInvocationExpression(*expr));
                } else if (expr->kind == "ClassCreate") {
                    stmt->initializers.push_back(parseInstanceCreationExpression(*expr));
                } else {
                    node.print();
                    throw std::runtime_error("Unrecognized StatExpr child kind: " + expr->kind);
                }
                stmt->initializers.back()->parent = stmt;
            }
        } else if (forInitNode->children[0]->kind == "LocalVarDecl") {
            // Local variable declaration
            stmt->initializers.push_back(parseVariableDeclarationExpression(*forInitNode->children[0], initScopeCount));
            stmt->initializers.back()->parent = stmt;
        } else {
            node.print();
            throw std::runtime_error("Unrecognized ForInit child kind: " + forInitNode->children[0]->kind);
        }
    }

    const common::ParseTree* conditionNode = node.children[4]->findOne("Expression");
    if (conditionNode) {
        stmt->condition = parseExpression(*conditionNode);
        stmt->condition->parent = stmt;
    }

    const common::ParseTree* updateNode = node.children[6]->findOne("StatExprList");
    if (updateNode) {
        for (auto expr : updateNode->findAll("StatExpr", "", false)) {
            expr = expr->children[0].get(); // StatExpr has a single child which is the actual expression
            if (expr->kind == "Assign") {
                stmt->updates.push_back(parseAssignmentExpression(*expr));
            } else if (expr->kind == "MethodCall") {
                stmt->updates.push_back(parseMethodInvocationExpression(*expr));
            } else if (expr->kind == "ClassCreate") {
                stmt->updates.push_back(parseInstanceCreationExpression(*expr));
            } else {
                node.print();
                throw std::runtime_error("Unrecognized StatExpr child kind: " + expr->kind);
            }
            stmt->updates.back()->parent = stmt;
        }
    }
    enterScope(); // Enter for statement scope for the body of the loop
    if (node.kind == "ForStat") {
        stmt->body = parseStatement(*node.children[8], bodyScopeCount);
        initScopeCount += bodyScopeCount; // Add body scope count to init scope count to ensure all scopes are exited correctly
    } else if (node.kind == "ForStatNoShortIf") {
        stmt->body = parseStatementNoShortIf(*node.children[6]);
    } else {
        node.print();
        throw std::runtime_error("Unrecognized ForStatement kind: " + node.kind);
    }

    if (initScopeCount > 0) {
        for (int i = 0; i < initScopeCount; i++) {
            exitScope(); // Exit forinit statement scopes
        }
    }
    exitScope(); // Exit for statement scope after parsing body

    stmt->body->parent = stmt;
    return stmt;
}

VariableDeclarationStatementNode* ASTParser::parseVariableDeclarationStatement(const common::ParseTree& node, int& newScopeCount) {
    if (node.kind != "LocalVarDeclStat") {
        node.print();
        throw std::runtime_error("Expected LocalVarDeclStat node");
    }
    VariableDeclarationStatementNode* stmt = ast->create<VariableDeclarationStatementNode>();

    common::ParseTree* localVarDeclNode = node.children[0].get();
    if (!localVarDeclNode || localVarDeclNode->kind != "LocalVarDecl") {
        node.print();
        throw std::runtime_error("LocalVarDeclStat node missing LocalVarDecl child");
    }
    stmt->type = parseType(*localVarDeclNode->children[0]);
    stmt->type->parent = stmt;
    // std::cerr << "Parsing variable declaration fragment in variable declaration statement\n";
    for (auto varDecl : localVarDeclNode->children[1]->findAll("VarDeclarator", "", false)) {
        
        stmt->fragments.push_back(parseVariableDeclarationFragment(*varDecl, true));
        stmt->fragments.back()->parent = stmt;
    }
    // std::cerr << "Finished parsing variable declaration fragments in variable declaration statement\n";
    newScopeCount = stmt->fragments.size(); // Number of new scopes created for the variable declarations
    return stmt;
}

AssignmentExpressionNode* ASTParser::parseAssignmentExpression(const common::ParseTree& node) {
    if (node.kind != "Assign") {
        node.print();
        throw std::runtime_error("Expected Assign node");
    }

    AssignmentExpressionNode* assignExpr = ast->create<AssignmentExpressionNode>();
    assignExpr->scope = currentScope;
    auto lhsNode = node.children[0].get();
    auto rhsNode = node.children[2].get();
    auto lhsKindNode = lhsNode->children[0].get();
    auto rhsKindNode = rhsNode->children[0].get();
    if (lhsKindNode->kind == "Name") {
        assignExpr->lhs = parseName(*lhsKindNode, NameNode::Expression);
    } else if (lhsKindNode->kind == "FieldAccess") {
        assignExpr->lhs = parseFieldAccessExpression(*lhsKindNode);
    } else if (lhsKindNode->kind == "ArrayAccess") {
        assignExpr->lhs = parseArrayAccessExpression(*lhsKindNode);
    } else {
        node.print();
        throw std::runtime_error("Unrecognized LHS kind in assignment: " + lhsKindNode->kind);
    }
    
    if (rhsKindNode->kind == "Assign") {
        assignExpr->rhs = parseAssignmentExpression(*rhsKindNode);
    } else if (rhsKindNode->kind == "CondOrExpr") {
        assignExpr->rhs = parseInfixExpression(*rhsKindNode, InfixExpressionNode::ConditionalOr);
    } else {
        node.print();
        throw std::runtime_error("Unrecognized RHS kind in assignment: " + rhsKindNode->kind);
    }
    assignExpr->lhs->parent = assignExpr;
    assignExpr->rhs->parent = assignExpr;
    return assignExpr;
}

ExpressionNode* ASTParser::parseExpression(const common::ParseTree& node) {
    if (node.kind != "Expression") {
        node.print();
        throw std::runtime_error("Expected Expression node");
    }

    const common::ParseTree* assignmentNode = node.children[0].get();
    if (assignmentNode && assignmentNode->children[0]->kind == "Assign") {
        return parseAssignmentExpression(*assignmentNode->children[0]);
    }
    const common::ParseTree* conditionalOrNode = assignmentNode->children[0].get();
    return parseInfixExpression(*conditionalOrNode, InfixExpressionNode::ConditionalOr);
}

ExpressionNode* ASTParser::parseInfixExpression(const common::ParseTree& node, InfixExpressionNode::Operator op) {
    InfixExpressionNode* infixExpr;
    switch (op) {
        case InfixExpressionNode::ConditionalOr:
            if (node.kind != "CondOrExpr") {
                node.print();
                throw std::runtime_error("Expected CondOrExpr node");
            }
            if (node.children.size() == 1) {
                return parseInfixExpression(*node.children[0], InfixExpressionNode::ConditionalAnd);
            }
            infixExpr = ast->create<InfixExpressionNode>();
            infixExpr->op = InfixExpressionNode::ConditionalOr;
            infixExpr->lhs = parseInfixExpression(*node.children[0], InfixExpressionNode::ConditionalOr);
            infixExpr->rhs = parseInfixExpression(*node.children[2], InfixExpressionNode::ConditionalAnd);
            break;
        case InfixExpressionNode::ConditionalAnd:
            if (node.kind != "CondAndExpr") {
                node.print();
                throw std::runtime_error("Expected ConditionalAnd node");
            }
            if (node.children.size() == 1) {
                return parseInfixExpression(*node.children[0], InfixExpressionNode::InclusiveOr);
            }
            infixExpr = ast->create<InfixExpressionNode>();
            infixExpr->op = InfixExpressionNode::ConditionalAnd;
            infixExpr->lhs = parseInfixExpression(*node.children[0], InfixExpressionNode::ConditionalAnd);
            infixExpr->rhs = parseInfixExpression(*node.children[2], InfixExpressionNode::InclusiveOr);
            break;
        case InfixExpressionNode::InclusiveOr:
            if (node.kind != "InclOrExpr") {
                node.print();
                throw std::runtime_error("Expected InclOrExpr node");
            }
            if (node.children.size() == 1) {
                return parseInfixExpression(*node.children[0], InfixExpressionNode::And);
            }
            infixExpr = ast->create<InfixExpressionNode>();
            infixExpr->op = InfixExpressionNode::InclusiveOr;
            infixExpr->lhs = parseInfixExpression(*node.children[0], InfixExpressionNode::InclusiveOr);
            infixExpr->rhs = parseInfixExpression(*node.children[2], InfixExpressionNode::And);
            break;
        case InfixExpressionNode::And:
            if (node.kind != "AndExpr") {
                node.print();
                throw std::runtime_error("Expected AndExpr node");
            }
            if (node.children.size() == 1) {
                return parseInfixExpression(*node.children[0], InfixExpressionNode::Equality);
            }
            infixExpr = ast->create<InfixExpressionNode>();
            infixExpr->op = InfixExpressionNode::And;
            infixExpr->lhs = parseInfixExpression(*node.children[0], InfixExpressionNode::And);
            infixExpr->rhs = parseInfixExpression(*node.children[2], InfixExpressionNode::Equality);
            break;
        case InfixExpressionNode::Equality:
            if (node.kind != "EqExpr") {
                node.print();
                throw std::runtime_error("Expected EqExpr node");
            }
            if (node.children.size() == 1) {
                return parseInfixExpression(*node.children[0], InfixExpressionNode::Relational);
            }
            infixExpr = ast->create<InfixExpressionNode>();
            infixExpr->op = node.children[1]->kind == "EQUAL" ? InfixExpressionNode::Equality : InfixExpressionNode::NotEqual;
            infixExpr->lhs = parseInfixExpression(*node.children[0], InfixExpressionNode::Equality);
            infixExpr->rhs = parseInfixExpression(*node.children[2], InfixExpressionNode::Relational);
            break;
        case InfixExpressionNode::Relational:
            if (node.children.size() == 3) {
                if (node.children[1]->kind == "LT") {
                    op = InfixExpressionNode::LessThan;
                } else if (node.children[1]->kind == "GT") {
                    op = InfixExpressionNode::GreaterThan;
                } else if (node.children[1]->kind == "LE") {
                    op = InfixExpressionNode::LessThanOrEqual;
                } else if (node.children[1]->kind == "GE") {
                    op = InfixExpressionNode::GreaterThanOrEqual;
                } else if (node.children[1]->kind == "INSTANCEOF") {
                    return parseInstanceofExpression(node);
                } else {
                    node.print();
                    throw std::runtime_error("Unrecognized relational operator: " + node.children[1]->kind);
                }
            }
        case InfixExpressionNode::LessThan:
        case InfixExpressionNode::GreaterThan:
        case InfixExpressionNode::LessThanOrEqual:
        case InfixExpressionNode::GreaterThanOrEqual:
            if (node.kind != "RelExpr") {
                node.print();
                throw std::runtime_error("Expected RelExpr node");
            }
            if (node.children.size() == 1) {
                return parseInfixExpression(*node.children[0], InfixExpressionNode::Additive);
            }
            infixExpr = ast->create<InfixExpressionNode>();
            infixExpr->op = op;
            infixExpr->lhs = parseInfixExpression(*node.children[0], InfixExpressionNode::Relational);
            infixExpr->rhs = parseInfixExpression(*node.children[2], InfixExpressionNode::Additive);
            break;
        case InfixExpressionNode::Additive:
            if (node.children.size() == 3) {
                if (node.children[1]->kind == "PLUS") {
                    op = InfixExpressionNode::Add;
                } else if (node.children[1]->kind == "MINUS") {
                    op = InfixExpressionNode::Subtract;
                } else {
                    node.print();
                    throw std::runtime_error("Unrecognized additive operator: " + node.children[1]->kind);
                }
            }
        case InfixExpressionNode::Add:
        case InfixExpressionNode::Subtract:
            if (node.kind != "AddExpr") {
                node.print();
                throw std::runtime_error("Expected AddExpr node");
            }
            if (node.children.size() == 1) {
                return parseInfixExpression(*node.children[0], InfixExpressionNode::Multiplicative);
            }
            infixExpr = ast->create<InfixExpressionNode>();
            infixExpr->op = op;
            infixExpr->lhs = parseInfixExpression(*node.children[0], InfixExpressionNode::Additive);
            infixExpr->rhs = parseInfixExpression(*node.children[2], InfixExpressionNode::Multiplicative);
            break;
        case InfixExpressionNode::Multiplicative:
             if (node.children.size() == 3) {
                if (node.children[1]->kind == "STAR") {
                    op = InfixExpressionNode::Multiply;
                } else if (node.children[1]->kind == "SLASH") {
                    op = InfixExpressionNode::Divide;
                } else if (node.children[1]->kind == "PERCENT") {
                    op = InfixExpressionNode::Remainder;
                } else {
                    node.print();
                    throw std::runtime_error("Unrecognized multiplicative operator: " + node.children[1]->kind);
                }
            }
        case InfixExpressionNode::Multiply:
        case InfixExpressionNode::Divide:
        case InfixExpressionNode::Remainder:
            if (node.kind != "MulExpr") {
                node.print();
                throw std::runtime_error("Expected MulExpr node");
            }
            if (node.children.size() == 1) {
                return parsePrefixExpression(*node.children[0]);
            }
            infixExpr = ast->create<InfixExpressionNode>();
            infixExpr->op = op;
            infixExpr->lhs = parseInfixExpression(*node.children[0], InfixExpressionNode::Multiplicative);
            infixExpr->rhs = parsePrefixExpression(*node.children[2]);
            break;
        default:
            node.print();
            throw std::runtime_error("Unrecognized infix operator");
    }
    infixExpr->lhs->parent = infixExpr;
    infixExpr->rhs->parent = infixExpr;
    infixExpr->scope = currentScope;
    return infixExpr;
}

InstanceofExpressionNode* ASTParser::parseInstanceofExpression(const common::ParseTree& node) {
    if (node.kind != "RelExpr") {
        node.print();
        throw std::runtime_error("Expected Relational node for instanceof expression");
    }
    InstanceofExpressionNode* instanceofExpr = ast->create<InstanceofExpressionNode>();
    instanceofExpr->expression = parseInfixExpression(*node.children[0], InfixExpressionNode::Relational);
    instanceofExpr->type = parseReferenceType(*node.children[2]);
    instanceofExpr->expression->parent = instanceofExpr;
    instanceofExpr->type->parent = instanceofExpr;
    instanceofExpr->scope = currentScope;
    return instanceofExpr;
}

ExpressionNode* ASTParser::parsePrefixExpression(const common::ParseTree& node, PrefixExpressionNode::Operator op) {
    if (op == PrefixExpressionNode::Negate) {
        if (node.kind != "UnaryExpr") {
            node.print();
            throw std::runtime_error("Expected UnaryExpr node for prefix expression");
        }
        if (node.children.size() == 1) {
            return parsePrefixExpression(*node.children[0], PrefixExpressionNode::Not); // Move to UnaryExprNotMinus
        }
        ExpressionNode* operand = parsePrefixExpression(*node.children[1], PrefixExpressionNode::Negate);
        if (operand->getNodeType() == NodeType::IntegerLiteral) {
            // Constant folding for negation of integer literals
            IntegerLiteralNode* intLiteral = static_cast<IntegerLiteralNode*>(operand);
            if (intLiteral->rawValue == "2147483648") {
                intLiteral->value = -2147483648; // Handle edge case for negation of Integer.MIN_VALUE
            } else {
                intLiteral->value = -intLiteral->value;
            }
            return intLiteral;
        }
        PrefixExpressionNode* prefixExpr = ast->create<PrefixExpressionNode>();
        prefixExpr->op = PrefixExpressionNode::Negate;
        prefixExpr->operand = operand;
        prefixExpr->operand->parent = prefixExpr;
        prefixExpr->scope = currentScope;
        return prefixExpr;
    } else {
        if (node.kind != "UnaryExprNotMinus") {
            node.print();
            throw std::runtime_error("Expected UnaryExprNotMinus node for prefix expression");
        }
        if (node.children[0]->kind == "PostfixExpr") {
            // Handle postfix directly
            const common::ParseTree* postfixNode = node.children[0].get();
            if (postfixNode->children[0]->kind == "Primary") {
                return parsePrimaryExpression(*postfixNode->children[0]);
            } else if (postfixNode->children[0]->kind == "Name") {
                return parseName(*postfixNode->children[0], NameNode::Expression);
            } else {
                node.print();
                throw std::runtime_error("Unrecognized PostfixExpr child kind: " + postfixNode->children[0]->kind);
            }
        } else if  (node.children[0]->kind == "BANG") {
            PrefixExpressionNode* prefixExpr = ast->create<PrefixExpressionNode>();
            prefixExpr->op = PrefixExpressionNode::Not;
            prefixExpr->operand = parsePrefixExpression(*node.children[1], PrefixExpressionNode::Negate);
            prefixExpr->operand->parent = prefixExpr;
            prefixExpr->scope = currentScope;
            return prefixExpr;
        } else if (node.children[0]->kind == "CastExpr") {
            return parseCastExpression(*node.children[0]);
        } else {
            node.print();
            throw std::runtime_error("Unrecognized UnaryExprNotMinus child kind: " + node.children[0]->kind);
        }
    }
}

ExpressionNode* ASTParser::parsePrimaryExpression(const common::ParseTree& node) {
    if (node.kind != "Primary") {
        node.print();
        throw std::runtime_error("Expected Primary node");
    }
    const common::ParseTree* primaryChildNode = node.children[0].get();
    if (primaryChildNode->kind == "PrimaryNoNewArray") {
        return parsePrimaryNoArrayExpression(*primaryChildNode);
    } else if (primaryChildNode->kind == "ArrayCreationExpr") {
        return parseArrayCreationExpression(*primaryChildNode);
    } else {
        node.print();
        throw std::runtime_error("Unrecognized Primary child kind: " + primaryChildNode->kind);
    }
}

ExpressionNode* ASTParser::parsePrimaryNoArrayExpression(const common::ParseTree& node) {
    if (node.kind != "PrimaryNoNewArray") {
        node.print();
        throw std::runtime_error("Expected PrimaryNoNewArray node");
    }
    const common::ParseTree* primaryNoArrayChildNode = node.children[0].get();
    if (primaryNoArrayChildNode->kind == "LPAREN") {
        ParenthesizedExpressionNode* parenExpr = ast->create<ParenthesizedExpressionNode>();
        parenExpr->expression = parseExpression(*node.children[1]);
        parenExpr->expression->parent = parenExpr;
        parenExpr->scope = currentScope;
        return parenExpr;
    } else if (primaryNoArrayChildNode->kind == "Name") {
        return parseName(*primaryNoArrayChildNode, NameNode::Expression);
    } else if (primaryNoArrayChildNode->kind == "THIS") {
        return parseThisExpression(*primaryNoArrayChildNode);
    } else if (primaryNoArrayChildNode->kind == "ClassCreate") {
        return parseInstanceCreationExpression(*primaryNoArrayChildNode);
    } else if (primaryNoArrayChildNode->kind == "FieldAccess") {
        return parseFieldAccessExpression(*primaryNoArrayChildNode);
    } else if (primaryNoArrayChildNode->kind == "MethodCall") {
        return parseMethodInvocationExpression(*primaryNoArrayChildNode);
    } else if (primaryNoArrayChildNode->kind == "ArrayAccess") {
        return parseArrayAccessExpression(*primaryNoArrayChildNode);
    } else if (primaryNoArrayChildNode->kind == "Literal") {
        const common::ParseTree* literalChild = primaryNoArrayChildNode->children[0].get();
        if (literalChild->kind == "LITERAL_INTEGER") {
            return parseIntegerLiteral(*literalChild);
        } else if (literalChild->kind == "LITERAL_BOOLEAN") {
            return parseBooleanLiteral(*literalChild);
        } else if (literalChild->kind == "LITERAL_CHAR") {
            return parseCharacterLiteral(*literalChild);
        } else if (literalChild->kind == "LITERAL_STRING") {
            return parseStringLiteral(*literalChild);
        } else if (literalChild->kind == "LITERAL_NULL") {
            return parseNullLiteral(*literalChild);
        } else {
            node.print();
            throw std::runtime_error("Unrecognized Literal child kind: " + literalChild->kind);
        }
    } else {
        node.print();
        throw std::runtime_error("Unrecognized PrimaryNoArray child kind: " + primaryNoArrayChildNode->kind);
    }
}

CastExpressionNode* ASTParser::parseCastExpression(const common::ParseTree& node) {
    // Variants:
    // CastExpr LPAREN PrimitiveType RPAREN UnaryExpr
    // CastExpr LPAREN PrimitiveType LBRACKET RBRACKET RPAREN UnaryExpr
    // CastExpr LPAREN Expression RPAREN UnaryExprNotMinus
    // CastExpr LPAREN Name LBRACKET RBRACKET RPAREN UnaryExprNotMinus
    // We can distinguish the first two from the last two based on whether the child after LPAREN is a PrimitiveType or an Expression/Name
    // In the case where it's an Expression, we can check if the returned expression is a Name. If it is, we can wrap it in a ClassOrInterfaceType node to represent the cast to a reference type.
     if (node.kind != "CastExpr") {
        node.print();
        throw std::runtime_error("Expected CastExpr node");
    }
    CastExpressionNode* castExpr = ast->create<CastExpressionNode>();
    castExpr->scope = currentScope;
    if (node.children[1]->kind == "PrimitiveType") {
        castExpr->type = parsePrimitiveType(*node.children[1]);
        castExpr->type->parent = castExpr;
        if (node.children.size() > 4) {
            // Array type
            ArrayTypeNode* arrayType = ast->create<ArrayTypeNode>();
            arrayType->scope = currentScope;
            arrayType->componentType = castExpr->type;
            arrayType->componentType->parent = arrayType;
            castExpr->type = arrayType;
            arrayType->parent = castExpr;
        }
        castExpr->expression = parsePrefixExpression(*node.children.back(), PrefixExpressionNode::Negate);
        castExpr->expression->parent = castExpr;
    } else if (node.children[1]->kind == "Expression") {
        auto expr = parseExpression(*node.children[1]);
        if (expr->getNodeType() == NodeType::Name || expr->getNodeType() == NodeType::QualifiedName || expr->getNodeType() == NodeType::SimpleName) {
            // Cast to reference type
            ClassOrInterfaceTypeNode* refType = ast->create<ClassOrInterfaceTypeNode>();
            refType->scope = currentScope;
            refType->name = static_cast<NameNode*>(expr);
            refType->name->parent = refType;
            static_cast<NameNode*>(expr)->variant = NameNode::Type; // Set the name variant to Type since it's being used as a type in this context
            castExpr->type = refType;
            castExpr->type->parent = castExpr;
            castExpr->expression = parsePrefixExpression(*node.children.back(), PrefixExpressionNode::Not);
            castExpr->expression->parent = castExpr;
        } else {
            node.print();
            throw std::runtime_error("Invalid CastExpr: expression after LPAREN is not a Name or a variant, but is instead a " + nodeTypeToString(expr->getNodeType()));
        }
    } else if (node.children[1]->kind == "Name") {
        // Cast to reference type array from rule
        // CastExpr LPAREN Name LBRACKET RBRACKET RPAREN UnaryExprNotMinus
        ArrayTypeNode* arrayType = ast->create<ArrayTypeNode>();
        arrayType->scope = currentScope;

        ClassOrInterfaceTypeNode* refType = ast->create<ClassOrInterfaceTypeNode>();
        refType->scope = currentScope;
        refType->name = parseName(*node.children[1], NameNode::Type);

        arrayType->componentType = refType;
        refType->name->parent = refType;
        refType->parent = arrayType;

        castExpr->type = arrayType;
        castExpr->type->parent = castExpr;
        castExpr->expression = parsePrefixExpression(*node.children.back(), PrefixExpressionNode::Not);
        castExpr->expression->parent = castExpr;
    } else {
        node.print();
        throw std::runtime_error("Unrecognized CastExpr child kind: " + node.children[1]->kind);
    }
     return castExpr;
}

VariableDeclarationExpressionNode* ASTParser::parseVariableDeclarationExpression(const common::ParseTree& node, int& newScopeCount) {
    if (node.kind != "LocalVarDecl") {
        node.print();
        throw std::runtime_error("Expected LocalVarDecl node for variable declaration expression");
    }
    VariableDeclarationExpressionNode* varDeclExpr = ast->create<VariableDeclarationExpressionNode>();
    varDeclExpr->scope = currentScope;
    varDeclExpr->type = parseType(*node.children[0]);
    varDeclExpr->type->parent = varDeclExpr;
    for (auto varDecl : node.children[1]->findAll("VarDeclarator", "", false)) {
        varDeclExpr->fragments.push_back(parseVariableDeclarationFragment(*varDecl, true));
        varDeclExpr->fragments.back()->parent = varDeclExpr;
    }
    newScopeCount = varDeclExpr->fragments.size(); // Number of new scopes created for the variable declarations
    return varDeclExpr;
}

SimpleNameNode* ASTParser::parseSimpleName(const common::ParseTree& node) {
    if (node.kind != "IDENTIFIER") {
        node.print();
        throw std::runtime_error("Expected IDENTIFIER node for simple name");
    }
    SimpleNameNode* simpleName = ast->create<SimpleNameNode>();
    simpleName->value = node.value;
    simpleName->scope = currentScope;
    return simpleName;
}

NameNode* ASTParser::parseName(const common::ParseTree& node, NameNode::Variant variant) {
    if (node.kind != "Name") {
        node.print();
        throw std::runtime_error("Expected Name node");
    }
    if (node.children.size() == 3) {
        // Qualified name
        NameNode::Variant qualifierVariant;
        if (variant == NameNode::Package || variant == NameNode::Type) {
            qualifierVariant = NameNode::Package; // No nested classes so qualifiers have to be packages
        } else {
            qualifierVariant = NameNode::Ambiguous;
        }
        QualifiedNameNode* nameNode = ast->create<QualifiedNameNode>();
        nameNode->scope = currentScope;
        nameNode->qualifier = parseName(*node.children[0], qualifierVariant);
        nameNode->qualifier->parent = nameNode;
        nameNode->name = parseSimpleName(*node.children[2]);
        nameNode->name->parent = nameNode;
        nameNode->name->variant = variant;
        return nameNode;
    } else if (node.children.size() == 1) {
        // Simple name
        SimpleNameNode* simpleName = parseSimpleName(*node.children[0]);
        simpleName->variant = variant;
        return simpleName;
    }
    node.print();
    throw std::runtime_error("Unexpected number of children in Name node");
}

PrimitiveTypeNode* ASTParser::parsePrimitiveType(const common::ParseTree& node) {
    if (node.kind != "PrimitiveType") {
        node.print();
        throw std::runtime_error("Expected PrimitiveType node");
    }
    PrimitiveTypeNode* primitiveType = ast->create<PrimitiveTypeNode>();

    primitiveType->scope = currentScope;
    const common::ParseTree* typeNode = node.children[0].get();
    if (typeNode->kind == "BOOLEAN") {
        primitiveType->kind = PrimitiveTypeNode::Boolean;
    } else { // Numeric
        const common::ParseTree* integralTypeNode = node.findOne("IntegralType"); // Nested under NumericType for some reason, might change later.
        // TODO
        
        const common::ParseTree* primType = integralTypeNode->children[0].get();
        if (primType->kind == "BYTE") {
            primitiveType->kind = PrimitiveTypeNode::Byte;
        } else if (primType->kind == "SHORT") {
            primitiveType->kind = PrimitiveTypeNode::Short;
        } else if (primType->kind == "INT") {
            primitiveType->kind = PrimitiveTypeNode::Int;
        } else if (primType->kind == "CHAR") {
            primitiveType->kind = PrimitiveTypeNode::Char;
        } else {
            node.print();
            throw std::runtime_error("Unrecognized primitive type: " + primType->kind);
        }
    }
    return primitiveType;
}

TypeNode* ASTParser::parseReferenceType(const common::ParseTree& node) {
    if (node.kind != "ReferenceType") {
        node.print();
        throw std::runtime_error("Expected ReferenceType node");
    }
    const common::ParseTree* refTypeChildNode = node.children[0].get();
    if (refTypeChildNode->kind == "ClassOrInterfaceType") {
        return parseClassOrInterfaceType(*refTypeChildNode);
    } else if (refTypeChildNode->kind == "ArrayType") {
        return parseArrayType(*refTypeChildNode);
    } else {
        node.print();
        throw std::runtime_error("Unrecognized ReferenceType child kind: " + refTypeChildNode->kind);
    }
}

ClassOrInterfaceTypeNode* ASTParser::parseClassOrInterfaceType(const common::ParseTree& node) {
    if (node.kind == "ClassType" || node.kind == "InterfaceType") {
        return parseClassOrInterfaceType(*node.children[0]);
    }
    if (node.kind != "ClassOrInterfaceType") {
        node.print();
        throw std::runtime_error("Expected ClassOrInterfaceType node");
    }
    ClassOrInterfaceTypeNode* typeNode = ast->create<ClassOrInterfaceTypeNode>();
    typeNode->scope = currentScope;
    typeNode->name = parseName(*node.children[0], NameNode::Type);
    typeNode->name->parent = typeNode;
    return typeNode;
}

ArrayTypeNode* ASTParser::parseArrayType(const common::ParseTree& node) {
    if (node.kind != "ArrayType") {
        node.print();
        throw std::runtime_error("Expected ArrayType node");
    }
    ArrayTypeNode* arrayType = ast->create<ArrayTypeNode>();
    arrayType->scope = currentScope;
    if (node.children[0]->kind == "PrimitiveType") {
        arrayType->componentType = parsePrimitiveType(*node.children[0]);
    } else if (node.children[0]->kind == "Name") {
        ClassOrInterfaceTypeNode* refType = ast->create<ClassOrInterfaceTypeNode>();
        refType->name = parseName(*node.children[0], NameNode::Type);
        refType->name->parent = refType;
        refType->scope = currentScope;
        arrayType->componentType = refType;
        
    } else {
        node.print();
        throw std::runtime_error("Unrecognized ArrayType component type kind: " + node.children[0]->kind);
    }
    arrayType->componentType->parent = arrayType;
    return arrayType;
}

TypeNode* ASTParser::parseType(const common::ParseTree& node) {
    if (node.kind != "Type") {
        node.print();
        throw std::runtime_error("Expected Type node");
    }
    const common::ParseTree* typeChildNode = node.children[0].get();
    if (typeChildNode->kind == "PrimitiveType") {
        return parsePrimitiveType(*typeChildNode);
    } else if (typeChildNode->kind == "ReferenceType") {
        return parseReferenceType(*typeChildNode);
    } else {
        node.print();
        throw std::runtime_error("Unrecognized Type child kind: " + typeChildNode->kind);
    }
}

ModifierNode* ASTParser::parseModifier(const common::ParseTree& node) {
    if (node.kind != "Modifier") {
        node.print();
        throw std::runtime_error("Expected Modifier node");
    }
    ModifierNode* modifier = ast->create<ModifierNode>();
    modifier->scope = currentScope; // Modifiers don't actually *need* associated scopes, but this can be useful for error reporting to know what the innermost scope is when we encounter an invalid modifier
    const common::ParseTree* modifierChildNode = node.children[0].get();
    if (modifierChildNode->kind == "PUBLIC") {
        modifier->kind = ModifierNode::Public;
    } else if (modifierChildNode->kind == "PROTECTED") {
        modifier->kind = ModifierNode::Protected;
    } else if (modifierChildNode->kind == "STATIC") {
        modifier->kind = ModifierNode::Static;
    } else if (modifierChildNode->kind == "FINAL") {
        modifier->kind = ModifierNode::Final;
    } else if (modifierChildNode->kind == "ABSTRACT") {
        modifier->kind = ModifierNode::Abstract;
    } else if (modifierChildNode->kind == "NATIVE") {
        modifier->kind = ModifierNode::Native;
    } else {
        node.print();
        throw std::runtime_error("Unrecognized modifier kind: " + modifierChildNode->kind);
    }
    return modifier;
}

ArrayCreationExpressionNode* ASTParser::parseArrayCreationExpression(const common::ParseTree& node) {
    if (node.kind != "ArrayCreationExpr") {
        node.print();
        throw std::runtime_error("Expected ArrayCreationExpr node");
    }
    ArrayCreationExpressionNode* arrayCreateExpr = ast->create<ArrayCreationExpressionNode>();
    arrayCreateExpr->scope = currentScope;
    if (node.children[1]->kind == "PrimitiveType") {
        arrayCreateExpr->type = parsePrimitiveType(*node.children[1]);
    } else if (node.children[1]->kind == "ClassOrInterfaceType") {
        arrayCreateExpr->type = parseClassOrInterfaceType(*node.children[1]);
    } else {
        node.print();
        throw std::runtime_error("Unrecognized ArrayCreationExpr type child kind: " + node.children[0]->kind);
    }
    arrayCreateExpr->type->parent = arrayCreateExpr;
    ExpressionNode* sizeExpr = parseExpression(*node.children[3]);
    arrayCreateExpr->size = sizeExpr;
    sizeExpr->parent = arrayCreateExpr;
    return arrayCreateExpr;
}

IntegerLiteralNode* ASTParser::parseIntegerLiteral(const common::ParseTree& node) {
    if (node.kind != "LITERAL_INTEGER") {
        node.print();
        throw std::runtime_error("Expected LITERAL_INTEGER node");
    }
    IntegerLiteralNode* literal = ast->create<IntegerLiteralNode>();
    try {
        literal->rawValue = node.value;
        literal->value = std::stoi(node.value);
    } catch (const std::exception& e) {
        if (literal->rawValue != "2147483648") {
            node.print();
            throw std::runtime_error("Failed to parse integer literal: " + node.value + ". Error: " + e.what());
        } else {
            literal->value = 0; // Will be handled if prefixed by unary minus
        }
    }
    literal->scope = currentScope;
    return literal;
}

BooleanLiteralNode* ASTParser::parseBooleanLiteral(const common::ParseTree& node) {
    if (node.kind != "LITERAL_BOOLEAN") {
        node.print();
        throw std::runtime_error("Expected LITERAL_BOOLEAN node");
    }
    BooleanLiteralNode* literal = ast->create<BooleanLiteralNode>();
    if (node.value == "true") {
        literal->value = true;
    } else if (node.value == "false") {
        literal->value = false;
    } else {
        node.print();
        throw std::runtime_error("Invalid boolean literal value: " + node.value);
    }
    literal->scope = currentScope;
    return literal;
}

CharacterLiteralNode* ASTParser::parseCharacterLiteral(const common::ParseTree& node) {
    if (node.kind != "LITERAL_CHAR") {
        node.print();
        throw std::runtime_error("Expected LITERAL_CHAR node");
    }
    CharacterLiteralNode* literal = ast->create<CharacterLiteralNode>();
    literal->scope = currentScope;
    if (node.value.size() < 2 || node.value.front() != '\'' || node.value.back() != '\'') {
        node.print();
        throw std::runtime_error("Invalid character literal format: " + node.value);
    }
    std::string charContent = node.value.substr(1, node.value.size() - 2); // Remove the surrounding single quotes
    if (charContent[0] == '\\') {
        // Handle escape sequences
        if (charContent.size() == 2) {
            switch (charContent[1]) {
                case 'n': literal->value = '\n'; break;
                case 't': literal->value = '\t'; break;
                case 'r': literal->value = '\r'; break;
                case '\'': literal->value = '\''; break;
                case '\\': literal->value = '\\'; break;
                case 'b': literal->value = '\b'; break;
                case 'f': literal->value = '\f'; break;
                case '0':
                case '1':
                case '2':
                case '3':
                case '4':
                case '5':
                case '6':
                case '7':
                    literal->value = std::stoi(charContent.substr(1), nullptr, 8);
                    break;
                default:
                    node.print();
                    throw std::runtime_error("Unrecognized escape sequence in character literal: \\" + std::string(1, charContent[1]));
            }
        } else {
            node.print();
            throw std::runtime_error("Invalid escape sequence in character literal: " + charContent);
        }
    } else if (charContent.size() == 1) {
        literal->value = charContent[0];
    } else {
        node.print();
        throw std::runtime_error("Character literal must contain exactly one character (or a valid escape sequence): " + charContent);
    }
    return literal;
}

StringLiteralNode* ASTParser::parseStringLiteral(const common::ParseTree& node) {
    if (node.kind != "LITERAL_STRING") {
        node.print();
        throw std::runtime_error("Expected LITERAL_STRING node");
    }
    StringLiteralNode* literal = ast->create<StringLiteralNode>();
    literal->scope = currentScope;
    std::stringstream ss(node.value.substr(1, node.value.size() - 2));
    char c;
    ss >> std::noskipws; // Don't skip whitespace since it can be part of the string literal
    while (ss >> c) {
        if (c == '\\') {
            char nextChar;
            ss >> nextChar;
            switch (nextChar) {
                case 'n': literal->value.push_back('\n'); break;
                case 't': literal->value.push_back('\t'); break;
                case 'r': literal->value.push_back('\r'); break;
                case '"': literal->value.push_back('"'); break;
                case '\\': literal->value.push_back('\\'); break;
                case 'b': literal->value.push_back('\b'); break;
                case 'f': literal->value.push_back('\f'); break;
                case '0':
                case '1':
                case '2':
                case '3':
                case '4':
                case '5':
                case '6':
                case '7': {
                    std::string octalDigits(1, nextChar);
                    for (int i = 0; i < 2; ++i) {
                        if (std::stoi(octalDigits, nullptr, 8) > 255) {
                            break; // Stop if adding another octal digit would exceed the maximum value for a byte
                        }
                        int nv = ss.peek();
                        if (nv == EOF) {
                            break; // Stop if we've reached the end of the string
                        }
                        char peekChar = (char)nv;
                        if (peekChar >= '0' && peekChar <= '7') {
                            ss >> peekChar; // Consume the next octal digit
                            octalDigits += peekChar;
                        } else {
                            break;
                        }
                    }
                    literal->value.push_back(static_cast<unsigned char>(std::stoi(octalDigits, nullptr, 8)));
                    break;
                }
                default:
                    node.print();
                    throw std::runtime_error("Unrecognized escape sequence in string literal: \\" + std::string(1, nextChar));
            }
        } else {
            literal->value.push_back(static_cast<unsigned char>(c));
        }
    }
    return literal;
}

NullLiteralNode* ASTParser::parseNullLiteral(const common::ParseTree& node) {
    if (node.kind != "LITERAL_NULL") {
        node.print();
        throw std::runtime_error("Expected LITERAL_NULL node");
    }
    NullLiteralNode* literal = ast->create<NullLiteralNode>();
    literal->scope = currentScope;
    return literal;
}

ThisExpressionNode* ASTParser::parseThisExpression(const common::ParseTree& node) {
    if (node.kind != "THIS") {
        node.print();
        throw std::runtime_error("Expected THIS node");
    }
    ThisExpressionNode* thisExpr = ast->create<ThisExpressionNode>();
    thisExpr->scope = currentScope;
    return thisExpr;
}

FieldAccessExpressionNode* ASTParser::parseFieldAccessExpression(const common::ParseTree& node) {
    if (node.kind != "FieldAccess") {
        node.print();
        throw std::runtime_error("Expected FieldAccess node");
    }
    FieldAccessExpressionNode* fieldAccessExpr = ast->create<FieldAccessExpressionNode>();
    fieldAccessExpr->scope = currentScope;
    fieldAccessExpr->expression = parsePrimaryExpression(*node.children[0]);
    fieldAccessExpr->fieldName = parseSimpleName(*node.children[2]);
    return fieldAccessExpr;
}

MethodInvocationExpressionNode* ASTParser::parseMethodInvocationExpression(const common::ParseTree& node) {
    if (node.kind != "MethodCall") {
        node.print();
        throw std::runtime_error("Expected MethodCall node");
    }
    MethodInvocationExpressionNode* methodCallExpr = ast->create<MethodInvocationExpressionNode>();
    methodCallExpr->scope = currentScope;
    if (node.children[0]->kind == "Name") {
        const common::ParseTree* nameNode = node.children[0].get();
        if (nameNode->children.size() == 3) {
            // Qualified method call
            methodCallExpr->methodName = parseSimpleName(*nameNode->children[2]);
            methodCallExpr->methodName->variant = NameNode::Method;
            methodCallExpr->methodName->parent = methodCallExpr;
            methodCallExpr->expression = parseName(*nameNode->children[0], NameNode::Ambiguous);
            methodCallExpr->expression->parent = methodCallExpr;

        } else {
            // Simple method call
            methodCallExpr->methodName = parseSimpleName(*nameNode->children[0]);
            methodCallExpr->methodName->variant = NameNode::Method;
            methodCallExpr->methodName->parent = methodCallExpr;
            methodCallExpr->expression = nullptr; // Unqualified method call, expression is implicitly "this"
        }
        for (auto argNode : node.children[2]->findAll("Expression", "", false)) {
            methodCallExpr->arguments.push_back(parseExpression(*argNode));
            methodCallExpr->arguments.back()->parent = methodCallExpr;
        }
    } else if (node.children[0]->kind == "Primary") {
        methodCallExpr->methodName = parseSimpleName(*node.children[2]);
        methodCallExpr->methodName->variant = NameNode::Method;
        methodCallExpr->methodName->parent = methodCallExpr;
        methodCallExpr->expression = parsePrimaryExpression(*node.children[0]);
        methodCallExpr->expression->parent = methodCallExpr;
        for (auto argNode : node.children[4]->findAll("Expression", "", false)) {
            methodCallExpr->arguments.push_back(parseExpression(*argNode));
            methodCallExpr->arguments.back()->parent = methodCallExpr;
        }
    } else {
        node.print();
        throw std::runtime_error("Unrecognized MethodCall child kind for method name: " + node.children[0]->kind);
    }
    return methodCallExpr;
}

InstanceCreationExpressionNode* ASTParser::parseInstanceCreationExpression(const common::ParseTree& node) {
    if (node.kind != "ClassCreate") {
        node.print();
        throw std::runtime_error("Expected ClassCreate node");
    }
    InstanceCreationExpressionNode* instanceCreateExpr = ast->create<InstanceCreationExpressionNode>();
    instanceCreateExpr->scope = currentScope;
    instanceCreateExpr->type = parseClassOrInterfaceType(*node.children[1]);
    instanceCreateExpr->type->parent = instanceCreateExpr;
    for (auto argNode : node.children[3]->findAll("Expression", "", false)) {
        instanceCreateExpr->arguments.push_back(parseExpression(*argNode));
        instanceCreateExpr->arguments.back()->parent = instanceCreateExpr;
    }
    return instanceCreateExpr;
}

ArrayAccessExpressionNode* ASTParser::parseArrayAccessExpression(const common::ParseTree& node) {
    if (node.kind != "ArrayAccess") {
        node.print();
        throw std::runtime_error("Expected ArrayAccess node");
    }
    ArrayAccessExpressionNode* arrayAccessExpr = ast->create<ArrayAccessExpressionNode>();
    arrayAccessExpr->scope = currentScope;
    if (node.children[0]->kind == "PrimaryNoNewArray") {
        arrayAccessExpr->array = parsePrimaryNoArrayExpression(*node.children[0]);
    } else if (node.children[0]->kind == "Name") {
        arrayAccessExpr->array = parseName(*node.children[0], NameNode::Expression);
    } else {
        node.print();
        throw std::runtime_error("Unrecognized ArrayAccess child kind for array expression: " + node.children[0]->kind);
    }
    arrayAccessExpr->index = parseExpression(*node.children[2]);
    arrayAccessExpr->array->parent = arrayAccessExpr;
    arrayAccessExpr->index->parent = arrayAccessExpr;
    return arrayAccessExpr;
}

AST& ASTParser::parseNew(const common::ParseTree& root, std::string name) {
    if (root.kind != "Start") {
        root.print();
        throw std::runtime_error("Expected Start node as root of parse tree");
    }
    astUnits.emplace_back(name);
    ast = &astUnits.back();
    currentScope = &globalScope;
    auto ref = parseCompilationUnit(*root.children[0]);
    ast->setRoot(ref);
    return *ast;
}


} // namespace semantics