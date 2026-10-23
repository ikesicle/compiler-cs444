#ifndef GENERATION_GEN_VISITOR_HPP
#define GENERATION_GEN_VISITOR_HPP

#include "ast/ast_visitor.hpp"
#include "ast/nodes.hpp"
#include "ast/java_type.hpp"
#include "ast/type_manager.hpp"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <string>
#include <unordered_map>
#include <set>

namespace generation {

using namespace ast;

class GenVisitor : public ASTVisitor {
    JavaType* currentClass = nullptr;
    MethodInfo currentMethod;
    std::ostream* output;
    TypeManager& typeManager;
    // Need to map instance fields to their offsets in obj layout, and class fields and methods to offsets in class layout.
    // For every field or method, if it is newly declared in the class, give it an offset.
    // For inherited fields and methods, use the same offset as in the superclass.
    // Technically this means that we need to generate superclass layout before subclass layout. 
public:
    static const int HEADER_SIZE = 28;
    static const int IMT_SIZE = 4;
    struct ClassLayoutInfo {
        std::unordered_map<std::string, int> fieldOffsets;
        std::unordered_map<Signature, int> methodOffsets;
        std::vector<std::string> vtableLabels, requiredImports; // In vtable order, for easy generation of vtables
        int nextOffset = HEADER_SIZE; // Start at 20 to leave space for imt ptr, interface array ptr, class low, class high, array base kind and type
        std::unordered_map<int, MethodInfo> implementedInterfaceMethodIDs; // For generating IMT stubs.
        std::set<int> implementedInterfaceIDs; // For implementing instanceof and interface method dispatch.
    };

    struct InstanceLayoutInfo {
        std::unordered_map<std::string, int> fieldOffsets;
        int nextOffset = 4; // 4 bytes reserved for class ptr
    };

    typedef std::unordered_map<ASTNode*, int> OffsetTable;

    OffsetTable currentMethodOffsets;
    int nextAvailableStackOffset = -4;
    std::unordered_map<JavaType*, ClassLayoutInfo> layoutInfos;
    std::unordered_map<JavaType*, InstanceLayoutInfo> instanceLayoutInfos;
    std::unordered_map<JavaType*, int> interfaceIDs; // For implementing instanceof and interface method dispatch.
    std::unordered_map<Signature, int> interfaceMethodSignatureIDs; // For interface method dispatch
    std::unordered_map<Signature, int> methodSignatureIDs; // For direct method dispatch in cases where signature is needed to distinguish methods (e.g. overloaded methods)
    std::vector<std::pair<std::vector<unsigned char>, std::string>> stringConstants; // Map from string literal value to label, to avoid generating duplicate string constants
    int nextLabelNumber = 0; // For generating unique labels
    int nextInterfaceSignatureID = 0; // For assigning unique IDs to method signatures in interfaces for use in IMT lookup stubs
    int nextMethodSignatureID = 0; // For assigning unique IDs to method signatures for use in direct dispatch when needed (e.g. overloaded methods)
    int nextInterfaceID = 0;
    explicit GenVisitor(TypeManager& tm) : typeManager(tm) {}
    
    void execute(JavaType& mainClass);

    int getInterfaceMethodSignatureID(const Signature& sig) {
        if (!interfaceMethodSignatureIDs.contains(sig)) {
            std::cerr << "Assigning interface method signature ID " << nextInterfaceSignatureID << " to signature " << sig.toString() << "\n";
            interfaceMethodSignatureIDs[sig] = nextInterfaceSignatureID++;
        }
        return interfaceMethodSignatureIDs[sig];
    }

    int getInterfaceID(JavaType* type) {
        if (!interfaceIDs.contains(type)) {
            interfaceIDs[type] = nextInterfaceID++;
        }
        return interfaceIDs[type];
    }

