#ifndef AST_JAVA_TYPE_HPP
#define AST_JAVA_TYPE_HPP
#include <functional>
#include <iostream>
#include <unordered_map>
#include <string>

namespace ast {

class JavaType;

struct Signature {
    std::string name;
    std::vector<JavaType*> paramTypes;

    bool operator==(const Signature& other) const {
        return name == other.name && paramTypes == other.paramTypes;
    }

    std::string toString() const;
};

}

namespace std {

template<>
struct hash<ast::Signature> {
    std::size_t operator()(const ast::Signature& signature) const noexcept {
        std::size_t seed = std::hash<std::string>{}(signature.name);
        for (const auto* paramType : signature.paramTypes) {
            seed ^= std::hash<const ast::JavaType*>{}(paramType) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        }
        return seed;
    }
};

}

namespace ast {

struct MethodInfo {
    union MethDecl {
        class MethodDeclarationNode* methodDecl;
        class ConstructorDeclarationNode* ctorDecl;
    };
    MethDecl decl{};
    JavaType* declaringType{}; // Set during hierarchy checking, used for protected access checks
    JavaType* returnType{}; // null if return type is void
    Signature signature;
    bool abs{};
    bool stat{};
    bool fin{};
    bool pub{};
    bool prot{};
    bool native{};
    bool isConstructor{};
    bool isInterfacedMethod{};

    std::string getFullName() const;
};

struct FieldInfo {
    class VariableDeclarationFragmentNode* decl{};
    JavaType* type{};
    JavaType* declaringType{}; // Set during hierarchy checking, used for protected access checks
    std::string name;
    bool stat{};
    bool fin{};
    bool pub{};
    bool prot{};

    std::string getFullName() const;
};


class JavaType {
public:
    enum Kind {
        Primitive,
        Reference,
        Array
    };
    std::string package;
    class TypeDeclarationNode* declaration; // null if this is a primitive type or an array type
    std::string name; // For primitive types, this is the type name. For reference types, this is the simple name.
    std::unordered_map<Signature, MethodInfo> methods; // Methods in CONTAIN
    std::unordered_map<std::string, FieldInfo> fields; // Fields in CONTAIN
    std::unordered_map<Signature, MethodInfo> constructors; // Constructors
    std::vector<JavaType*> superTypes;
    JavaType* directSuperClass = nullptr; // null for interfaces and Object, set during hierarchy checking for classes
    JavaType* arrayComponentType = nullptr; // Non-null if this is an array type, points to the component type
    Kind kind = Reference;
    bool isInterface = false;
    bool fin = false;
    bool abs = false;
    bool resolved = false; // Whether this type has been fully resolved (used to prevent infinite recursion in resolution)
    int elementSize = 4;

    static JavaType Boolean;
    static JavaType Byte;
    static JavaType Char;
    static JavaType Short;
    static JavaType Int;
    static JavaType Null;
    static JavaType* String;
    static JavaType* Object;
    static JavaType* Cloneable; // Placeholder, will be set to the actual type during hierarchy checking
    static JavaType* Serializable; // Placeholder, will be set to the actual type during hierarchy checking
    static JavaType Void; // Not a real type, but used to represent the return type of void methods rather than nullptr

    bool isSubtypeOf(const JavaType& other) const {
        // std::cerr << "Comparing " << name << " (" << this << ") to " << other.name << " (" << &other << ")\n";
        if (this == &other) return true;
        for (const auto* superType : superTypes) {
            if (superType->isSubtypeOf(other)) return true;
        }
        return false;
    }

    bool operator==(const JavaType& other) const {
        return kind == other.kind && declaration == other.declaration && name == other.name && arrayComponentType == other.arrayComponentType && name == other.name;
    }

    bool canWidenToReference(const JavaType& other) const {
        if (*this == other) return true;
        if ((other.kind == Kind::Reference || other.kind == Kind::Array) && this == &JavaType::Null) return true; // null is assignable to any reference type
        if (kind == Kind::Reference && &other == JavaType::Object) return true;
        if (isInterface && &other == JavaType::Object) return true;
        if (kind == Kind::Array && (
            &other == JavaType::Object ||
            &other == JavaType::Cloneable ||
            &other == JavaType::Serializable
        )) return true; // All array types are subtypes of Object
        if (kind == Kind::Array && other.kind == Kind::Array) {
            if (arrayComponentType->kind == Kind::Reference && other.arrayComponentType->kind == Kind::Reference) {
                return arrayComponentType->canWidenToReference(*other.arrayComponentType);
            }
        }
        return isSubtypeOf(other);
    }


    bool canWidenToNumeric(const JavaType& other) const {
        if (*this == JavaType::Byte && (other == JavaType::Short || other == JavaType::Int)) return true;
        if (*this == JavaType::Short && other == JavaType::Int) return true;
        if (*this == JavaType::Char && other == JavaType::Int) return true;
        return false;
    }
    bool canNarrowToNumeric(const JavaType& other) const {
        if (this == &JavaType::Byte && &other == &JavaType::Char) return true;
        if (this == &JavaType::Short && (&other == &JavaType::Char || &other == &JavaType::Byte)) return true;
        if (this == &JavaType::Char && (&other == &JavaType::Byte || &other == &JavaType::Short)) return true;
        if (this == &JavaType::Int && (&other == &JavaType::Char || &other == &JavaType::Byte || &other == &JavaType::Short)) return true;
        return false;
    }

    bool assignableTo(const JavaType& other) const {
        bool result = canWidenToReference(other) || canWidenToNumeric(other);
        // std::cerr << "" << name << " assignable to " << other.name << "?: " << result << "\n";
        return canWidenToReference(other) || canWidenToNumeric(other);
    }

    bool castableTo(const JavaType& other) const {
        if (*this == other) return true;
        if (kind != Kind::Primitive && other.kind != Kind::Primitive) {
            return assignableTo(other) || other.assignableTo(*this) ||
                (isInterface && other.isInterface) ||
                (isInterface && other.kind == Kind::Reference && !other.fin) ||
                (other.isInterface && kind == Kind::Reference && !fin);
        } else if (isNumeric() && other.isNumeric()) {
            return canWidenToNumeric(other) || canNarrowToNumeric(other);
        } else if (kind == Kind::Array && other.kind == Kind::Array && arrayComponentType->kind == Kind::Reference && other.arrayComponentType->kind == Kind::Reference) {
            return arrayComponentType->castableTo(*other.arrayComponentType);
        }
        return false;
    }

    bool isNumeric() const {
        return *this == JavaType::Byte || *this == JavaType::Short || *this == JavaType::Int || *this == JavaType::Char;
    }

    bool containsMethod(const Signature& sig) const {
        return methods.count(sig) > 0;
    }

    bool containsField(const std::string& name) const {
        return fields.count(name) > 0;
    }

    bool allAbstract() const {
        for (const auto& [sig, method] : methods) {
            if (!method.abs) return false;
        }
        return true;
    }

    std::string getFullName() const;
};

}

#endif // SEMANTICS_JAVA_TYPE_HPP