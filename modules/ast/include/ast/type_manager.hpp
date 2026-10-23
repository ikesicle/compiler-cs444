#ifndef AST_TYPEMANAGER_HPP
#define AST_TYPEMANAGER_HPP

#include "java_type.hpp"
#include "nodes.hpp"
#include <iostream>
#include <memory>
#include <vector>
#include <unordered_map>
#include <stack>

namespace ast {

class TypeManager {

    std::vector<std::unique_ptr<JavaType>> generatedTypes;
    std::unordered_map<JavaType*, JavaType*> arrayTypeCache;
public:
    std::unordered_map<JavaType*, std::pair<int, int>> classTreeIDCache{};
    JavaType* getArrayType(JavaType* componentType) {
        if (arrayTypeCache.count(componentType)) {
            return arrayTypeCache[componentType];
        }
        generatedTypes.push_back(std::make_unique<JavaType>());
        JavaType* arrayTypePtr = generatedTypes.back().get();
        arrayTypePtr->kind = JavaType::Kind::Array;
        arrayTypePtr->arrayComponentType = componentType;
        arrayTypePtr->name = componentType->name + "#array";
        arrayTypePtr->resolved = true; // Array types are considered resolved as soon as they're created
        arrayTypePtr->package = componentType->package; // Put array type in same package as component type for simplicity.
        arrayTypePtr->directSuperClass = JavaType::Object; // All array types directly extend Object
        arrayTypePtr->superTypes = { JavaType::Object, JavaType::Cloneable, JavaType::Serializable }; // All array types are subtypes of Object
        arrayTypePtr->fields = { { "length", FieldInfo{ 
            .decl = nullptr,
            .type = &JavaType::Int, 
            .declaringType = arrayTypePtr,  
            .name = "length",
            .stat = false, 
            .fin = true, 
            .pub = true } 
        } }; // All array types have a non-static int field named length
        arrayTypeCache[componentType] = arrayTypePtr;
        std::cerr << "Created array type for component type " << componentType->name << ": " << arrayTypePtr << "\n";
        return arrayTypePtr;
    }

    JavaType* getNewType() {
        generatedTypes.push_back(std::make_unique<JavaType>());
        return generatedTypes.back().get();
    }

    Signature getSignature(const MethodDeclarationNode* m) {
        Signature sig;
        sig.name = m->name ? m->name->value : "<unknown>";
        for (size_t i = 0; i < m->parameters.size(); i++) {
            sig.paramTypes.push_back(getTypeFromNode(m->parameters[i]->type));
        }
        return sig;
    }

    Signature getSignature(const MethodInvocationExpressionNode* m) {
        Signature sig;
        sig.name = m->methodName ? m->methodName->getSimpleName() : "<unknown>";
        for (size_t i = 0; i < m->arguments.size(); i++) {
            if (m->arguments[i]->resolvedType == nullptr) {
                throw std::runtime_error("Unresolved type in method invocation argument");
            }
            sig.paramTypes.push_back(m->arguments[i]->resolvedType);
        }
        return sig;
    }

    Signature getSignature(const InstanceCreationExpressionNode* c) {
        Signature sig;
        sig.name = c->type ? c->type->name->getSimpleName() : "<unknown>";
        for (size_t i = 0; i < c->arguments.size(); i++) {
            if (c->arguments[i]->resolvedType == nullptr) {
                throw std::runtime_error("Unresolved type in constructor invocation argument");
            }
            sig.paramTypes.push_back(c->arguments[i]->resolvedType);
        }
        return sig;
    }

     Signature getSignature(const FieldAccessExpressionNode* f) {
        Signature sig;
        sig.name = f->fieldName ? f->fieldName->getSimpleName() : "<unknown>";
        return sig;
    }

    Signature getSignature(const ConstructorDeclarationNode* c) {
        Signature ret;
        ret.name = c->name->getSimpleName();
        for (size_t i = 0; i < c->parameters.size(); i++) {
            ret.paramTypes.push_back(getTypeFromNode(c->parameters[i]->type));
        }
        return ret;
    }

    JavaType* getTypeFromNode(const TypeNode* t) {
        if (!t) {
            throw std::runtime_error("Null type node");
        }
        if (t->getNodeType() == NodeType::PrimitiveType) {
            auto* p = static_cast<const PrimitiveTypeNode*>(t);
            switch (p->kind) {
                case PrimitiveTypeNode::Kind::Boolean: return &JavaType::Boolean;
                case PrimitiveTypeNode::Kind::Byte:    return &JavaType::Byte;
                case PrimitiveTypeNode::Kind::Char:    return &JavaType::Char;
                case PrimitiveTypeNode::Kind::Int:     return &JavaType::Int;
                case PrimitiveTypeNode::Kind::Short:   return &JavaType::Short;
            }
        } else if (t->getNodeType() == NodeType::ArrayType) {
            auto* a = static_cast<const ArrayTypeNode*>(t);
            return getArrayType(getTypeFromNode(a->componentType));
        } else if (t->getNodeType() == NodeType::ClassOrInterfaceType) {
            auto* c = static_cast<const ClassOrInterfaceTypeNode*>(t);
            if (!c || !c->resolvedDeclaration) {
                throw std::runtime_error("Unresolved type in TypeManager: " + (c ? c->name->toString() : "<null>"));
            }
            // std::cerr << "Resolved type for node " << c->name->toString() << ": " << c->resolvedDeclaration->resolvedType << "\n";
            return c->resolvedDeclaration->resolvedType;
        }
        throw std::runtime_error("Unhandled type node in TypeManager");
    }

    void clear() {
        generatedTypes.clear();
        arrayTypeCache.clear();
    }

    std::vector<JavaType*> getAllTypes() const {
        std::vector<JavaType*> result;
        for (const auto& typePtr : generatedTypes) {
            result.push_back(typePtr.get());
        }
        return result;
    }

    void generateClassTreeIDs() {
        classTreeIDCache.clear();
        std::unordered_map<JavaType*, std::vector<JavaType*>> children;
        for (const auto& typePtr : generatedTypes) {
            JavaType* type = typePtr.get();
            if (type->directSuperClass) {
                if (!children.count(type->directSuperClass)) {
                    children[type->directSuperClass] = std::vector<JavaType*>();
                }
                children[type->directSuperClass].push_back(type);
            }
        }
        int nextID = 0;
        classTreeIDDFSHelper(children, JavaType::Object, nextID);
    }

    void classTreeIDDFSHelper(std::unordered_map<JavaType*, std::vector<JavaType*>>& children, JavaType* current, int& nextID) {
        int currentID = nextID++;
        if (children.count(current)) {
            for (JavaType* child : children[current]) {
                classTreeIDDFSHelper(children, child, nextID);
            }
        }
        classTreeIDCache[current] = {currentID, nextID++};
    }

    void resolveArrayTypes() {
        for (const auto& [componentType, arrayType] : arrayTypeCache) {
            arrayType->superTypes.clear();
            arrayType->superTypes.push_back(JavaType::Object);
            arrayType->superTypes.push_back(JavaType::Cloneable);
            arrayType->superTypes.push_back(JavaType::Serializable);
            arrayType->directSuperClass = JavaType::Object;
            arrayType->methods.clear();
            for (const auto& [signature, method] : JavaType::Object->methods) {
                arrayType->methods[signature] = method;
            }
        }
    }
};

}

#endif // AST_TYPEMANAGER_HPP