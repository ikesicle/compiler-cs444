#ifndef SEMANTICS_HIERARCHYCHECKER_HPP
#define SEMANTICS_HIERARCHYCHECKER_HPP

#include "ast/nodes.hpp"
#include "ast/tagged_scope.hpp"
#include "ast/java_type.hpp"
#include "ast/type_manager.hpp"

#include <cstdlib>
#include <iostream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>


namespace semantics {

using namespace ast;

class HierarchyChecker {
    struct MInfo {
        const MethodDeclarationNode* decl{};
        std::string ret;
        bool abs{};
        bool stat{};
        bool fin{};
        bool pub{};
        bool prot{};
    };

    using MMap = std::unordered_map<std::string, MInfo>;

    TaggedScope* globalScope;
    std::vector<CompilationUnitNode*> compilationUnits;
    std::unordered_map<Signature, MethodInfo> javaLangObjectMethods;
    TypeManager& typeManager;
    // For storing generated JavaType instances for classes/interfaces in the AST
    // Can be moved somewhere else if needed.

    void die(const std::string& msg) {
        throw std::runtime_error("Hierarchy check failed: " + msg);
    }

    using TD   = TypeDeclarationNode;
    using ModK = ModifierNode::Kind;

    std::string nameOf(const TD* td) {
        return (td && td->name) ? td->name->value : "<unknown>";
    }

    std::string packageOf(const CompilationUnitNode* cu) {
        return (cu && cu->packageDeclaration && cu->packageDeclaration->name) ? cu->packageDeclaration->name->toString() : "";
    }

    bool hasMod(const std::vector<ModifierNode*>& ms, ModK k) {
        for (auto* m : ms) if (m && m->kind == k) return true;
        return false;
    }

    bool isFinal(const TD* td) { return td && hasMod(td->modifiers, ModK::Final); }
    bool isAbs(const TD* td)   { return td && hasMod(td->modifiers, ModK::Abstract); }


    JavaType& typeStr(const TypeNode* t) {
        if (t->getNodeType() == NodeType::PrimitiveType) {
            auto* p = static_cast<const PrimitiveTypeNode*>(t);
            switch (p->kind) {
                case PrimitiveTypeNode::Kind::Boolean: return JavaType::Boolean;
                case PrimitiveTypeNode::Kind::Byte:    return JavaType::Byte;
                case PrimitiveTypeNode::Kind::Char:    return JavaType::Char;
                case PrimitiveTypeNode::Kind::Int:     return JavaType::Int;
                case PrimitiveTypeNode::Kind::Short:   return JavaType::Short;
            }
        } else if (t->getNodeType() == NodeType::ArrayType) {
            auto* a = static_cast<const ArrayTypeNode*>(t);
            return *typeManager.getArrayType(&typeStr(a->componentType));
        } else if (t->getNodeType() == NodeType::ClassOrInterfaceType) {
            auto* c = static_cast<const ClassOrInterfaceTypeNode*>(t);
            if (!c || !c->resolvedDeclaration) {
                throw std::runtime_error("Unresolved type in hierarchy checker: " + (c ? c->name->toString() : "<null>"));
            }
            auto ret = c->resolvedDeclaration->resolvedType;
            return *ret;
        }
        std::cerr << "Failed to resolve type from node: " << t->getNodeType() << "\n";
        throw std::runtime_error("Unhandled type node in hierarchy checker");
    }

    void collect(const std::vector<CompilationUnitNode*>& units, std::vector<TD*>& out) {
        for (auto* u : units)
            for (auto* td : u->typeDeclarations)
                if (td) out.push_back(td);
    }

    std::vector<const TD*> supers(const TD* td) {
        std::vector<const TD*> v;
        if (!td->extends.empty()) {
            for (auto* e : td->extends) {
                if (e && e->resolvedDeclaration) {
                    v.push_back(e->resolvedDeclaration);
                }
            }
        }
        for (auto* i : td->implements) {
            if (i && i->resolvedDeclaration) v.push_back(i->resolvedDeclaration);
        }
        if (v.empty() && !td->isInterface && td->resolvedType != JavaType::Object) v.push_back(JavaType::Object->declaration); 
        // Implicitly extend Object if no supers and not an interface
        return v;
    }