    int getMethodSignatureID(const Signature& sig) {
        if (!methodSignatureIDs.contains(sig)) {
            methodSignatureIDs[sig] = nextMethodSignatureID++;
            // std::cerr << "Assigning method signature ID " << methodSignatureIDs[sig] << " to signature " << sig.toString() << "\n";
        }
        return methodSignatureIDs[sig];
    }

    std::string generateLabel() {
        return ".LABEL_" + std::to_string(nextLabelNumber++);
    }

    std::string classLabelOf(const JavaType& type) {
        return "CLASS_" + type.getFullName();
    }

    std::string methodLabelOf(const MethodInfo& method) {
        return "FUNC_" + method.getFullName() + "#" + std::to_string(getMethodSignatureID(method.signature));
    }

    std::string fieldLabelOf(const FieldInfo& field) {
        return "FIELD_V_" + field.getFullName();
    }

    std::string fieldReferenceLabelOf(FieldInfo field, JavaType* accessingType) {
        field.declaringType = accessingType; // For generating correct label for static field access from subclasses
        return "FIELD_R_" + field.getFullName();
    }

    std::string imtLabelOf(const JavaType& type) {
        return "IMT_CLASS_" + type.getFullName();
    }

    std::string imtStubLabelOf(const JavaType& type, int slot) {
        return "IMT_STUB_" + type.getFullName() + "#" + std::to_string(slot);
    }

    std::string interfaceArrayLabelOf(const JavaType& type) {
        return "INTERFACE_ARRAY_" + type.getFullName();
    }

    std::string staticInitLabelOf(const JavaType& type) {
        return "STATIC_INIT_" + type.getFullName();
    }

    std::string instanceInitLabelOf(const JavaType& type) {
        return "INSTANCE_INIT_" + type.getFullName();
    }

    // Generates code that takes a class base ptr in eax and checks if it's an instance of the given type. 1 for true and 0 for false.
    void generateCheckInstanceof(JavaType& type) {
        // Assume object is in eax
        std::string tmpLabel = generateLabel();
        std::string endLabel = generateLabel();
        if (type.kind == JavaType::Primitive) {
            throw std::runtime_error("Cannot generate instanceof check directly for primitive type");
        }
        
        *output << "    cmp eax, 0\n";
        *output << "    je " << endLabel << "\n";
        *output << "    mov eax, [eax]\n"; // Class infblock ptr in eax
        if (type.isInterface) {
            // IMT lookup
            *output << "    mov eax, [eax + 4] ; Interface Array inherit table\n";
            *output << "    mov eax, [eax + " << getInterfaceID(&type) * 4 << "]\n";
            *output << "    jmp " << endLabel << "\n";
        } else if (type.arrayComponentType) {
            // Check if it's an array and if the component 
            *output << "; instanceof " << type.getFullName() << "\n";
            int arrayTypeID;
            if (type.arrayComponentType == &JavaType::Boolean) {
                arrayTypeID = 1;
            } else if (type.arrayComponentType == &JavaType::Byte) {
                arrayTypeID = 2;
            } else if (type.arrayComponentType == &JavaType::Char) {
                arrayTypeID = 3;
            } else if (type.arrayComponentType == &JavaType::Short) {
                arrayTypeID = 4;
            } else if (type.arrayComponentType == &JavaType::Int) {
                arrayTypeID = 5;
            } else {
                arrayTypeID = 6; // Reference type
            }
            *output << "    mov ecx, [eax + 16] ; Array type ID\n";
            *output << "    cmp ecx, " << arrayTypeID << "\n";
            *output << "    je " << tmpLabel << "\n";
            *output << "    mov eax, 0\n"; // Fail
            *output << "    jmp " << endLabel << "\n";
            *output << tmpLabel << ":\n";
            if (arrayTypeID == 6) {
                *output << "    mov eax, [eax + 20] ; Array component type ptr\n";
                generateCheckInstanceof(*type.arrayComponentType);
            } else {
                *output << "    mov eax, 1\n"; // Pass for primitive array types
            }
            *output << endLabel << ":\n";
            return;
        }
        // Class hierarchy check
        *output << "    ; instanceof " << type.getFullName() << "\n";
        *output << "    mov ecx, [eax + 8] ; Class ID low\n";
        *output << "    mov edx, [eax + 12] ; Class ID high\n";
        *output << "    cmp ecx, " << typeManager.classTreeIDCache[&type].first << "\n";
        *output << "    jl " << tmpLabel << "\n";
        *output << "    cmp edx, " << typeManager.classTreeIDCache[&type].second << "\n";
        *output << "    jg " << tmpLabel << "\n";
        *output << "    mov eax, 1\n";
        *output << "    jmp " << endLabel << "\n";
        *output << tmpLabel << ":\n";
        *output << "    mov eax, 0\n";
        *output << endLabel << ":\n";
    }

