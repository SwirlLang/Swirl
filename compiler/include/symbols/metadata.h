#pragma once

class IdentInfo;
class Namespace;
struct Type;

namespace llvm {
    class Value;
}

struct TableEntry {
    bool is_const    = false;
    bool is_param    = false;
    bool is_exported = false;
    bool is_volatile = false;
    bool is_static   = false;
    bool is_mod_namespace = false;

    Namespace*  scope      = nullptr;  // set when the entry also encodes a namespace
    Type*       swirl_type = nullptr;
    Type*       method_of  = nullptr;  // set when the function is a method, holds the encapsulating type
    Node*       node_loc   = nullptr;

    llvm::Value* llvm_value = nullptr;
};

struct IntrinsicDef {
    Type* return_type = nullptr;
};

struct ExportedSymbolMeta_t {
    IdentInfo* id = nullptr;
    Namespace* scope = nullptr;
};