    void chkKinds(const TD* td) {
        if (!td) return;

        if (!td->isInterface) {
            if (!td->extends.empty()) {
                TD *e = td->extends[0]->resolvedDeclaration;
                if (e) {
                    if (e->isInterface) die("class extends interface");
                    if (isFinal(e)) die("class extends final class");
                }
            }

            for (auto* i : td->implements) {
                if (i && i->resolvedDeclaration && !i->resolvedDeclaration->isInterface) {
                    die("class implements class");
                }
            }
        } else {
            if (!td->extends.empty() && td->extends[0]->resolvedDeclaration &&
                !td->extends[0]->resolvedDeclaration->isInterface)
                die("interface extends class");

            if (!td->implements.empty())
                die("interface has implements clause");
        }

        std::unordered_set<const TD*> seen;
        for (auto* s : supers(td))
            if (!seen.insert(s).second)
                die("duplicate super type");
    }

    void chkLocal(TD* td) {
        std::unordered_map<Signature, JavaType*> meths;
        std::unordered_set<Signature> ctors;

        for (auto* b : td->bodyDeclarations) {
            if (auto* m = dynamic_cast<MethodDeclarationNode*>(b)) {
                auto k = typeManager.getSignature(m);
                JavaType* r;
                if (m->returnType == nullptr) {
                    r = &JavaType::Void; // void return type
                } else {
                    r = &typeStr(m->returnType);
                }
                if (meths.count(k) && meths[k] != r)
                    die("method return mismatch");
                if (meths.count(k))
                    die("duplicate method");
                meths[k] = r;
            }
            if (auto* c = dynamic_cast<ConstructorDeclarationNode*>(b)) {
                if (!ctors.insert(typeManager.getSignature(c)).second)
                    die("duplicate constructor");
            }
        }
    }

    enum Mark { W, G, B };

    void dfs(const JavaType* t, std::unordered_map<const JavaType*, Mark>& m) {
        if (!t) return;
        if (m[t] == Mark::G) die("cycle in hierarchy");
        if (m[t] == Mark::B) return;

        m[t] = Mark::G;
        for (auto* s : t->superTypes) dfs(s, m);
        m[t] = Mark::B;
    }

    JavaType& mergeIface(JavaType& type, JavaType& supertype) {
        buildIface(supertype);
        for (auto& [k, mi] : supertype.methods) {
            if (type.methods.count(k) && type.methods.at(k).returnType != mi.returnType)
                die("inherited method return mismatch");
            type.methods.emplace(k, mi);
        }
        return type;
    }

    JavaType& buildIface(JavaType& type) {
        if (type.resolved) return type;
        TypeDeclarationNode* td = type.declaration;
        type.name = nameOf(td);
        type.isInterface = true;
        type.abs = isAbs(td);
        type.fin = isFinal(td);
        // std::cerr << "Building interface " << type.name << " @ " << td << "\n";
        // NOTE: Interfaces can only extend interfaces, so we don't need to check the kind here
        for (auto* i : td->extends) {
            if (i && i->resolvedDeclaration) {
                mergeIface(type, *i->resolvedDeclaration->resolvedType);
            }
        }

        if (td->extends.empty()) {
            auto& javaLangObjectMethods = buildClass(*JavaType::Object).methods;
            for (auto& [k, m] : javaLangObjectMethods) {
                // Public, non-final methods from Object are inherited by interfaces that extend nothing
                if (!m.fin && m.pub) {
                    MethodInfo m2 = m;
                    m2.abs = true; // Methods from Object are abstract from the perspective of interfaces
                    m2.declaringType = &type;
                    m2.isInterfacedMethod = true;
                    type.methods.emplace(k, m2);
                    std::cerr << k.toString() << " from JLO: " << nameOf(td) << " has return type " << (m2.returnType ? m2.returnType->name : "void") << " declared in " << m2.declaringType->name << "\n";
                }
            }
        }

        for (auto* b : td->bodyDeclarations) {
            if (b->getNodeType() != NodeType::MethodDeclaration) continue;
            auto* m = static_cast<MethodDeclarationNode*>(b);

            MethodInfo mi;
            mi.decl.methodDecl = m;
            mi.returnType = m->returnType ? &typeStr(m->returnType) : &JavaType::Void;
            mi.abs  = td->isInterface || m->isAbstract();
            mi.stat = m->isStatic();
            mi.fin  = m->isFinal();
            mi.pub  = m->isPublic();
            mi.prot = m->isProtected();
            mi.native = m->isNative();
            mi.signature = typeManager.getSignature(m);
            mi.isInterfacedMethod = true;
            mi.declaringType = &type;
            if (javaLangObjectMethods.count(mi.signature) && javaLangObjectMethods[mi.signature].pub && javaLangObjectMethods[mi.signature].fin) {
                die("interface method overrides final method (only getClass) from Object");
            }
            //std::cerr << k.toString() << " in interface " << nameOf(td) << " has return type " << (mi.returnType ? mi.returnType->name : "void") << "\n";
            if (type.methods.count(mi.signature)) {
                chkIOverride(mi, type.methods[mi.signature]);
            }
            type.methods[mi.signature] = mi;
        }
        type.resolved = true;
        return type;
    }