    void generateExpressionValue(ExpressionNode* node) {
        // Leaves address of lvalue in eax
        //
        if (node == nullptr) {
            throw std::runtime_error("Cannot generate code for null expression node");
        }
        node->accept(*this);
        if (node->isLValue) {
            switch (node->resolvedType->elementSize) {
                case 1:
                    *output << "    movsx eax, byte [eax] ; zero-extend byte return value for assignable expression\n";
                    break;
                case 2:
                    if (node->resolvedType == &JavaType::Char) {
                        *output << "    movzx eax, word [eax] ; zero-extend char return value for assignable expression\n";
                    } else {
                        *output << "    movsx eax, word [eax] ; sign-extend short return value for assignable expression\n";
                    }
                    break;
                case 4:
                    *output << "    mov eax, [eax] ; int or reference return value for assignable expression\n";
                    break;
                default:
                    throw std::runtime_error("Invalid element size for type in expression");
            }
        }
    }

    std::string registerStringConstant(const std::vector<unsigned char>& value) {
        stringConstants.push_back({value, "STR_CONST_" + std::to_string(stringConstants.size())});
        return stringConstants.back().second;
    }

    void generateClassLayout(JavaType& type);
    bool visit(MethodDeclarationNode* node) override;
    bool visit(ConstructorDeclarationNode* node) override;
    bool visit(FieldDeclarationNode* node) override;
    bool visit(BlockNode* node) override;
    bool visit(EmptyStatementNode* node) override;
    bool visit(ExpressionStatementNode* node) override;
    bool visit(ReturnStatementNode* node) override;
    bool visit(IfStatementNode* node) override;
    bool visit(WhileStatementNode* node) override;
    bool visit(ForStatementNode* node) override;
    bool visit(VariableDeclarationStatementNode* node) override;
    bool visit(VariableDeclarationFragmentNode* node) override;
    bool visit(SingleVariableDeclarationNode* node) override;
    bool visit(SimpleNameNode* node) override;
    bool visit(QualifiedNameNode* node) override;
    bool visit(AssignmentExpressionNode* node) override;
    bool visit(InfixExpressionNode* node) override;
    bool visit(InstanceofExpressionNode* node) override;
    bool visit(PrefixExpressionNode* node) override;
    bool visit(CastExpressionNode* node) override;
    bool visit(VariableDeclarationExpressionNode* node) override;
    bool visit(ArrayCreationExpressionNode* node) override;
    bool visit(IntegerLiteralNode* node) override;
    bool visit(BooleanLiteralNode* node) override;
    bool visit(CharacterLiteralNode* node) override;
    bool visit(StringLiteralNode* node) override;
    bool visit(NullLiteralNode* node) override;
    bool visit(ThisExpressionNode* node) override;
    bool visit(ParenthesizedExpressionNode* node) override;
    bool visit(FieldAccessExpressionNode* node) override;
    bool visit(MethodInvocationExpressionNode* node) override;
    bool visit(InstanceCreationExpressionNode* node) override;
    bool visit(ArrayAccessExpressionNode* node) override;

    // Expressions



}; // namespace generation

}

#endif // GENERATION_GEN_VISITOR_HPP