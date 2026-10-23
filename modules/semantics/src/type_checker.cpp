#include "semantics/type_checker.hpp"
#include "ast/ast_printer.hpp"
#include "ast/java_type.hpp"
#include "ast/nodes.hpp"
#include <iostream>

namespace semantics {

bool TypeChecker::visit(TypeDeclarationNode* node) {
    currentType = node;
    return true;
}

void TypeChecker::leave(TypeDeclarationNode* node) {
    currentType = nullptr;
}

bool TypeChecker::visit(MethodDeclarationNode* node) {
    currentMethod = MethodInfo {
        .decl = { .methodDecl = node },
        .returnType = node->returnType ? typeManager.getTypeFromNode(node->returnType) : &JavaType::Void,
        .signature = typeManager.getSignature(node),
        .abs = node->isAbstract(),
        .stat = node->isStatic(),
        .fin = node->isFinal(),
        .pub = node->isPublic(),
        .prot = node->isProtected(),
        .native = node->isNative(),
        .isConstructor = false
    };
    for (ModifierNode* m : node->modifiers) {
        switch (m->kind) {
            case ModifierNode::Kind::Public: currentMethod.pub = true; break;
            case ModifierNode::Kind::Protected: currentMethod.prot = true; break;
            case ModifierNode::Kind::Static: currentMethod.stat = true; break;
            case ModifierNode::Kind::Final: currentMethod.fin = true; break;
            case ModifierNode::Kind::Abstract: currentMethod.abs = true; break;
            default: break; // Ignore other modifiers for now
        }
    }
    if (node->body) node->body->accept(*this); // Type check constructor body immediately since constructors don't have separate declarations like methods do
    
    return false;
}

void TypeChecker::leave(MethodDeclarationNode* node) {
    currentMethod = {};
}

bool TypeChecker::visit(ConstructorDeclarationNode* node) {
    currentMethod = MethodInfo {
        .decl = { .ctorDecl = node },
        .returnType = nullptr,
        .signature = typeManager.getSignature(node),
        .abs = false,
        .stat = false,
        .fin = false,
        .pub = node->isPublic(),
        .prot = node->isProtected(),
        .native = false,
        .isConstructor = true,
    };
    for (ModifierNode* m : node->modifiers) {
        switch (m->kind) {
            case ModifierNode::Kind::Public: currentMethod.pub = true; break;
            case ModifierNode::Kind::Protected: currentMethod.prot = true; break;
            case ModifierNode::Kind::Static: currentMethod.stat = true; break;
            case ModifierNode::Kind::Final: currentMethod.fin = true; break;
            case ModifierNode::Kind::Abstract: currentMethod.abs = true; break;
            default: break; // Ignore other modifiers for now
        }
    }
    node->body->accept(*this); // Type check constructor body immediately since constructors don't have separate declarations like methods do
    return false;
}

void TypeChecker::leave(ConstructorDeclarationNode* node) {
    currentMethod = {};
}

bool TypeChecker::visit(FieldDeclarationNode* node) {
    // Spoof a method info
    currentMethod = MethodInfo {
        .decl = {},
        .returnType = nullptr,
        .signature = { "", {} },
        .abs = false,
        .stat = false,
        .fin = false,
        .pub = false,
        .prot = false,
        .isConstructor = false
    };
    for (ModifierNode* m : node->modifiers) {
        switch (m->kind) {
            case ModifierNode::Kind::Public: currentMethod.pub = true; break;
            case ModifierNode::Kind::Protected: currentMethod.prot = true; break;
            case ModifierNode::Kind::Static: currentMethod.stat = true; break;
            case ModifierNode::Kind::Final: currentMethod.fin = true; break;
            case ModifierNode::Kind::Abstract: currentMethod.abs = true; break;
            default: break; // Ignore other modifiers for now
        }
    }

    JavaType* fieldType = typeManager.getTypeFromNode(node->type);
    for (auto fragment : node->fragments) {
        currentMethod.signature.name = "[field] " + fragment->name->value;
        fragment->accept(*this); // Type check variable declarators immediately
        if (!fragment->initializer) continue; // No initializer, so nothing to check
        if (fragment->initializer->resolvedType) {
            if (fragment->initializer->resolvedType->assignableTo(*fieldType)) {
                // OK
            } else {
                die("Type of field initializer is not assignable to field type");
            }
        } else {
            die("Field initializer has unresolved type");
        }
    }
    return false;
}

void TypeChecker::leave(FieldDeclarationNode* node) {
    currentMethod = {};
}

void TypeChecker::leave(IntegerLiteralNode* node) {
    node->resolvedType = &JavaType::Int;
}

void TypeChecker::leave(BooleanLiteralNode* node) {
    node->resolvedType = &JavaType::Boolean;
}

void TypeChecker::leave(StringLiteralNode* node) {
    node->resolvedType = JavaType::String;
}

void TypeChecker::leave(NullLiteralNode* node) {
    node->resolvedType = &JavaType::Null;
}

void TypeChecker::leave(CharacterLiteralNode* node) {
    node->resolvedType = &JavaType::Char;
}

void TypeChecker::leave(PrefixExpressionNode* node) {
    if (node->operand->resolvedType == nullptr) {
        die("Operand of prefix expression has unresolved type");
    }
    if (node->operand->resolvedType->isNumeric() && node->op == PrefixExpressionNode::Operator::Negate) {
        node->resolvedType = &JavaType::Int;
    } else if (node->operand->resolvedType == &JavaType::Boolean && node->op == PrefixExpressionNode::Operator::Not) {
        node->resolvedType = &JavaType::Boolean;
    } else {
        die("Invalid operand type for prefix operator");
    }
}

void TypeChecker::leave(InfixExpressionNode* node) {
    node->resolvedType = nullptr;
    JavaType* leftType = node->lhs->resolvedType;
    JavaType* rightType = node->rhs->resolvedType;
    if (leftType == nullptr || rightType == nullptr) {
        node->accept(ASTPrinter::Instance);
        die("Unresolved type in infix expression");
    }
    if (leftType->isNumeric() && rightType->isNumeric()) {
        switch (node->op) {
            case InfixExpressionNode::Add:
            case InfixExpressionNode::Subtract:
            case InfixExpressionNode::Multiply:
            case InfixExpressionNode::Divide:
            case InfixExpressionNode::Remainder:
                node->resolvedType = &JavaType::Int;
                break;
            case InfixExpressionNode::Equality:
            case InfixExpressionNode::NotEqual:
            case InfixExpressionNode::LessThanOrEqual:
            case InfixExpressionNode::LessThan:
            case InfixExpressionNode::GreaterThanOrEqual:
            case InfixExpressionNode::GreaterThan:
                node->resolvedType = &JavaType::Boolean;
                break;
            case InfixExpressionNode::InclusiveOr:
            case InfixExpressionNode::And:
                die("Bitwise operations are not allowed");
                break;
            default:
                break;
        }
    }
    if (leftType == &JavaType::Boolean && rightType == &JavaType::Boolean) {
        switch (node->op) {
            case InfixExpressionNode::ConditionalAnd:
            case InfixExpressionNode::ConditionalOr:
            case InfixExpressionNode::Equality:
            case InfixExpressionNode::NotEqual:
            case InfixExpressionNode::InclusiveOr:
            case InfixExpressionNode::And:
                node->resolvedType = &JavaType::Boolean;
                break;
            default:
                break;
        }
    }
    if ((leftType->kind == JavaType::Reference || leftType == &JavaType::Null || leftType->kind == JavaType::Array) &&
        (rightType->kind == JavaType::Reference || rightType == &JavaType::Null || rightType->kind == JavaType::Array) &&
        (leftType->assignableTo(*rightType) || rightType->assignableTo(*leftType))
    ) {
        switch (node->op) {
            case InfixExpressionNode::Equality:
            case InfixExpressionNode::NotEqual:
                node->resolvedType = &JavaType::Boolean;
                break;
            default:
                break;
        }
    }
    if ((leftType == JavaType::String && rightType != &JavaType::Void) ||
        (rightType == JavaType::String && leftType != &JavaType::Void)) {
        switch (node->op) {
            case InfixExpressionNode::Add:
                node->resolvedType = JavaType::String;
                break;
            default:
                break;
        }
    }
    if (node->resolvedType == nullptr) {
        std::cerr << "Operators: " << leftType->name << " " << node->op << " " << rightType->name << "\n";
        die("Invalid operator / operand type combination");
    }
}

bool TypeChecker::visit(SimpleNameNode* node) {
    return false;
}

void TypeChecker::leave(SimpleNameNode* node) {
    node->resolvedType = nullptr; // Default to unresolved, and try to resolve as an expression
    switch (node->variant) {
        case NameNode::Package:
        case NameNode::Type:
        case NameNode::RawIdentifier:
        case NameNode::Method:
            break;
        case NameNode::Expression: {
            // node->accept(ASTPrinter::Instance);
            if (node->resolvedReference->getNodeType() == NodeType::VariableDeclarationFragment) {
                VariableDeclarationFragmentNode* varDecl = static_cast<VariableDeclarationFragmentNode*>(node->resolvedReference);
                JavaType* varType = resolveType(varDecl);
                if (!varType) {
                    die("Couldn't resolve variable declaration fragment to a type");
                }
                if (currentMethod.stat && isNonStaticField(varDecl)) {
                    die("Context staticity does not match variable declaration staticity (implicit this)");
                }
                node->resolvedType = varType;
                node->assignable = isAssignable(varDecl); // Final variables are not assignable
                node->isLValue = true;
                break;
            } else if (node->resolvedReference->getNodeType() == NodeType::SingleVariableDeclaration) {
                JavaType* paramType = resolveType(static_cast<SingleVariableDeclarationNode*>(node->resolvedReference));
                if (paramType) {
                    node->resolvedType = paramType;
                    break;
                }
            }
            std::cerr << "Simple name expression has unexpected resolved reference node type " << node->resolvedReference->getNodeType() << "\n";
            die("Simple name expression has unexpected resolved reference node type");
            break;
        }
        default:
            std::cerr << "Unrecognized name node variant: " << node->variant << "\n";
            die("Unrecognized name node variant");
            break;
    }
}

bool TypeChecker::visit(QualifiedNameNode* node) {
    // Recurse down qualifier
    node->qualifier->accept(*this);
    return false;
}

void TypeChecker::leave(QualifiedNameNode* node) {
    // Invariant: if ambiguous, child should have been resolved to an expression.
    node->resolvedType = nullptr; // Default to unresolved, and try to resolve as an expression
    switch (node->variant) {
        // Possible that the qualifier is left as a package name from disambig, up and down the chain. 
        // In this case the resolvedType will be left as null. Will have to check for that in places which have sub-exprs
        case NameNode::Package: 
        case NameNode::Type:
        case NameNode::RawIdentifier:
        case NameNode::Method:
            break;
        case NameNode::Expression: {
            if (node->qualifier->variant != NameNode::Type) {
                node->accept(ASTPrinter::Instance);
                die("Expected qualifier to be resolved to a type for qualified name expression");
            }
            auto* classType = ((TypeDeclarationNode*)node->qualifier->resolvedReference)->resolvedType;
            auto fieldIt = classType->fields.find(node->name->getSimpleName());
            if (fieldIt == classType->fields.end() || !fieldIt->second.stat) {
                die("No static field named " + node->name->getSimpleName() + " found in type " + classType->name);
                
            }

            if (fieldIt->second.prot) {
                JavaType* curType = currentType->resolvedType;
                if (!curType->isSubtypeOf(*fieldIt->second.declaringType) && classType->package != curType->package) {
                    std::cerr << "Accessing type: " << curType->name << ", field origin type: " << fieldIt->second.declaringType->name << "\n";
                    die("Cannot access protected static field from a non-subclass");
                }
            }
            
            node->assignable = !fieldIt->second.fin; // Final fields are not assignable
            node->isLValue = true;
            node->resolvedType = fieldIt->second.type;
            node->resolvedReference = fieldIt->second.decl;
            node->variant = NameNode::Expression; // Disambiguate to expression name
            break;
        }
        case NameNode::Ambiguous: {
            if (node->qualifier->variant != NameNode::Expression) {
                node->accept(ASTPrinter::Instance);
                die("Expected qualifier to be resolved to an expression for ambiguous qualified name");
            }
            auto* classType = node->qualifier->resolvedType;
            auto fieldIt = classType->fields.find(node->name->getSimpleName());
            if (fieldIt == classType->fields.end() || fieldIt->second.stat) { // Instance field
                die("No instance field named " + node->name->getSimpleName() + " found in type " + classType->name);
            }

            if (fieldIt->second.prot) {
                JavaType* curType = currentType->resolvedType;
                if (!classType->isSubtypeOf(*curType) || 
                    !curType->isSubtypeOf(*fieldIt->second.declaringType) && 
                    classType->package != curType->package) {
                    std::cerr << "Accessing type: " << curType->name << ", field origin type: " << fieldIt->second.declaringType->name << "\n";
                    die("Cannot access protected static field from a non-subclass");
                }
            }
            
            node->assignable = !fieldIt->second.fin; // Final fields are not assignable
            node->isLValue = true;
            node->resolvedType = fieldIt->second.type;
            node->resolvedReference = fieldIt->second.decl;
            node->variant = NameNode::Expression; // Disambiguate to expression name
            break;
        }
        default:
            std::cerr << "Unrecognized name node variant: " << node->variant << "\n";
            die("Unrecognized name node variant");
            break;
    }
}

bool TypeChecker::visit(VariableDeclarationFragmentNode* node) {
    return true;
}

void TypeChecker::leave(AssignmentExpressionNode* node) {
    JavaType* leftType = node->lhs->resolvedType;
    JavaType* rightType = node->rhs->resolvedType;
    // node->accept(ASTPrinter::Instance);
    if (leftType == nullptr || rightType == nullptr) {
        die("Unresolved type in assignment expression");
    }
    if (!node->lhs->assignable) {
        std::cerr << "Attempting to assign to a node: " << node->lhs->getNodeType() << " that is not assignable\n";
        die("Left-hand side of assignment is not assignable, either by type or by node property");
    }
    if (!rightType->assignableTo(*leftType)) {
        std::cerr << "Left-hand side type: " << leftType->name << ", right-hand side type: " << rightType->name << "\n";
        die("Type of right-hand side of assignment is not assignable to type of left-hand side");
    }
    node->resolvedType = leftType;
}

void TypeChecker::leave(MethodInvocationExpressionNode* node) {
    Signature sig = typeManager.getSignature(node);
    if (node->expression == nullptr) {
        // std::cerr << "Unqualified method invocation: looking for method " << sig.toString() << " in current type\n";
        // Unqualified method invocation, implicit this
        JavaType* thisType = currentType->resolvedType;
        if (thisType == nullptr) {
            die("Unresolved current type for unqualified method invocation");
        }
        auto methodIt = thisType->methods.find(sig);
        if (methodIt == thisType->methods.end()) {
            die("Method not found in current type " + thisType->name);
        }
        if (methodIt->second.stat) {
            die("Attempting to call static method without class expression");
        } else if (currentMethod.stat) {
            die("Attempting to call instance method from static context");
        }
        // No prot check needed because we're already in the class
        node->resolvedDeclaration = const_cast<MethodDeclarationNode*>(methodIt->second.decl.methodDecl);
        if (methodIt->second.returnType) {
            node->resolvedType = methodIt->second.returnType;
        } else {
            node->resolvedType = &JavaType::Void; // void return type
        }
        return;
    }
    auto nodeType = node->expression->getNodeType();
    if (nodeType == NodeType::SimpleName || nodeType == NodeType::QualifiedName) {
        NameNode* nameNode = static_cast<NameNode*>(node->expression);
        if (nameNode->variant == NameNode::Type) {
            JavaType* type = ((TypeDeclarationNode*)nameNode->resolvedReference)->resolvedType;
            auto methodIt = type->methods.find(sig);
            if (methodIt == type->methods.end()) {
                die("Method not found in type " + type->name);
            }
            if (!methodIt->second.stat) die("Trying to call an instance method from a type");
            
            if (methodIt->second.prot) {
                JavaType* curType = currentType->resolvedType;
                if (!curType->isSubtypeOf(*methodIt->second.declaringType) && type->package != curType->package) {
                    std::cerr << "Accessing type: " << curType->name << ", method origin type: " << methodIt->second.declaringType->name << "\n";
                    die("Cannot access protected static method from a non-subclass");
                }
            }
            node->resolvedDeclaration = const_cast<MethodDeclarationNode*>(methodIt->second.decl.methodDecl);
            node->resolvedType = methodIt->second.returnType;    

        } else if (nameNode->variant == NameNode::Expression) {
            if (!nameNode->resolvedReference) {
                nameNode->accept(ASTPrinter::Instance);
                std::cerr << "Unresolved name node reference, assuming we're dealing with something like .length\n";
                die("No methods can be called on primitive types");
            }
            switch (nameNode->resolvedReference->getNodeType()) {
                case NodeType::VariableDeclarationFragment: {
                    VariableDeclarationFragmentNode* varDecl = static_cast<VariableDeclarationFragmentNode*>(nameNode->resolvedReference);
                    auto varType = resolveType(varDecl);
                    if (varType == nullptr) {
                        die("Unresolved variable declaration fragment type in method invocation expression");
                    }
                    auto methodIt = varType->methods.find(sig);
                    if (methodIt == varType->methods.end()) die("Method not found in type " + varType->name);


                    if (methodIt->second.stat) {
                        die("Trying to call a static method from an instance");
                    }

                    if (methodIt->second.prot) {
                        JavaType* curType = currentType->resolvedType;
                        if ((!curType->isSubtypeOf(*methodIt->second.declaringType) || 
                            !varType->isSubtypeOf(*curType)) && varType->package != curType->package) {
                            std::cerr << "Accessing type: " << curType->name << ", method origin type: " << varType->name << "\n";
                            die("Cannot access protected instance method from a non-subclass");
                        }
                    }

                    node->resolvedType = methodIt->second.returnType;
                    node->resolvedDeclaration = const_cast<MethodDeclarationNode*>(methodIt->second.decl.methodDecl);
                    break;
                }
                case NodeType::SingleVariableDeclaration: {
                    SingleVariableDeclarationNode* paramDecl = static_cast<SingleVariableDeclarationNode*>(nameNode->resolvedReference);
                    auto varType = resolveType(paramDecl);
                    if (varType == nullptr) {
                        die("Unresolved variable declaration fragment type in method invocation expression");
                    }
                    auto methodIt = varType->methods.find(sig);
                    if (methodIt == varType->methods.end()) die("Method not found in type " + varType->name);

                    if (methodIt->second.prot) {
                        JavaType* curType = currentType->resolvedType;
                        if ((!curType->isSubtypeOf(*methodIt->second.declaringType) ||
                             !varType->isSubtypeOf(*curType)) && varType->package != curType->package) {
                            std::cerr << "Accessing type: " << curType->name << ", method origin type: " << varType->name << "\n";
                            die("Cannot access protected instance method from a non-subclass");
                        }
                    }

                    node->resolvedType = methodIt->second.returnType;
                    node->resolvedDeclaration = const_cast<MethodDeclarationNode*>(methodIt->second.decl.methodDecl);
                    break;
                }
                default:
                    node->accept(ASTPrinter::Instance);
                    nameNode->resolvedReference->accept(ASTPrinter::Instance);
                    die("Unrecognized simple name reference kind in method invocation expression");
                    break;
            }
        } else {
            die("Unexpected name node variant in method invocation expression (possibly undisambiguable expression)");
        }
        return;
    }
    JavaType* expressionType = node->expression->resolvedType;
    if (expressionType == nullptr) {
        die("Unresolved type in method invocation expression");
    }
    
    auto methodIt = expressionType->methods.find(sig);
    if (methodIt == expressionType->methods.end()) die("Method not found in type " + expressionType->name);
    if (methodIt->second.prot) {
        JavaType* curType = currentType->resolvedType;
        if ((!expressionType->isSubtypeOf(*curType) ||
            !curType->isSubtypeOf(*methodIt->second.declaringType)) && expressionType->package != curType->package) {
            std::cerr << "Accessing type: " << curType->name << ", method origin type: " << expressionType->name << "\n";
            die("Cannot access protected instance method from a non-subclass");
        }
    }

    if (methodIt->second.stat) {
        die("Trying to call a static method from an instance");
    }

    node->resolvedType = methodIt->second.returnType;
    node->resolvedDeclaration = const_cast<MethodDeclarationNode*>(methodIt->second.decl.methodDecl);
        
}

bool TypeChecker::visit(FieldAccessExpressionNode* node) {
    node->expression->accept(*this);
    return false;
}

void TypeChecker::leave(FieldAccessExpressionNode* node) {
    // 1. Ensure the receiver expression was successfully typed
    JavaType* receiverType = node->expression->resolvedType;
    if (!receiverType) {
        // Might be a type name instead of an expression, so try to resolve as a type and look for static fields
        if (node->expression->getNodeType() != NodeType::SimpleName &&
            node->expression->getNodeType() != NodeType::QualifiedName
        ) {
            die("Unresolved receiver type in field access expression");
        }
        NameNode* nameNode = static_cast<NameNode*>(node->expression);
        
        if (nameNode->variant != NameNode::Type) {
            die("Expected a type name for static field access");
        }
        JavaType* type = ((TypeDeclarationNode*)nameNode->resolvedReference)->resolvedType;
        auto fieldIt = type->fields.find(node->fieldName->getSimpleName());
        if (fieldIt == type->fields.end() || !fieldIt->second.stat) {
            die("No static field named " + node->fieldName->getSimpleName() + " found in type " + type->name);
        }

        if (fieldIt->second.prot) {
            JavaType* accessingType = currentType->resolvedType;
            if ((!accessingType->isSubtypeOf(*fieldIt->second.declaringType) || 
                !type->isSubtypeOf(*accessingType)) && accessingType->package != type->package) {
                die("Cannot access protected field from a non-subclass in a different package");
            }
        }

        node->assignable = !fieldIt->second.fin; // Final fields are not assignable
        node->isLValue = true;
        node->resolvedType = fieldIt->second.type;
        return; // Success
    }

    std::string fieldName = node->fieldName->getSimpleName();

    // 2. Resolve the field in the receiver's type (including inherited fields)
    auto it = receiverType->fields.find(fieldName);
    if (it == receiverType->fields.end()) {
        node->accept(ASTPrinter::Instance);
        die("Field '" + fieldName + "' not found in type " + receiverType->name);
    }

    FieldInfo& info = it->second;

    // 3. Static vs Non-Static Check
    // Therefore, the field must NOT be static.
    if (info.stat) {
        die("Attempting to access static field '" + fieldName + "' via instance.");
    }

    // Protected check
    if (info.prot) {
        JavaType* accessingType = currentType->resolvedType;
        if ((!accessingType->isSubtypeOf(*info.declaringType) || 
            !receiverType->isSubtypeOf(*accessingType)) &&
            accessingType->package != receiverType->package) {
            die("Cannot access protected field '" + fieldName + "' from a non-subclass in a different package");
        }
    }

    // Success: Assign the field's type to this node
    if (info.fin) node->assignable = false;
    node->isLValue = true;
    node->resolvedType = info.type;
}

void TypeChecker::leave(CastExpressionNode* node) {
    JavaType* targetType = typeManager.getTypeFromNode(node->type);
    if (targetType == nullptr) {
        die("Unresolved target type in cast expression");
    }
    JavaType* srcType = node->expression->resolvedType;
    if (srcType == nullptr) {
        die("Unresolved expression type in cast expression");
    }
    if (srcType->castableTo(*targetType)) {
        node->resolvedType = targetType;
    } else if (
        (srcType->isInterface && targetType->isInterface) ||
        (srcType->isInterface && !targetType->isInterface && !targetType->fin) ||
        (!srcType->isInterface && targetType->isInterface && !srcType->fin)
    ) {
        // Allow casting between unrelated interfaces, since in Java this is allowed at compile time and only fails at runtime
        node->resolvedType = targetType;
    } else {
        std::cerr << targetType->name << " <--  cast " << srcType->name << " is " << srcType->castableTo(*targetType) << std::endl;
        std::cerr << targetType->name << " <--  assign " << srcType->name << " is " << srcType->assignableTo(*targetType) << std::endl;
        std::cerr << srcType->name << " <--  assign " << targetType->name << " is " << targetType->assignableTo(*srcType) << std::endl;
        die("Incompatible types in cast expression");
    }

}

void TypeChecker::leave(InstanceofExpressionNode* node) {
    if (node->expression->resolvedType == nullptr) {
        die("Unresolved expression type in instanceof expression");
    }
    JavaType* type = node->expression->resolvedType;
    if (type != &JavaType::Null && type->kind != JavaType::Reference && type->kind != JavaType::Array) {
        die("Operand to instanceof must be either null or a reference type (includes arrays)");
    }
    node->resolvedCheckType = typeManager.getTypeFromNode(node->type);
    if (node->resolvedCheckType == nullptr) {
        die("Unresolved check type in instanceof expression");
    }
    if (node->resolvedCheckType->kind != JavaType::Reference && node->resolvedCheckType->kind != JavaType::Array) {
        die("Check type in instanceof expression must be a reference type (includes arrays)");
    }
    node->resolvedType = &JavaType::Boolean;

}

void TypeChecker::leave(ParenthesizedExpressionNode* node) {
    node->resolvedType = node->expression->resolvedType;
    node->assignable = node->expression->assignable;
    node->isLValue = node->expression->isLValue;
}

void TypeChecker::leave(ReturnStatementNode* node) {
    if (currentMethod.isConstructor) {
        if (node->expression) {
            die("Constructor return statement cannot have an expression");
        }
    } else {
        if (node->expression) {
            if (currentMethod.returnType == &JavaType::Void) {
                die("Void method return statement cannot have an expression");
            } else if (!node->expression->resolvedType) {
                die("Unresolved return expression type");
            } else if (!node->expression->resolvedType->assignableTo(*currentMethod.returnType)) {
                die("Return expression type is not assignable to method return type");
            }
        } else {
            if (currentMethod.returnType != &JavaType::Void) {
                node->accept(ASTPrinter::Instance);
                die("Non-void method return statement must have an expression");
            }
        }
    }
}

void TypeChecker::leave(IfStatementNode* node) {
    if (node->condition->resolvedType == nullptr) {
        die("Unresolved condition type in if statement");
    }
    if (node->condition->resolvedType != &JavaType::Boolean) {
        die("Condition expression in if statement must be of type boolean");
    }
}

void TypeChecker::leave(WhileStatementNode* node) {
    if (node->condition->resolvedType == nullptr) {
        die("Unresolved condition type in while statement");
    }
    if (node->condition->resolvedType != &JavaType::Boolean) {
        die("Condition expression in while statement must be of type boolean");
    }
}

void TypeChecker::leave(ForStatementNode* node) {
    if (node->condition->resolvedType == nullptr) {
        die("Unresolved condition type in for statement");
    }
    if (node->condition->resolvedType != &JavaType::Boolean) {
        die("Condition expression in for statement must be of type boolean");
    }
}

void TypeChecker::leave(ArrayAccessExpressionNode* node) {
    if (node->array->resolvedType == nullptr) {
        die("Unresolved array type in array access expression");
    }
    if (node->array->resolvedType->kind != JavaType::Kind::Array) {
        die("Attempting to index into a non-array type in array access expression");
    }
    if (node->index->resolvedType == nullptr) {
        die("Unresolved index type in array access expression");
    }
    if (!node->index->resolvedType->isNumeric()) {
        die("Index expression in array access must be of type int");
    }
    node->resolvedType = node->array->resolvedType->arrayComponentType;
}

void TypeChecker::leave(InstanceCreationExpressionNode* node) {
    Signature sig = typeManager.getSignature(node);
    auto typeIt = typeManager.getTypeFromNode(node->type);
    if (typeIt == nullptr) {
        die("Unresolved type in instance creation expression");
    }
    if (typeIt->kind != JavaType::Kind::Reference) {
        die("Attempting to instantiate a non-class type in instance creation expression");
    }
    if (typeIt->abs) {
        die("Cannot instantiate abstract class " + typeIt->name);
    }
    auto methodIt = typeIt->constructors.find(sig);
    if (methodIt == typeIt->constructors.end()) {
        die("Constructor with matching signature not found in type " + typeIt->name);
    }
    
    if (methodIt->second.prot) {
        JavaType* curType = currentType->resolvedType;
        if (curType->package != typeIt->package) {
            std::cerr << "Accessing type: " << curType->name << ", constructor origin type: " << typeIt->name << "\n";
            die("Cannot access protected constructor explicitly from a different package");
        }
    }
    node->resolvedType = typeIt;
}

void TypeChecker::leave(ThisExpressionNode* node) {
    if (currentMethod.stat) {
        die("Cannot use 'this' in a static context");
    }
    node->resolvedType = currentType->resolvedType;
}

void TypeChecker::leave(ArrayCreationExpressionNode* node) {
    if (node->size->resolvedType == nullptr) {
        die("Unresolved size type in array creation expression");
    }
    if (!node->size->resolvedType->isNumeric()) {
        die("Size expression in array creation must be of type int");
    }
    JavaType* componentType = typeManager.getTypeFromNode(node->type);
    if (componentType == nullptr) {
        die("Unresolved component type in array creation expression");
    }
    node->resolvedType = typeManager.getArrayType(componentType);

}

void TypeChecker::leave(SingleVariableDeclarationNode* node) {
    JavaType* leftType = typeManager.getTypeFromNode(node->type);
    if (!node->initializer) {
        die("Single variable declaration missing initializer");
    }
    JavaType* rightType = node->initializer->resolvedType;
    if (rightType == nullptr) {
        die("Unresolved type in assignment expression");
    }

    if (!rightType->assignableTo(*leftType)) {
        std::cerr << "Variable declaration type: " << leftType->name << ", initializer type: " << rightType->name << "\n";
        die("Type of right-hand side of single-variable assignment is not assignable to type of left-hand side");
    }
}

void TypeChecker::leave(VariableDeclarationStatementNode* node) {
    JavaType* leftType = typeManager.getTypeFromNode(node->type);
    for (auto fragment : node->fragments) {
        if (!fragment->initializer) {
            die("Variable declaration statement missing initializer");
        }
        JavaType* rightType = fragment->initializer->resolvedType;
        if (rightType == nullptr) {
            die("Unresolved type in assignment expression");
        }

        if (!rightType->assignableTo(*leftType)) {
            std::cerr << "Variable declaration type: " << leftType->name << ", initializer type: " << rightType->name << "\n";
            die("Type of right-hand side of declaration assignment is not assignable to type of left-hand side");
        }
    }
}

void TypeChecker::leave(VariableDeclarationExpressionNode* node) {
    JavaType* leftType = typeManager.getTypeFromNode(node->type);
    for (auto fragment : node->fragments) {
        if (!fragment->initializer) {
            die("Variable declaration expression missing initializer");
        }
        JavaType* rightType = fragment->initializer->resolvedType;
        if (rightType == nullptr && fragment->initializer) {
            die("Unresolved type in assignment expression");
        }
        if (!fragment->initializer) continue; // No initializer, so nothing to check

        if (!rightType->assignableTo(*leftType)) {
            std::cerr << "Variable declaration type: " << leftType->name << ", initializer type: " << rightType->name << "\n";
            die("Type of right-hand side of declaration expression assignment is not assignable to type of left-hand side");
        }
    }
}

} // namespace semantics