    void chkOverride(const MethodInfo& sub, const MethodInfo& sup) {
        try {
            if (!sub.stat && sup.stat) die("static override (subclass inst, superclass static)");
            if (sub.stat && !sup.stat) die("static override (subclass static, superclass inst)");
            if (sub.returnType != sup.returnType) die("return mismatch");
            if (sub.prot && sup.pub) die("weaker access");
            if (sup.fin) die("override final");
        } catch (const std::runtime_error& e) {
            std::cerr << "When trying to override method " << sup.signature.toString() << " declared in " << sup.declaringType->name << " \nwith method " << sub.signature.toString() << " declared in " << sub.declaringType->name << ": " << e.what() << "\n";
            throw;
        }
    }

    void chkIOverride(const MethodInfo& sub, const MethodInfo& sup) {
        if (sub.returnType != sup.returnType) die("return mismatch interface");
        if (sup.fin) die("override final interface");
    }

    JavaType& buildClass(JavaType& type) {
        if (type.resolved) return type;
        TypeDeclarationNode* td = type.declaration;

        type.name = nameOf(td);
        type.isInterface = false;
        type.abs = isAbs(td);
        type.fin = isFinal(td);
        
        type.directSuperClass = nullptr;

        if (!td->extends.empty()) {
            type.directSuperClass = &buildClass(*td->extends[0]->resolvedDeclaration->resolvedType);
        } else if (td != JavaType::Object->declaration) {
            // Classes which extend nothing implicitly extend Object
            // std::cerr << "Class " << nameOf(td) << " extends nothing, so implicitly extends java.lang.Object\n";
            type.directSuperClass = &buildClass(*JavaType::Object);
            if (!type.directSuperClass) {
                throw std::runtime_error("java.lang.Object type not resolved");
            }
        } else {
            // std::cerr << "Skipping java.lang.Object since it has no superclass\n";
        }
        if (type.directSuperClass != nullptr) {
            for (auto [k, m] : type.directSuperClass->methods) {
                type.methods[k] = m;
            }
            for (auto& [k, f] : type.directSuperClass->fields) {
                type.fields[k] = f;
            }
        }

        for (auto* b : td->bodyDeclarations) {
            if (!b) continue;
            switch (b->getNodeType()) {
                case NodeType::FieldDeclaration: {
                    auto* f = static_cast<FieldDeclarationNode*>(b);
                    for (auto* frag : f->fragments) {
                        if (!frag || !frag->name) continue;
                        std::string fname = frag->name->getSimpleName();
                        FieldInfo fi;
                        fi.decl = frag;
                        fi.declaringType = &type;
                        fi.type = &typeStr(f->type);
                        fi.name = fname;
                        fi.stat = f->isStatic();
                        fi.fin  = f->isFinal();
                        fi.pub  = f->isPublic();
                        fi.prot = f->isProtected();
                        type.fields[fname] = fi;
                    }
                    break;
                }
                case NodeType::MethodDeclaration: {
                    auto* m = static_cast<MethodDeclarationNode*>(b);

                    MethodInfo mi2;
                    mi2.decl.methodDecl = m;
                    mi2.returnType = m->returnType ? &typeStr(m->returnType) : &JavaType::Void;
                    mi2.signature = typeManager.getSignature(m);
                    mi2.declaringType = &type;
                    mi2.abs  = m->isAbstract();
                    mi2.stat = m->isStatic();
                    mi2.fin  = m->isFinal();
                    mi2.pub  = m->isPublic();
                    mi2.prot = m->isProtected();
                    mi2.native = m->isNative();
                    mi2.isConstructor = false;
                    // std::cerr << "Method " << mi2.signature.toString() << " in class " << nameOf(td) << " has return type " << mi2.returnType->name << "\n";
                    if (type.methods.count(mi2.signature)) {
                        chkOverride(mi2, type.methods[mi2.signature]);
                        mi2.isInterfacedMethod = type.methods[mi2.signature].isInterfacedMethod; // If the method being overridden is from an interface, then this method is also an interfaced method
                    }
                    if (&type == JavaType::Object && mi2.pub && !mi2.fin) {
                        mi2.isInterfacedMethod = true; 
                    }
                    type.methods[mi2.signature] = mi2;
                    break;
                }
                case NodeType::ConstructorDeclaration: {
                    auto* c = static_cast<ConstructorDeclarationNode*>(b);
                    Signature k = typeManager.getSignature(c);
                    if (k.name != type.name) {
                        die("constructor name does not match class name");
                    }
                    for (auto& [sig, constructor] : type.constructors) {
                        if (sig == k) {
                            die("duplicate constructor");
                        }
                    }
                    
                    MethodInfo ci;
                    ci.decl.ctorDecl = c;
                    ci.signature = k;
                    ci.declaringType = &type;
                    ci.abs = false; // Constructors are never abstract
                    ci.stat = c->isStatic();
                    ci.fin  = c->isFinal();
                    ci.pub  = c->isPublic();
                    ci.prot = c->isProtected();
                    ci.native = c->isNative(); // Don't think this is possible in Java but we'll check it anyway
                    ci.isConstructor = true;
                    type.constructors[ci.signature] = ci;
                    break;
                }
                default:
                    break;
            }
            
        }

        for (auto* i : td->implements) {
            if (i && i->resolvedDeclaration) {
                for (auto& [k, m] : buildIface(*i->resolvedDeclaration->resolvedType).methods) {
                    if (type.methods.count(k)) {
                        if (type.methods[k].returnType != m.returnType) {
                            die("two method return mismatch - interface method has different return type than another interface/class");
                        }
                        type.methods[k].isInterfacedMethod = true;
                        if (type.methods[k].stat) die("static override (class static, interface instance)");
                        if (type.methods[k].prot && (!type.methods[k].abs || type.methods[k].declaringType == &type)) {
                            // Either it already has an implementation (not abs) and that implementation is protected, or it's declared in this class to be protected, in which case it's illegal anyways
                            std::cerr << "Method " << k.toString() << " in class " << nameOf(td) << " has weaker access than in interface " << nameOf(i->resolvedDeclaration) << "\n";
                            die("weaker access - interface method is public but implementation is protected");
                        }
                        type.methods[k].pub = true; // Interface methods are always public, so the implementation must be public as well
                    } else {
                        type.methods[k] = m;
                    }
                }
            }
        }

        for (auto& [_, m] : type.methods)
            if (m.abs && !type.abs)
                die("non-abstract class has abstract methods");

        type.resolved = true;
        return type;
    }

