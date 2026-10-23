#include "generation/gen_visitor.hpp"
#include "ast/java_type.hpp"
#include "ast/nodes.hpp"
#include <cassert>
#include <cctype>
#include <iostream>
#include <ranges>
#include <string>

namespace generation {

/*
IMT Array Ptr - 0
Interface Inherit Array Ptr - 4
Class ID low - 8
Class ID high - 12
Array type ID - 16 - one of  0, 1, 2, 3, 4, 5, 6
    - 0 - not an array
    - 1 - bool
    - 2 - byte
    - 3 - char
    - 4 - short
    - 5 - int
    - 6 - reference
Array base - 20 (for reference array types, points to class of array elements, other cases this is just 0)
interface ID - 24 - -1 if not an interface type, otherwise ID of this interface
Fields and Method Ptrs (construct from LayoutInfo) - HEADER_SIZE+

Imt Array (IMT_SIZE entries)
Interface inherit array
Method, constructor, and static field implementations / storage locations
IMT stubs
*/

void GenVisitor::execute(JavaType& mainClass) {
    std::ofstream outputFile;
    typeManager.resolveArrayTypes();
    typeManager.generateClassTreeIDs();
    for (const auto& type : typeManager.getAllTypes()) {
        if (type->isInterface) {
            layoutInfos[type] = ClassLayoutInfo{}; // Interfaces don't have vtables or fields so we can just set empty layout info for them
            instanceLayoutInfos[type] = InstanceLayoutInfo{};
            for (auto& [signature, method] : type->methods) {
                if (method.isInterfacedMethod) {
                    getInterfaceMethodSignatureID(signature);
                }
            }
            
        } else {
            generateClassLayout(*type);
        }
    }
    std::filesystem::create_directory("output");
    for (const auto& type : typeManager.getAllTypes()) {
        stringConstants.clear();
        outputFile = std::ofstream("output/JAVA_" + (type->package.empty() ? "" : type->package + ".") + type->name + ".s");
        output = &outputFile;
        currentClass = type;
        ClassLayoutInfo layoutInfo = layoutInfos[currentClass];
        *output << "SECTION .data\n";
        *output << "global " << classLabelOf(*currentClass) << "\n";
        *output << "extern __exception\n";
        *output << "extern __check_assignable\n";
        *output << "extern __malloc\n";
        *output << classLabelOf(*currentClass) << ":\n";
        *output << "    dd " << imtLabelOf(*currentClass) << " ; IMT\n";
        *output << "    dd " << interfaceArrayLabelOf(*currentClass) << " ; Interface array\n";
        // Low, high
        *output << "    dd " << typeManager.classTreeIDCache[currentClass].first << " ; Class ID low\n";
        *output << "    dd " << typeManager.classTreeIDCache[currentClass].second << " ; Class ID high\n";
        // end
        int arrayTypeID = 0;
        if (currentClass->arrayComponentType) {
            // Check if it's an array and if the component type matches
            if (currentClass->arrayComponentType == &JavaType::Boolean) {
                arrayTypeID = 1;
            } else if (currentClass->arrayComponentType == &JavaType::Byte) {
                arrayTypeID = 2;
            } else if (currentClass->arrayComponentType == &JavaType::Char) {
                arrayTypeID = 3;
            } else if (currentClass->arrayComponentType == &JavaType::Short) {
                arrayTypeID = 4;
            } else if (currentClass->arrayComponentType == &JavaType::Int) {
                arrayTypeID = 5;
            } else {
                arrayTypeID = 6; // Class type
            }
            *output << "    dd " << arrayTypeID << " ; Array Type ID\n";
            if (arrayTypeID == 6) {
                *output << "extern " << classLabelOf(*currentClass->arrayComponentType) << "\n";
                *output << "    dd " << classLabelOf(*currentClass->arrayComponentType) << " ; AC " << currentClass->arrayComponentType->name << "\n";
            }
            else *output << "    dd 0 ; Primitive type\n";
        } else {
            *output << "    dd 0 ; Not an array\n";
            *output << "    dd 0 ; No component type\n";
        }
        *output << "    dd " << (currentClass->isInterface ? getInterfaceID(currentClass) : -1) << " ; Interface ID\n";
        for (auto& import : layoutInfo.requiredImports) {
            *output << "extern " << import << "\n";
        }
        for (int i = 0; i < layoutInfo.vtableLabels.size(); i++) {
            *output << "    dd " << layoutInfo.vtableLabels[i] << "\n";
        }

        *output << imtLabelOf(*currentClass) << ":\n";
        for (int i = 0; i < nextInterfaceSignatureID; i++) {
            if (!layoutInfo.implementedInterfaceMethodIDs.contains(i)) {
                *output << "    dd 0 ; Empty IMT slot\n";
            } else {
                *output << "    dd " << methodLabelOf(layoutInfo.implementedInterfaceMethodIDs.at(i)) << "\n";
            }
        }
        for (auto& [fieldName, field] : currentClass->fields) {
            // Static fields go in class infoblock
            if (!field.stat) continue;
            if (field.declaringType != currentClass) continue;
            *output << "global " << fieldLabelOf(field) << "\n";
            *output << fieldLabelOf(field) << ": dd 0 ; static " << fieldName << "\n";
            // Will need to generate initializer later if there's an initializer expression, but for now just initialize to 0
        }
        *output << interfaceArrayLabelOf(*currentClass) << ":\n";
        for (int i = 0; i < nextInterfaceID; i++) {
            if (layoutInfo.implementedInterfaceIDs.contains(i)) {
                *output << "    dd 1\n";
            } else {
                *output << "    dd 0\n";
            }
        }
        *output << "SECTION .text\n";
        for (auto& [signature, method] : currentClass->methods) {
            if (method.abs || method.declaringType != currentClass) continue;
            method.decl.methodDecl->accept(*this);
        }
        for (auto& [signature, constructor] : currentClass->constructors) {
            constructor.decl.ctorDecl->accept(*this);
        }
        
        if (currentClass->kind == JavaType::Reference) {
            *output << "global " << staticInitLabelOf(*currentClass) << "\n";
            *output << staticInitLabelOf(*currentClass) << ":\n";
            // Static initializer code goes here
            *output << "    push ebp\n";
            *output << "    mov ebp, esp\n";
            for (BodyDeclarationNode* bodyDecl : currentClass->declaration->bodyDeclarations) {
                if (bodyDecl->getNodeType() != NodeType::FieldDeclaration) continue;
                FieldDeclarationNode* fieldDecl = static_cast<FieldDeclarationNode*>(bodyDecl);
                if (fieldDecl->isStatic()) {
                    fieldDecl->accept(*this);
                }
            }
            *output << "    mov esp, ebp\n";
            *output << "    pop ebp\n";
            *output << "    ret 4\n";
            

             // Instance initializer
            *output << instanceInitLabelOf(*currentClass) << ":\n";
            // Static initializer code goes here
            *output << "    push ebp\n";
            *output << "    mov ebp, esp\n";
            currentMethodOffsets.clear();
            currentMethodOffsets[nullptr] = 8; // Spoofing "this" pointer as first local variable for field initializers to reference
            for (BodyDeclarationNode* bodyDecl : currentClass->declaration->bodyDeclarations) {
                if (bodyDecl->getNodeType() != NodeType::FieldDeclaration) continue;
                FieldDeclarationNode* fieldDecl = static_cast<FieldDeclarationNode*>(bodyDecl);
                if (!fieldDecl->isStatic()) {
                    fieldDecl->accept(*this);
                }
            }
            *output << "    mov esp, ebp\n";
            *output << "    pop ebp\n";
            *output << "    ret 4\n";
        }
        *output << "SECTION .data ; String constants\n";
        *output << "extern " << classLabelOf(*JavaType::String) << "\n";
        *output << "extern " << classLabelOf(*typeManager.getArrayType(&JavaType::Char)) << "\n";
        for (auto& [strv, strLabel] : stringConstants) {
            *output << strLabel << ": \n";
            
            *output << "    dd " << classLabelOf(*JavaType::String) << "\n";
            *output << "    dd " << strLabel << "chars\n";
            *output << strLabel << "chars" << ": \n";
            *output << "    dd " << classLabelOf(*typeManager.getArrayType(&JavaType::Char)) << "\n";
            *output << "    dd " << strv.size() << "\n";
            for (unsigned char c : strv) {
                *output << "    dw " << (int)c << " ; '";
                if (std::isprint(c)) {
                    *output << (char)c;
                } else {
                    *output << "\\x" << std::hex << (int)c << std::dec;
                }
                *output << "'\n";
            }
        }
        currentClass = nullptr;
    }

    outputFile = std::ofstream("output/main.s");
    std::string inheritLabel = generateLabel();
    std::string trueLabel = generateLabel();
    std::string falseLabel = generateLabel();
    std::string endLabel = generateLabel();
    
    *output << "global _start\n";
    *output << "extern __exception\n";
    *output << "global __check_assignable\n";
    // Assignability of base type: either it's a subclass or the array type is an interface so we need to see if the interface is implemented
    *output << "__check_assignable:\n";
    // eax has value to assign, ecx has assignment target's required type class ptr
    *output << "    push eax\n"; 
    *output << "    push ebx\n"; 
    *output << "    cmp eax, 0\n";
    *output << "    je " << endLabel << " ; null doesn't need to be checked\n";
    *output << "    mov eax, [eax]\n"; // Class infoblock ptr of value to assign in eax
    *output << "    mov edx, [ecx + 24] ; Required type interface ID\n";
    *output << "    cmp edx, -1\n";
    *output << "    je " << inheritLabel << "\n";
    *output << "    mov ebx, [eax + 4] ; Interface array ptr\n";
    *output << "    imul edx, 4\n";
    *output << "    add ebx, edx\n";
    *output << "    mov ebx, [ebx]\n";
    *output << "    cmp ebx, 0\n";
    *output << "    je " << falseLabel << "\n";
    *output << "    jmp " << endLabel << "\n";
    *output << inheritLabel << ":\n";
    *output << "    mov edx, [eax + 8] ; Class ID low\n";
    *output << "    mov ebx, [ecx + 8] ; Required type ID low\n";
    *output << "    cmp edx, ebx\n";
    *output << "    jle " << falseLabel << "\n";
    *output << "    mov edx, [eax + 12] ; Class ID high\n";
    *output << "    mov ebx, [ecx + 12] ; Required type ID high\n";
    *output << "    cmp edx, ebx\n";
    *output << "    jge " << falseLabel << "\n";
    *output << "    jmp " << trueLabel << "\n";
    *output << falseLabel << ":\n";
    *output << "    mov eax, 0\n";
    *output << "    jmp " << endLabel << "\n";
    *output << trueLabel << ":\n";
    *output << "    mov eax, 1\n";
    *output << endLabel << ":\n";
    *output << "    pop ebx\n"; 
    *output << "    pop eax\n"; 
    *output << "    ret\n"; 
    *output << "_start:\n";
    Signature mainSig = {"test", {}};
    if (!mainClass.methods.contains(mainSig)) {
        throw std::runtime_error("Main method with signature 'test()' not found in main class '" + mainClass.name + "'");
    }
    MethodInfo& mainMethod = mainClass.methods[mainSig];
    if (!mainMethod.stat || mainMethod.returnType != &JavaType::Int) {
        throw std::runtime_error("Main method must be static and return int");
    }
    for (auto& type : typeManager.getAllTypes()) {
        if (type->kind != JavaType::Reference || type->isInterface) continue;
        *output << "extern " << staticInitLabelOf(*type) << "\n";
        *output << "    call " << staticInitLabelOf(*type) << "\n";
    }
    *output << "extern " << methodLabelOf(mainClass.methods[mainSig]) << "\n";
    *output << "    call " << methodLabelOf(mainClass.methods[mainSig]) << "\n";
    *output << "    mov ebx, eax\n"; // Return value of main in ebx for exit syscall
    *output << "    mov eax, 1\n";
    *output << "    int 0x80\n";
}

void GenVisitor::generateClassLayout(JavaType& type) {
     if (layoutInfos.contains(&type)) {
         return; // Already generated layout for this class
     }
     ClassLayoutInfo base{};
     InstanceLayoutInfo instanceBase{};
     if (type.directSuperClass) {
        generateClassLayout(*type.directSuperClass);
        base = layoutInfos[type.directSuperClass];
        instanceBase = instanceLayoutInfos[type.directSuperClass];
     }
     base.requiredImports.clear();
    //  std::cerr << "Generating layout for class " << type.name << " with superclass " << (type.directSuperClass ? type.directSuperClass->name : "<none>") << "\n";
     for (auto& [signature, method] : type.methods) {
        if (method.isConstructor) continue; // Constructors are not inherited and don't go in the vtable
        // std::cerr << "Processing method " << signature.toString() << " declared in " << method.declaringType->name << ": is abs? " << method.abs << "\n";
        if (method.declaringType == &type) {
            if (!base.methodOffsets.contains(signature)) {
                base.methodOffsets[signature] = base.nextOffset;
                base.vtableLabels.push_back("0 ; no implementation for " + signature.name); // Placeholder
                base.nextOffset += 4;
            }
            if (method.isInterfacedMethod) {
                int imethodID = getInterfaceMethodSignatureID(signature);
                base.implementedInterfaceMethodIDs[imethodID] = method;
            }
            
            if (!method.abs) {
                int idx = (base.methodOffsets[signature] - HEADER_SIZE) / 4; // Get index in vtable
                assert(idx >= 0 && idx < base.vtableLabels.size());
                base.vtableLabels[idx] = methodLabelOf(method);
            }
        } else {
            // std::cerr << "Type " << type.name << " inherits method " << signature.name << " from " << method.declaringType->name << "\n";
            base.requiredImports.push_back(methodLabelOf(method));
        }
     }
     for (auto& [fieldName, field] : type.fields) {
        if (field.declaringType == &type) {
            if (!field.stat) {
                std::string fqName = type.getFullName() + "." + fieldName;
                if (!instanceBase.fieldOffsets.contains(fqName)) {
                    instanceBase.fieldOffsets[fqName] = instanceBase.nextOffset;
                    std::cerr << "Assigning instance field " << fqName << " offset " << instanceBase.nextOffset << "\n";
                    instanceBase.nextOffset += 4; // All instance fields are 4 bytes for simplicity for now
                }
            } else {
                if (!base.fieldOffsets.contains(fieldName)) {
                    base.fieldOffsets[fieldName] = base.nextOffset;
                    base.vtableLabels.push_back("0 ; no value for static field " + fieldName); // Placeholder
                    base.nextOffset += 4;
                }
                int idx = (base.fieldOffsets[fieldName] - HEADER_SIZE) / 4; // Get index in vtable
                base.vtableLabels[idx] = fieldLabelOf(field); // Placeholder
            }
        } else {
            if (field.stat) base.requiredImports.push_back(fieldLabelOf(field));
            else {
                std::string hoist = type.getFullName() + "." + fieldName;
                std::string tgt = field.getFullName();
                instanceBase.fieldOffsets[hoist] = instanceBase.fieldOffsets[tgt];
            }
        }
     }
     for (auto& stype : type.superTypes) {
        if (stype->isInterface) {
            int interfaceID = getInterfaceID(stype);
            base.implementedInterfaceIDs.insert(interfaceID);
        }
     }
     layoutInfos[&type] = base;
     instanceLayoutInfos[&type] = instanceBase;
}

bool GenVisitor::visit(MethodDeclarationNode* node) {
    currentMethod = currentClass->methods[typeManager.getSignature(node)];
    // At this point params are already on stack.
    *output << "; " << currentMethod.signature.toString() << "\n";
    *output << "global " << methodLabelOf(currentMethod) << "\n";
    *output << methodLabelOf(currentMethod) << ":\n";
    *output << "    push ebp\n";
    *output << "    mov ebp, esp\n";
    // Method body generation
    currentMethodOffsets.clear();
    int bpOffset = 8; // Track offset from BP for params (+4 for return address, +4 again for sp)
    for (auto param : node->parameters | std::views::reverse) {
        currentMethodOffsets[param] = bpOffset; // Params are at positive offsets from BP
        bpOffset += 4; // All params are 4 bytes for simplicity for now
    }
    if (!currentMethod.stat) {
        // This pointer, this is rather hacky but it works?
        currentMethodOffsets[nullptr] = bpOffset;
    }
    int paramCount = currentMethodOffsets.size();
    nextAvailableStackOffset = -4;
    if (currentMethod.native) {
        std::string nativeLabel = "NATIVE" + currentMethod.declaringType->package + "." + currentMethod.declaringType->name + "." + currentMethod.signature.name;
        *output << "    extern " << nativeLabel << "\n";
        *output << "    mov eax, [ebp + " << currentMethodOffsets[node->parameters[0]] << "] ; Load only parameter\n";
        *output << "    call " << nativeLabel << "\n";
    } else {
        node->body->accept(*this);
    }
    *output << methodLabelOf(currentMethod) << "_return:\n";
    *output << "    mov esp, ebp\n";
    *output << "    pop ebp\n";
    *output << "    ret " << (paramCount) * 4 << "\n";
    return false;
}

bool GenVisitor::visit(ConstructorDeclarationNode* node) {
    currentMethod = currentClass->constructors[typeManager.getSignature(node)];
    // At this point params are already on stack.
    *output << "; " << currentMethod.signature.toString() << "\n";
    *output << "global " << methodLabelOf(currentMethod) << "\n";
    *output << methodLabelOf(currentMethod) << ":\n";
    *output << "    push ebp\n";
    *output << "    mov ebp, esp\n";
    // Method body generation
    currentMethodOffsets.clear();
    int bpOffset = 8; // Track offset from BP for params (+4 for return address)
    for (auto param : node->parameters | std::views::reverse) {
        currentMethodOffsets[param] = bpOffset; // Params are at positive offsets from BP
        bpOffset += 4; // All params are 4 bytes for simplicity for now
    }
    currentMethodOffsets[nullptr] = bpOffset;
    int paramCount = currentMethodOffsets.size();
    nextAvailableStackOffset = -4;
    if (currentClass->directSuperClass != nullptr) {
        // Call 0-arg ctor of superclass, passing this pointer
        *output << "    mov eax, [ebp + " << currentMethodOffsets[nullptr] << "] ; Load this pointer for superclass ctor call\n";
        *output << "    push eax\n";
        JavaType* superClass = currentClass->directSuperClass;
        Signature superCtorSig = {superClass->name, {}};
        *output << "extern " << methodLabelOf(superClass->constructors[superCtorSig]) << "\n";
        *output << "    call " << methodLabelOf(superClass->constructors[superCtorSig]) << "\n";
    }
    *output << "    mov eax, [ebp + " << currentMethodOffsets[nullptr] << "] ; Load this pointer for superclass ctor call\n";
    *output << "    push eax\n";
    *output << "    call " << instanceInitLabelOf(*currentClass) << "\n";
    node->body->accept(*this);
    *output << methodLabelOf(currentMethod) << "_return:\n";
    *output << "    mov esp, ebp\n";
    *output << "    pop ebp\n";
    *output << "    ret " << paramCount * 4 << "\n";
    return false;
}

bool GenVisitor::visit(FieldDeclarationNode* node) {
    for (auto fragment : node->fragments) {
        if (fragment->initializer) {
            generateExpressionValue(fragment->initializer);
            if (node->isStatic()) {
                *output << "    mov ebx, " << fieldLabelOf(currentClass->fields[fragment->name->getSimpleName()]) << " ; static field initializer\n";
                *output << "    mov [ebx], eax ; static field initializer\n";
            } else {
                *output << "    mov edx, eax ; instance field initializer value\n";
                *output << "    mov eax, [ebp + " << currentMethodOffsets[nullptr] << "] ; Load this pointer for instance field initializer\n";
                std::string fqName = currentClass->getFullName() + "." + fragment->name->getSimpleName();
                *output << "    add eax, " << instanceLayoutInfos[currentClass].fieldOffsets[fqName] << " ; instance field initializer\n";
                *output << "    mov [eax], edx ; instance field initializer\n";
            }
        } else {
            if (node->isStatic()) {
                *output << "    mov ebx, " << fieldLabelOf(currentClass->fields[fragment->name->getSimpleName()]) << " ; static field initializer\n";
                *output << "    mov [ebx], dword 0 ; static field initializer\n";
            }
        }
    }
    return false;
}

bool GenVisitor::visit(BlockNode* node) {
    int savedOffset = nextAvailableStackOffset;
    for (auto statement : node->statements) {
        statement->accept(*this);
    }
    if (nextAvailableStackOffset != savedOffset) {
        *output << "    add esp, " << (savedOffset - nextAvailableStackOffset) << " ; Clean up stack for local variables declared in block\n";
    }
    nextAvailableStackOffset = savedOffset; // Reset stack offset after block to free local variables
    return false;
}

bool GenVisitor::visit(EmptyStatementNode* node) {
    return true;
}

bool GenVisitor::visit(ExpressionStatementNode* node) {
    return true;
}

bool GenVisitor::visit(ReturnStatementNode* node) {
    if (node->expression) generateExpressionValue(node->expression);
    *output << "    jmp " << methodLabelOf(currentMethod) << "_return\n";
    return false;
}

bool GenVisitor::visit(IfStatementNode* node) {
    std::string endLabel = generateLabel();
    std::string elseLabel = generateLabel();

    generateExpressionValue(node->condition);
    *output << "    cmp eax, 0\n";
    *output << "    je " << elseLabel << "\n";
    node->thenBranch->accept(*this);
    *output << "    jmp " << endLabel << "\n";
    *output << elseLabel << ": ; else branch\n" ;
    if (node->elseBranch) {
        node->elseBranch->accept(*this);
    }
    *output << endLabel << ": ; end of if statement\n";
    return false;
}

bool GenVisitor::visit(WhileStatementNode* node) {
    std::string startLabel = generateLabel();
    std::string endLabel = generateLabel();
    *output << startLabel << ": ; start of while loop\n";
    generateExpressionValue(node->condition);
    *output << "    cmp eax, 0\n";
    *output << "    je " << endLabel << "\n";
    node->body->accept(*this);
    *output << "    jmp " << startLabel << "\n";
    *output << endLabel << ": ; end of while loop\n";
    return false;
}

bool GenVisitor::visit(ForStatementNode* node) {
    std::string startLabel = generateLabel();
    std::string endLabel = generateLabel();
    int savedOffset = nextAvailableStackOffset;
    for (auto init : node->initializers) {
        init->accept(*this);
    }
    *output << startLabel << ": ; start of for loop\n";
    generateExpressionValue(node->condition);
    *output << "    cmp eax, 0\n";
    *output << "    je " << endLabel << "\n";
    node->body->accept(*this);
    for (auto update : node->updates) {
        update->accept(*this);
    }
    *output << "    jmp " << startLabel << "\n";
    *output << endLabel << ": ; end of for loop\n";
    if (nextAvailableStackOffset != savedOffset) {
        *output << "    add esp, " << (savedOffset - nextAvailableStackOffset) << " ; Clean up stack for local variables declared in for loop\n";
    }
    nextAvailableStackOffset = savedOffset; // Reset stack offset after block to free local variables
    return false;

}

bool GenVisitor::visit(VariableDeclarationStatementNode* node) {
    for (auto fragment : node->fragments) {
        fragment->accept(*this);
    }
    return false;
}

bool GenVisitor::visit(VariableDeclarationFragmentNode* node) {
    generateExpressionValue(node->initializer);
    *output << "    push eax ; local variable declare " << node->name->getSimpleName() << "\n";
    currentMethodOffsets[node] = nextAvailableStackOffset;
    nextAvailableStackOffset -= 4;
    return false;
}

bool GenVisitor::visit(SingleVariableDeclarationNode* node) {
    generateExpressionValue(node->initializer);
    *output << "    push eax ; local variable declare " << node->name->getSimpleName() << "\n";
    currentMethodOffsets[node] = nextAvailableStackOffset;
    nextAvailableStackOffset -= 4;
    return false;
}

bool GenVisitor::visit(SimpleNameNode *node) {
    // If it gets called here it should only ever be an expression name
    if (node->variant != NameNode::Expression) {
        throw std::runtime_error("SimpleNameNode visitor called on non-expression name");
    }
    // Loading either a local variable or a field
    if (currentMethodOffsets.contains(node->resolvedReference)) {
        int offset = currentMethodOffsets[node->resolvedReference];
        *output << "    mov eax, ebp\n";
        *output << "    add eax, " << offset << "\n";
        // std::cerr << "Accessing local variable " << node->getSimpleName() << " at offset " << offset << " from ebp\n";
    } else {
        FieldInfo& field = currentClass->fields[node->getSimpleName()];
        // Field access with implicit this
        if (field.stat) {
            *output << "    mov eax, " << fieldLabelOf(field) << " ; static field access\n";
            *output << "    mov ecx, 0\n"; // When setting lvalue, treat ecx as pointer to required type.
        } else {
            *output << "    mov eax, [ebp + " << currentMethodOffsets[nullptr] << "] ; Load this pointer\n";
            InstanceLayoutInfo& layoutInfo = instanceLayoutInfos[currentClass];
            std::string fqName = currentClass->getFullName() + "." + node->getSimpleName();
            *output << "    add eax, " << layoutInfo.fieldOffsets[fqName] << "\n";
            // std::cerr << "Accessing field " << node->getSimpleName() << " at offset " << layoutInfo.fieldOffsets[fqName] << " from ebp\n";
        }
        
    }
    *output << "    mov ecx, 0\n"; // When setting lvalue, treat ecx as pointer to required type.
    return false;
}

bool GenVisitor::visit(QualifiedNameNode* node) {
    switch (node->qualifier->variant) {
        case NameNode::Type: {
            JavaType* type = static_cast<TypeDeclarationNode*>(node->qualifier->resolvedReference)->resolvedType;
            // Static field access
            FieldInfo& field = type->fields[node->name->getSimpleName()];
            ClassLayoutInfo& layoutInfo = layoutInfos[type];
            if (field.declaringType != currentClass) {
                *output << "extern " << classLabelOf(*type) << "\n";
            }
            *output << "    mov eax, " << classLabelOf(*type) << " ; static field class ref\n";
            *output << "    mov eax, [eax + " << layoutInfo.fieldOffsets[node->name->getSimpleName()] << "] ; static field access\n";
            *output << "    mov ecx, 0\n"; // When setting lvalue, treat ecx as pointer to required type.

            break;
        }
        case NameNode::Expression: {
            // Instance field access, need to check for null
            generateExpressionValue(node->qualifier);
            *output << "    cmp eax, 0\n";
            *output << "    je __exception\n"; // Null pointer check
            JavaType* receiverType = node->qualifier->resolvedType;
            std::string fqName = receiverType->getFullName() + "." + node->name->getSimpleName();
            *output << "    add eax, " << instanceLayoutInfos[receiverType].fieldOffsets[fqName] << " ; instance field\n";
            break;
        }
        default: {
            throw std::runtime_error("How did we get here?");
            break;
        }
    }
    *output << "    mov ecx, 0\n"; // When setting lvalue, treat ecx as pointer to required type.
    return false;
}

// Invariant: assignable nodes always return address in eax, non-assignable nodes return a value in eax
bool GenVisitor::visit(AssignmentExpressionNode* node) {
    node->lhs->accept(*this); // Value to be assigned is now in eax, check type if it exists is in ecx
    *output << "    push eax\n";
    *output << "    push ecx\n";
    generateExpressionValue(node->rhs);
    *output << "    pop ecx\n";
    *output << "    pop ebx\n";
    // Now, eax has value to assign, ebx has assignment target, ecx has assignment target's required type class ptr
    std::string nullLabel = generateLabel();
    *output << "    cmp ecx, 0\n"; 
    *output << "    je " << nullLabel << "\n";
    *output << "    call __check_assignable\n"; // If nonzero, this is an array assignment and we need to do an assignability check.
    *output << nullLabel << ":\n";
    switch (node->lhs->resolvedType->elementSize) {
        case 1:
            *output << "    mov byte [ebx], al ; zero-extend byte value for assignment\n";
            break;
        case 2:
            *output << "    mov word [ebx], ax ; zero-extend char/short value for assignment\n";
            break;
        case 4:
            *output << "    mov [ebx], eax ; int or reference value for assignment\n";
            break;
        default:
            throw std::runtime_error("Invalid element size for type in assignment");
    }
    return false;
}

bool GenVisitor::visit(InfixExpressionNode* node) {
    // Oh boy
    switch (node->op) {
        case InfixExpressionNode::Add: {
            if (node->resolvedType == JavaType::String) {
                MethodInfo& concat = JavaType::String->methods[{"concat", {JavaType::String}}];
                ClassLayoutInfo leftLayout = layoutInfos[node->lhs->resolvedType];
                ClassLayoutInfo rightLayout = layoutInfos[node->rhs->resolvedType];
                generateExpressionValue(node->lhs);
                *output << "    push eax\n";
                if (node->lhs->resolvedType->kind == JavaType::Primitive) {
                    MethodInfo& valueOf = JavaType::String->methods[{"valueOf", {node->lhs->resolvedType}}];
                    *output << "extern " << methodLabelOf(valueOf) << "\n";
                    *output << "    call " << methodLabelOf(valueOf) << " ; Convert left operand to string\n";
                } else {
                    MethodInfo& valueOf = JavaType::String->methods[{"valueOf", {JavaType::Object}}];
                    *output << "extern " << methodLabelOf(valueOf) << "\n";
                    *output << "    call " << methodLabelOf(valueOf) << " ; Convert left operand to string\n";
                }
                *output << "    push eax\n"; // Push left string ptr
                generateExpressionValue(node->rhs);
                *output << "    push eax\n";
                if (node->rhs->resolvedType->kind == JavaType::Primitive) {
                    MethodInfo& valueOf = JavaType::String->methods[{"valueOf", {node->rhs->resolvedType}}];
                    *output << "extern " << methodLabelOf(valueOf) << "\n";
                    *output << "    call " << methodLabelOf(valueOf) << " ; Convert right operand to string\n";
                } else {
                    MethodInfo& valueOf = JavaType::String->methods[{"valueOf", {JavaType::Object}}];
                    *output << "extern " << methodLabelOf(valueOf) << "\n";
                    *output << "    call " << methodLabelOf(valueOf) << " ; Convert right operand to string\n";
                }
                *output << "    push eax\n"; // Push right string ptr
                // Now stack should look like ... [left string ptr] [right string ptr]
                *output << "extern " << methodLabelOf(concat) << "\n";
                *output << "    call " << methodLabelOf(concat) << "\n";
                // eax now has concatenated string ptr
            } else if (node->lhs->resolvedType->isNumeric()) {
                generateExpressionValue(node->lhs);
                *output << "    push eax\n";
                generateExpressionValue(node->rhs);
                *output << "    pop ebx\n";
                *output << "    add eax, ebx\n";
            } else {
                throw std::runtime_error("Generation: Invalid operand types for + operator");
            }
            break;
        }
        case InfixExpressionNode::Subtract:
        case InfixExpressionNode::Multiply: {
            generateExpressionValue(node->lhs);
            *output << "    push eax\n";
            generateExpressionValue(node->rhs);
            // Right operand value is now in eax
            *output << "    mov ebx, eax\n";
            *output << "    pop eax\n";
            if (node->op == InfixExpressionNode::Subtract) {
                *output << "    sub eax, ebx\n";
            } else {
                *output << "    imul ebx\n";
            }
            break;
        }
        case InfixExpressionNode::Divide:
        case InfixExpressionNode::Remainder: {
            generateExpressionValue(node->lhs);
            *output << "    push eax\n";
            generateExpressionValue(node->rhs); // Right operand value in eax
            *output << "    cmp eax, 0\n";
            *output << "    je __exception ; DivideByZero\n"; // Division by zero check
            *output << "    mov ebx, eax\n";
            *output << "    pop eax\n";
            *output << "    cdq\n"; // Sign extend eax into edx:eax for division
            *output << "    idiv ebx\n";
            if (node->op == InfixExpressionNode::Remainder) {
                *output << "    mov eax, edx\n";
            }
            break;
        }
        case InfixExpressionNode::LessThan:
        case InfixExpressionNode::GreaterThan:
        case InfixExpressionNode::LessThanOrEqual:
        case InfixExpressionNode::GreaterThanOrEqual: {
            generateExpressionValue(node->lhs); // Left operand value in eax
            *output << "    push eax\n";
            generateExpressionValue(node->rhs); // Right operand value in eax
            *output << "    mov ebx, eax\n";
            *output << "    pop edx\n";
            *output << "    xor eax, eax\n";
            *output << "    cmp edx, ebx\n";
            switch (node->op) {
                case InfixExpressionNode::LessThan:
                    *output << "    setl al\n";
                    break;
                case InfixExpressionNode::GreaterThan:
                    *output << "    setg al\n";
                    break;
                case InfixExpressionNode::LessThanOrEqual:
                    *output << "    setle al\n";
                    break;
                case InfixExpressionNode::GreaterThanOrEqual:
                    *output << "    setge al\n";
                    break;
                default:
                    break;
            }
            break;
        }
        case InfixExpressionNode::ConditionalOr:
        case InfixExpressionNode::ConditionalAnd: {
            std::string shortCircuitLabel = generateLabel();
            std::string endLabel = generateLabel();
            generateExpressionValue(node->lhs); // Left operand value in eax
            if (node->op == InfixExpressionNode::ConditionalOr) {
                *output << "    cmp eax, 0\n";
                *output << "    jne " << shortCircuitLabel << "\n"; // Short circuit if left operand is true (nonzero)
            } else if (node->op == InfixExpressionNode::ConditionalAnd) {
                *output << "    cmp eax, 0\n";
                *output << "    je " << shortCircuitLabel << "\n"; // Short circuit if left operand is false (zero)
            }
            *output << "    push eax\n";
            generateExpressionValue(node->rhs); // Right operand value in eax
            *output << "    pop ebx\n";
            if (node->op == InfixExpressionNode::ConditionalOr) {
                *output << "    or eax, ebx\n"; // We treat 1 and 0 as true and false so this is fine
            } else if (node->op == InfixExpressionNode::ConditionalAnd) {
                *output << "    and eax, ebx\n";
            }
            *output << "    jmp " << endLabel << "\n";
            *output << shortCircuitLabel << ":\n";
            if (node->op == InfixExpressionNode::ConditionalOr) {
                *output << "    mov eax, 1\n"; // Short circuit result is true
            } else if (node->op == InfixExpressionNode::ConditionalAnd) {
                *output << "    mov eax, 0\n"; // Short circuit result is false
            }
            *output << endLabel << ":\n";

            break;
        }
        case InfixExpressionNode::InclusiveOr:
        case InfixExpressionNode::And: {
            generateExpressionValue(node->lhs); // Left operand value in eax
            *output << "    push eax\n";
            generateExpressionValue(node->rhs); // Right operand value in eax
            *output << "    pop ebx\n";
            if (node->op == InfixExpressionNode::ConditionalOr) {
                *output << "    or eax, ebx\n"; // We treat 1 and 0 as true and false so this is fine
            } else if (node->op == InfixExpressionNode::ConditionalAnd) {
                *output << "    and eax, ebx\n";
            }
            break;
        }
        case InfixExpressionNode::Equality:
        case InfixExpressionNode::NotEqual: {
            generateExpressionValue(node->lhs); // Left operand value in eax
            *output << "    push eax\n";
            generateExpressionValue(node->rhs); // Right operand value in eax
            *output << "    mov ebx, eax\n";
            *output << "    pop edx\n";
            *output << "    xor eax, eax\n";
            *output << "    cmp edx, ebx\n";
            if (node->op == InfixExpressionNode::Equality) {
                *output << "    sete al\n";
            } else {
                *output << "    setne al\n";
            } // Works for reference and primitive comparison I guess?
            break;
        }
        default:
            std::cerr << "In generation, unrecognized infix operator: " << node->op << " of type " << (int)node->op << "\n";
            throw std::runtime_error("Unrecognized infix operator");
    }
    return false;
}

bool GenVisitor::visit(InstanceofExpressionNode* node) {
    generateExpressionValue(node->expression);
    generateCheckInstanceof(*node->resolvedCheckType);
    // Only care about boolean value so don't need to preserve
    return false;
}

bool GenVisitor::visit(PrefixExpressionNode* node) {
    if (node->op == PrefixExpressionNode::Not) {
        generateExpressionValue(node->operand);
        *output << "    xor eax, 1\n";
        return false;
    } else { // Negation
        generateExpressionValue(node->operand);
        *output << "    neg eax\n";
        return false;
    }
}

bool GenVisitor::visit(CastExpressionNode* node) {
    generateExpressionValue(node->expression);
    if (node->resolvedType->kind == JavaType::Primitive) {
        // Just generate code for primitive cast, which is basically just a no-op
        return false;
    }
    std::string passLabel = generateLabel();

    *output << "    push eax\n";
    *output << "    cmp eax, 0\n";
    *output << "    je " << passLabel << "\n"; // Null can be cast to any reference type, so skip check if null
    generateCheckInstanceof(*node->resolvedType);
    *output << "    cmp eax, 1\n";
    *output << "    je " << passLabel << "\n";
    *output << "    call __exception\n"; // Throw ClassCastException
    *output << passLabel << ":\n";
    *output << "    pop eax\n"; // Original pointer back in eax, now treated as the casted type
    return false;
}

bool GenVisitor::visit(VariableDeclarationExpressionNode* node) {
    for (auto fragment : node->fragments) {
        fragment->accept(*this);
    }
    return false;
}

bool GenVisitor::visit(ArrayCreationExpressionNode* node) {
    generateExpressionValue(node->size);
    *output << "    cmp eax, 0\n";
    *output << "    jl __exception ; NegativeArraySizeException\n";
    JavaType* arrayType = node->resolvedType->arrayComponentType;
    *output << "    push eax\n";
    *output << "    imul eax, " << arrayType->elementSize << "\n"; // Size of each element
    *output << "    add eax, 8\n"; // Parent array class ptr and array length
    *output << "    call __malloc\n"; // Address of new array in eax
    *output << "extern " << classLabelOf(*node->resolvedType) << "\n";
    *output << "    mov [eax], dword " << classLabelOf(*node->resolvedType) << " ; Set class ptr\n";
    *output << "    pop ebx\n";
    *output << "    mov [eax + 4], ebx\n";
    *output << "    imul ebx, " << arrayType->elementSize << "\n";
    *output << "    add ebx, 8\n";
    *output << "    mov ecx, 8\n";
    std::string initLoopLabel = generateLabel();
    std::string initLoopEndLabel = generateLabel();
    *output << initLoopLabel << ":\n";
    *output << "    cmp ecx, ebx\n";
    *output << "    jge " << initLoopEndLabel << "\n";
    switch (arrayType->elementSize) {
        case 1:
            *output << "    mov byte [eax + ecx], 0\n";
            break;
        case 2:
            *output << "    mov word [eax + ecx], 0\n";
            break;
        case 4:
            *output << "    mov dword [eax + ecx], 0\n";
            break;
        default:
            throw std::runtime_error("Invalid element size for array creation");
    }
    *output << "    add ecx, " << arrayType->elementSize << "\n";
    *output << "    jmp " << initLoopLabel << "\n";
    *output << initLoopEndLabel << ":\n";
    // eax still holds address of new array, which is the result of this expression
    return false;
}

bool GenVisitor::visit(IntegerLiteralNode* node) {
    *output << "    mov eax, " << node->value << "\n";
    return false;
}

bool GenVisitor::visit(BooleanLiteralNode* node) {
    *output << "    mov eax, " << (node->value ? 1 : 0) << "\n";
    return false;
}

bool GenVisitor::visit(CharacterLiteralNode* node) {
    *output << "    mov eax, " << (int)node->value << "\n";
    return false;
}

bool GenVisitor::visit(StringLiteralNode* node) {
    *output << "    mov eax, " << registerStringConstant(node->value) << "\n";
    return false;
}

bool GenVisitor::visit(NullLiteralNode* node) {
    *output << "    mov eax, 0\n";
    return false;
}

bool GenVisitor::visit(ThisExpressionNode* node) {
    *output << "    mov eax, [ebp + " << currentMethodOffsets[nullptr] << "] ; Load this pointer\n";
    return false;
}

bool GenVisitor::visit(ParenthesizedExpressionNode* node) {
    return true; // No fancy tricks
}

bool GenVisitor::visit(FieldAccessExpressionNode* node) {
    generateExpressionValue(node->expression); // Object reference in eax
    *output << "    cmp eax, 0\n";
    *output << "    je __exception ; NullPointerException for field access on null reference\n";
    JavaType* receiverType = node->expression->resolvedType;
    InstanceLayoutInfo& layoutInfo = instanceLayoutInfos[receiverType];
    std::string fqName = receiverType->getFullName() + "." + node->fieldName->getSimpleName();
    *output << "    add eax, " << layoutInfo.fieldOffsets[fqName] << "\n"; // Address within object of field
    *output << "    mov ecx, 0\n"; // When setting lvalue, treat ecx as pointer to required type.
    return false;
}

bool GenVisitor::visit(MethodInvocationExpressionNode* node) {
    if (node->resolvedDeclaration->isStatic()) {
        // Static method, just call it directly
        for (auto arg : node->arguments) {
            generateExpressionValue(arg);
            *output << "    push eax\n";
        }
        // In semantic stage, we confirm static methods have namenode with type associated
        NameNode* qualifier = static_cast<NameNode*>(node->expression);
        JavaType* type = ((TypeDeclarationNode*)qualifier->resolvedReference)->resolvedType;
        Signature methodSig = typeManager.getSignature(node);
        MethodInfo& method = type->methods[methodSig];
        if (method.declaringType != currentClass) {
            *output << "extern " << methodLabelOf(type->methods[methodSig]) << "\n";
        }
        *output << "    call " << methodLabelOf(type->methods[methodSig]) << "\n";
        return false;
    }
    JavaType* receiverType;
    if (node->expression) {
        generateExpressionValue(node->expression); // Object reference in eax
        receiverType = node->expression->resolvedType;
    } else {
        *output << "    mov eax, [ebp + " << currentMethodOffsets[nullptr] << "] ; Implicit this\n";
        receiverType = currentClass;
    }
    *output << "    cmp eax, 0\n";
    *output << "    je __exception ; NullPointerException for method call on null reference\n";
    *output << "    push eax ; Push 'this' reference for method call\n";

    for (auto arg : node->arguments) {
        generateExpressionValue(arg);
        *output << "    push eax\n";
    }

    *output << "    mov eax, [esp + " << node->arguments.size() * 4 << "] ; Retrieve this object\n";
    *output << "    mov eax, [eax] ; Get class infoblock ptr\n";
    Signature methodSig = typeManager.getSignature(node);
    if (receiverType->isInterface) {
        *output << "    mov eax, [eax] ; Get IMT ptr\n";
        *output << "    mov eax, [eax + " << getInterfaceMethodSignatureID(methodSig) * 4 << "]\n";
        *output << "    call eax\n";
    } else {
        // Vtable dispatch
        ClassLayoutInfo& layoutInfo = layoutInfos[receiverType];
        int methodOffset = layoutInfo.methodOffsets[methodSig];
        *output << "    mov eax, [eax + " << methodOffset << "]\n";
        *output << "    call eax\n";
    }
    return false;
}

bool GenVisitor::visit(InstanceCreationExpressionNode* node) {
    Signature constructorSig = typeManager.getSignature(node);
    InstanceLayoutInfo& layoutInfo = instanceLayoutInfos[node->resolvedType];
    // Size of instance is given by nextOffset field implicitly
    *output << "    mov eax, " << layoutInfo.nextOffset << "\n";
    *output << "    call __malloc\n"; // Address of new instance in eax
    *output << "    push eax\n"; // Push new instance reference for constructor call
    *output << "    push eax\n"; // Duplicate for constructor call, will be used to set class ptr
    if (node->resolvedType != currentClass) {
        *output << "extern " << classLabelOf(*node->resolvedType) << "\n";
    }
    *output << "    mov [eax], dword " << classLabelOf(*node->resolvedType) << " ; Set class ptr\n";
    for (auto arg : node->arguments) {
        generateExpressionValue(arg);
        *output << "    push eax\n";
    }
    if (node->resolvedType != currentClass) {
        *output << "extern " << methodLabelOf(node->resolvedType->constructors[constructorSig]) << "\n";
    }\
    *output << "    call " << methodLabelOf(node->resolvedType->constructors[constructorSig]) << "\n";
    *output << "    pop eax\n"; // Pop duplicate instance reference
    return false;
}

// Remember: array layout is
// Class ptr - 0
// Length - 4
// Data - 8+
bool GenVisitor::visit(ArrayAccessExpressionNode* node) {
    generateExpressionValue(node->array);
    *output << "    cmp eax, 0\n";
    *output << "    je __exception ; NullPointerException for array access on null reference\n";
    *output << "    push eax\n";
    generateExpressionValue(node->index); // Index value in eax
    std::string labelPass = generateLabel();
    std::string labelFail = generateLabel();
    *output << "    pop ebx\n"; // Array reference in ebx
    *output << "    mov ecx, [ebx + 4] ; Get array length\n";
    *output << "    cmp eax, ecx\n";
    *output << "    jge " << labelFail << "\n";
    *output << "    cmp eax, 0\n";
    *output << "    jl " << labelFail << "\n";
    *output << "    jmp " << labelPass << "\n";
    *output << labelFail << ":\n";
    *output << "    call __exception\n"; // Throw ArrayIndexOutOfBoundsException
    *output << labelPass << ":\n";
    *output << "    imul eax, " << node->resolvedType->elementSize << "\n"; // Size of each element
    *output << "    add eax, 8\n"; // 
    *output << "    add eax, ebx\n"; // Address of array element in eax
    *output << "    mov ecx, [ebx] ; Array class infoblock\n";
    *output << "    mov ecx, [ecx + 20] ; Array base class infoblock\n";

    return false;
}

} // namespace generation
