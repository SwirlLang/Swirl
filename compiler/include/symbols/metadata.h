#pragma once

class IdentInfo;

struct TableEntry {
    bool is_type     = false;
    bool is_const    = false;
    bool is_param    = false;
    bool is_exported = false;
    bool is_volatile = false;

    llvm::Value* ptr        = nullptr;
    llvm::Type*  llvm_type  = nullptr;
    Type*        swirl_type = nullptr;
};


struct ExportedSymbolMeta_t {
    IdentInfo* id = nullptr;
    fs::path    path;  // to be set if the exported symbol is a module (say `export import mod`) to the
                       // absolute path of the exported module
};