    void chkIfaceMethods(const TD* td) {
        if (!td->isInterface) return;
        for (auto* b : td->bodyDeclarations) {
            auto* m = dynamic_cast<MethodDeclarationNode*>(b);
            if (!m) continue;
            if (m->isStatic() || m->isFinal())
                die("bad interface method modifier");
            if (m->body) die("interface method has body");
        }
    }

public:
    HierarchyChecker(TaggedScope* globalScope, const std::vector<CompilationUnitNode*>& compilationUnits, TypeManager& typeManager)
        : globalScope(globalScope), compilationUnits(compilationUnits), typeManager(typeManager) {
            typeManager.clear();
            auto pkg = globalScope->getChildByTag("java.lang");
            if (pkg) {
                auto obj = pkg->lookup("Object", NodeType::TypeDeclaration);
                if (!obj.empty()) {
                    auto objTd = static_cast<TD*>(obj[0]);
                    if (objTd) {
                        JavaType::Object = typeManager.getNewType();
                        JavaType::Object->declaration = objTd;
                        JavaType::Object->name = nameOf(objTd);
                        objTd->resolvedType = JavaType::Object;
                        // std::cerr << "Created JavaType for " << nameOf(objTd) << " at " << objTd << ": " << objTd->resolvedType << "\n";
                    } else {
                        die("java.lang.Object can't be cast to a TypeDeclarationNode");
                    }
                } else {
                    die("java.lang.Object not found");
                }
                obj = pkg->lookup("String", NodeType::TypeDeclaration);
                if (!obj.empty()) {
                    auto objTd = static_cast<TD*>(obj[0]);
                    if (objTd) {
                        JavaType::String = typeManager.getNewType();
                        JavaType::String->declaration = objTd;
                        JavaType::String->name = nameOf(objTd);
                        objTd->resolvedType = JavaType::String;
                        // std::cerr << "Created JavaType for " << nameOf(objTd) << " at " << objTd << ": " << objTd->resolvedType << "\n";
                    } else {
                        die("java.lang.String can't be cast to a TypeDeclarationNode");
                    }
                } else {
                    die("java.lang.String not found");
                }
                obj = pkg->lookup("Cloneable", NodeType::TypeDeclaration);
                if (!obj.empty()) {
                    auto objTd = static_cast<TD*>(obj[0]);
                    if (objTd) {
                        JavaType::Cloneable = typeManager.getNewType();
                        JavaType::Cloneable->declaration = objTd;
                        JavaType::Cloneable->name = nameOf(objTd);
                        objTd->resolvedType = JavaType::Cloneable;
                        // std::cerr << "Created JavaType for " << nameOf(objTd) << " at " << objTd << ": " << objTd->resolvedType << "\n";
                    } else {
                        die("java.lang.Cloneable can't be cast to a TypeDeclarationNode");
                    }
                } else {
                    die("java.lang.Cloneable not found");
                }
            } else {
                die("java.lang package not found");
            }
            pkg = globalScope->getChildByTag("java.io");
            if (pkg) {
                auto obj = pkg->lookup("Serializable", NodeType::TypeDeclaration);
                if (!obj.empty()) {
                    auto objTd = static_cast<TD*>(obj[0]);
                    if (objTd) {
                        JavaType::Serializable = typeManager.getNewType();
                        JavaType::Serializable->declaration = objTd;
                        JavaType::Serializable->name = nameOf(objTd);
                        objTd->resolvedType = JavaType::Serializable;
                        // std::cerr << "Created JavaType for " << nameOf(objTd) << " at " << objTd << ": " << objTd->resolvedType << "\n";
                    } else {
                        die("java.io.Serializable can't be cast to a TypeDeclarationNode");
                    }
                } else {
                    die("java.io.Serializable not found");
                }
            } else {
                die("java.io package not found");
            }
            for (auto bd : JavaType::Object->declaration->bodyDeclarations) {
                if (!bd || bd->getNodeType() != NodeType::MethodDeclaration) continue;
                auto* m = static_cast<MethodDeclarationNode*>(bd);
                if (!m->isPublic()) continue; // Protected methods in Object are not inherited by interfaces  
                MethodInfo mi;
                mi.decl.methodDecl = m;
                mi.returnType = m->returnType ? &typeStr(m->returnType) : &JavaType::Void;
                mi.abs  = true; // All methods in Object are abstract from the perspective of interfaces
                mi.stat = false;
                mi.fin  = m->isFinal(); // Final methods from Object remain final
                mi.pub  = m->isPublic();
                mi.prot = false; // Protected methods in Object are not inherited by interfaces
                mi.native = m->isNative();
                mi.isConstructor = false;
                auto k = typeManager.getSignature(m);
                javaLangObjectMethods[k] = mi;
            }
        }
    
