#include "weeder/weeder.hpp"
#include <cstdlib>
#include <iostream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

using common::ParseTree;

namespace weeder {

namespace {

[[noreturn]] void fail(const std::string& msg) {
    std::cerr << "Weeder error: " << msg << "\n";
    std::exit(42);
}

std::string getBaseName(const std::string& path) {
    size_t slash = path.find_last_of("/\\");
    std::string name = (slash == std::string::npos) ? path : path.substr(slash + 1);
    size_t dot = name.rfind('.');
    if (name.substr(dot) != ".java") {
        std::cerr << "Error: Input file must have .java extension. Has " << name.substr(dot) << "\n";
        fail("Input file must have .java extension. ");
    }
    if (dot != std::string::npos) name = name.substr(0, dot);
    return name;
}

void checkAscii(const ParseTree& n) {
    if (n.isTerminal()) {
        for (unsigned char ch : n.value) {
            if (ch > 127) fail("Non-ASCII character in input (must be 0..127).");
        }
    }
    for (const auto& c : n.children) checkAscii(*c);
}

struct Mods {
    std::unordered_set<std::string> set;
    std::unordered_map<std::string, int> cnt;
};

Mods parseModifiers(const ParseTree& mo) {

    Mods out;
    if (mo.kind != "ModifiersOpt") return out;
    if (mo.children.empty()) return out;

    const ParseTree* cur = mo[0];

    while (cur && cur->kind == "Modifiers") {
        if (cur->children.size() == 1) {
            const ParseTree* mod = cur->children[0].get();
            if (mod && mod->children.size() == 1) {
                std::string mk = mod->children[0]->kind;
                out.set.insert(mk);
                out.cnt[mk]++;
            }
            break;
        } else if (cur->children.size() == 2) {
            const ParseTree* left = cur->children[0].get();
            const ParseTree* mod  = cur->children[1].get();
            if (mod && mod->children.size() == 1) {
                std::string mk = mod->children[0]->kind;
                out.set.insert(mk);
                out.cnt[mk]++;
            }
            cur = left;
        } else {
            break;
        }
    }
    return out;
}

void checkOneAccess(const Mods& m) {
    int a = 0;
    if (m.set.count("PUBLIC")) a++;
    if (m.set.count("PROTECTED")) a++;
    if (m.set.count("PRIVATE")) a++;
    if (a > 1) fail(">1 access modifier specified.");
    if (a == 0) fail("Package private access is not permitted in JOOS.");
}

void checkClassMods(const ParseTree& cd) {
    if (cd.kind != "ClassDecl") return;
    if (cd.children.empty()) return;
    Mods m = parseModifiers(*cd.children[0]);
    checkOneAccess(m);

    if (m.set.count("ABSTRACT") && m.set.count("FINAL"))
        fail("A class cannot be both abstract and final.");
}

void checkMethod(const ParseTree& md) {
    if (md.kind != "MethodDecl") return;
    if (md.children.size() != 2) return;

    const ParseTree& hdr = *md.children[0];
    const ParseTree& bod = *md.children[1];

    if (hdr.children.empty()) return;
    Mods m = parseModifiers(*hdr.children[0]);
    checkOneAccess(m);

    bool isAbs = m.set.count("ABSTRACT") != 0;
    bool isNat = m.set.count("NATIVE")   != 0;
    bool isSta = m.set.count("STATIC")   != 0;
    bool isFin = m.set.count("FINAL")    != 0;
    bool hasBlk = (bod.kind == "MethodBody" && !bod.children.empty() && bod.children[0]->kind == "Block");
    bool hasSem = (bod.kind == "MethodBody" && !bod.children.empty() && bod.children[0]->kind == "SEMI");

    if (hasBlk && (isAbs || isNat))
        fail("Abstract/native cannot have a body).");
    if (hasSem && !(isAbs || isNat))
        fail("Non-abstract/non-native must have a body).");

    if (isAbs && (isSta || isFin))
        fail("An abstract method cannot be static or final.");
    if (isSta && isFin)
        fail("A static method cannot be final.");
    if (isNat && !isSta)
        fail("A native method must be static.");
}

void checkVoidUse(const ParseTree& n,
                  bool inMethodHeader = false,
                  bool inType = false,
                  bool inFormal = false,
                  bool inField = false,
                  bool inLocal = false) {

    bool mh = inMethodHeader || (n.kind == "MethodHeader");
    bool ty = inType         || (n.kind == "Type");
    bool fp = inFormal       || (n.kind == "FormalPara");
    bool fd = inField        || (n.kind == "FieldDecl");
    bool lv = inLocal        || (n.kind == "LocalVarDecl");

    if (n.kind == "VOID") {
        if (!mh || ty || fp || fd || lv) {
            fail("The type void may only be used as the return type of a method.");
        }
    }
    for (const auto& c : n.children) {
        checkVoidUse(*c, mh, ty, fp, fd, lv);
    }
}

void checkFormalInit(const ParseTree& n) {
    if (n.kind == "FormalPara") {
        if (n.findOne("ASSIGN")) {
            fail("A formal parameter of a method must not have an initializer.");
        }
    }
    for (const auto& c : n.children) checkFormalInit(*c);
}

void checkFileName(const ParseTree& n, const std::string& base) {
    if (n.kind == "ClassDecl") {
        if (n.children.size() >= 3 && n.children[2]->kind == "IDENTIFIER") {
            if (n.children[2]->value != base)
                fail("Class name  must match file base name");
        } else {
            std::cerr << "WTF? ClassDecl with no IDENTIFIER child?\n";
        }
    } else if (n.kind == "InterfaceDecl") {
        if (n.children.size() >= 3 && n.children[2]->kind == "IDENTIFIER") {
            if (n.children[2]->value != base)
                fail("Interface name must match file base name");
        } else {
            std::cerr << "WTF? InterfaceDecl with no IDENTIFIER child?\n";
        }
    }
    for (const auto& c : n.children) checkFileName(*c, base);
}

void checkInterface(const ParseTree& id) {
    if (id.kind != "InterfaceDecl") return;
    if (id.findOne("FieldDecl")) fail("An interface cannot contain fields.");
    if (id.findOne("ConstructorDecl")) fail("An interface cannot contain constructors.");
}

void checkInterfaceMethod(const ParseTree& amd) {
    if (amd.kind != "AbstractMethodDecl") return;
    if (amd.children.empty()) return;
    const ParseTree& hdr = *amd.children[0];
    if (hdr.children.empty()) return;
    Mods m = parseModifiers(*hdr.children[0]);
    checkOneAccess(m);

    if (m.set.count("STATIC") || m.set.count("FINAL") || m.set.count("NATIVE"))
        fail("An interface method cannot be static, final, or native.");
}

bool bodyHasCtor(const ParseTree& cb) {
    if (cb.kind != "ClassBody") return false;
    for (const auto& c : cb.children) {
        if (c->kind == "ClassBodyDeclsOpt" && !c->children.empty()) {
            std::vector<const ParseTree*> st;
            st.push_back(c->children[0].get());
            while (!st.empty()) {
                const ParseTree* cur = st.back(); st.pop_back();
                if (cur->kind == "ConstructorDecl") return true;
                for (const auto& cc : cur->children) st.push_back(cc.get());
            }
        }
    }
    return false;
}

void checkCtorExists(const ParseTree& cd) {
    if (cd.kind != "ClassDecl") return;
    const ParseTree* body = nullptr;

    for (const auto& c : cd.children) {
        if (c->kind == "ClassBody") { body = c.get(); break; }
    }
    if (!body) return;
    if (!bodyHasCtor(*body))
        fail("Every class must contain at least one explicit constructor.");
}

void checkNoFinalField(const ParseTree& fd) {
    if (fd.kind != "FieldDecl") return;
    if (fd.children.empty()) return;
    Mods m = parseModifiers(*fd.children[0]);
    checkOneAccess(m);
    if (m.set.count("FINAL"))
        fail("No field can be final.");
}

int countBrace(const ParseTree& n) {
    int count = 0;
    if (n.kind == "LBRACKET") {
        count = 1;
    }
    
    for (size_t i = 0; i < n.children.size(); ++i) {
        count = count + countBrace(*n.children[i]);
    }
    return count;
}

void checkMultiDim(const ParseTree& n) {
    if (n.kind == "ArrayType" || n.kind == "Type") {
        if (countBrace(n) > 1) {
            fail("Multidimensional arrays are forbidden.");
        }
    }
}

void checkValidCast(const ParseTree& ce) {
    if (ce.children.size() != 4) return;
    ParseTree& type = *ce.children[1];
    if (type.kind == "Expression") {
        auto& prims = type.flatten();
        std::string expect = "IDENTIFIER";
        for (const ParseTree* p : prims) {
            if (p->kind != expect) {
                fail("Invalid cast: expected a type name or primitive type");
            } else {
                if (expect == "IDENTIFIER") {
                    expect = "DOT";
                } else if (expect == "DOT") {
                    expect = "IDENTIFIER";
                }
            }
        }
        if (expect == "IDENTIFIER") {
            fail("Invalid cast: expected a type name or primitive type");
        }
    }
}

// Returns true if should search within.
bool checkPosNegIntRange(ParseTree& n) {
    if (n.kind != "UnaryExpr") return true;
    auto& prims = n.flatten();
    if (prims.size() == 1 && prims[0]->kind == "LITERAL_INTEGER") {
        try {
            (void)std::stoi(prims[0]->value);
        } catch (const std::exception& e) {
            fail("Literal integer out of range: " + prims[0]->value);
        }
        return true;
    } else if (prims.size() == 2 && prims[0]->kind == "MINUS" && prims[1]->kind == "LITERAL_INTEGER") {
        try {
            (void)std::stoi("-" +prims[1]->value);
        } catch (const std::exception& e) {
            fail("Invalid integer literal: " + prims[1]->value);
        }
        return false; // don't search within, already checked the literal
    }
    return true;
}

void runStructuralChecks(ParseTree& n) {
    if (n.kind == "ClassDecl") checkClassMods(n);
    if (n.kind == "MethodDecl") checkMethod(n);
    if (n.kind == "FieldDecl") checkNoFinalField(n);
    if (n.kind == "AbstractMethodDecl") checkInterfaceMethod(n);
    if (n.kind == "InterfaceDecl") checkInterface(n);
    if (n.kind == "ClassDecl") checkCtorExists(n);
    checkMultiDim(n);
    if (n.kind == "CastExpr") checkValidCast(n);
    for (const auto& c : n.children) {
        runStructuralChecks(*c);
    }
}

void runIntegerChecks(ParseTree& n) {
    if (checkPosNegIntRange(n)) {
        for (const auto& c : n.children) {
            runIntegerChecks(*c);
        }
    }
}

} // empty namespace

void weed(common::ParseTree& root, const std::string& filePath) {
    checkAscii(root);
    std::string base = getBaseName(filePath);
    checkFileName(root, base);
    runStructuralChecks(root);
    runIntegerChecks(root);
    checkVoidUse(root);
    checkFormalInit(root);
}

} // namespace weeder

