#include "ast/java_type.hpp"

namespace ast {

JavaType JavaType::Boolean {

    .package = "",
    .name = "boolean",
    .kind = Primitive,
    .resolved = true
};
JavaType JavaType::Byte{
    .package = "",
    .name = "byte",
    .kind = Primitive,
    .resolved = true,
    .elementSize = 1
};
JavaType JavaType::Char{
    .package = "",
    .name = "char",
    .kind = Primitive,
    .resolved = true,
    .elementSize = 2
};
JavaType JavaType::Short{
    .package = "",
    .name = "short",
    .kind = Primitive,
    .resolved = true,
    .elementSize = 2
};
JavaType JavaType::Int{
    .package = "",
    .name = "int",
    .kind = Primitive,
    .resolved = true
};

JavaType JavaType::Null{
    .package = "",
    .name = "null",
    .kind = Reference,
    .resolved = true
};

JavaType JavaType::Void{
    .package = "",
    .name = "void",
    .kind = Primitive,
    .resolved = true
};

JavaType* JavaType::String = nullptr; // Placeholder
JavaType* JavaType::Object = nullptr; // Placeholder
JavaType* JavaType::Cloneable = nullptr; // Placeholder
JavaType* JavaType::Serializable = nullptr; // Placeholder

std::string Signature::toString() const{
    std::string result = name + "(";
    for (size_t i = 0; i < paramTypes.size(); i++) {
        result += paramTypes[i] ? paramTypes[i]->name : "void";
        if (i < paramTypes.size() - 1) result += ", ";
    }
    result += ")";
    return result;
}

std::string MethodInfo::getFullName() const
{
    return declaringType->getFullName() + "." + signature.name;
}

std::string JavaType::getFullName() const
{
    if (package.empty()) {
        return name;
    } else {
        return package + "." + name;
    }
}

std::string FieldInfo::getFullName() const
{
    return declaringType->getFullName() + "." + name;
}

}