    void check() {
        std::vector<TD*> decls;
        collect(compilationUnits, decls);
        for (auto* td : decls) {
            if (td->resolvedType) continue; // Already created a JavaType for this type declaration
            td->resolvedType = typeManager.getNewType();
            // std::cerr << "Created JavaType for " << nameOf(td) << " at " << td << ": " << td->resolvedType << "\n";
            td->resolvedType->declaration = td;
            td->resolvedType->name = nameOf(td);
        }

        for (auto* cu : compilationUnits) {
            std::string pkgName = packageOf(cu);
            for (auto* td : cu->typeDeclarations) {
                if (!td) continue;
                if (!td->resolvedType) {
                    die("Type declaration has no resolved JavaType");
                }
                td->resolvedType->package = pkgName;
            }
        }

        for (auto* td : decls) {
            chkKinds(td);
            chkLocal(td);
            chkIfaceMethods(td);
            for (auto* s : supers(td)) {
                if (!s->resolvedType) {
                    die("super type has no JavaType");
                }
                // std::cerr << "Adding super type " << s->resolvedType->name << " to " << td->resolvedType->name << "\n";
                td->resolvedType->superTypes.push_back(s->resolvedType);
            }
        }
        
        std::unordered_map<const JavaType*, Mark> mark;
        for (auto* td : decls) dfs(td->resolvedType, mark);

        for (auto* td : decls) {
            td->isInterface ? buildIface(*td->resolvedType) : buildClass(*td->resolvedType);
        }

        for (auto* td : decls) {
            JavaType* t = td->resolvedType;
            for (auto* supertype : t->superTypes) {
                if (supertype->isInterface) continue; // No need to check constructors for interfaces since they don't have any
                Signature zerocons = { supertype->name, {} };
                auto it = supertype->constructors.find(zerocons);
                if (it == supertype->constructors.end()) {
                    die("constructor mismatch - superclass has no zero-arg constructor");
                }
                // Subclass will have access to constructor anyways so we don't need to check
            }
        }
    }
};

} // namespace semantics

#endif // SEMANTICS_HIERARCHYCHECKER_HPP