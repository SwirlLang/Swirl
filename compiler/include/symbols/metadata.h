#pragma once

class IdentInfo;
class Namespace;
struct Type;

namespace llvm {
    class Value;
    class Type;
}

struct TableEntry {
    bool is_const    = false;
    bool is_param    = false;
    bool is_method   = false;
    bool is_exported = false;
    bool is_volatile = false;
    bool is_mod_namespace = false;

    Namespace*  scope      = nullptr;  // set when the entry also encodes a namespace
    Type*       swirl_type = nullptr;
    Node*       node_loc   = nullptr;

    llvm::Value* llvm_value = nullptr;
    llvm::Type*  llvm_type  = nullptr;
};


struct ExportedSymbolMeta_t {
    IdentInfo* id = nullptr;
    Namespace* scope = nullptr;
};

