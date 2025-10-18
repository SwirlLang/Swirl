#pragma once
#include <mutex>
#include <optional>
#include <filesystem>

#include "ast/Nodes.h"


struct Type;
struct Node;
class IdentInfo;
class SourceManager;
class ErrorPipeline;


struct ErrorContext {
    IdentInfo* ident  = nullptr;
    Type*      type_1 = nullptr;
    Type*      type_2 = nullptr;

    std::filesystem::path path_1{};
    std::filesystem::path path_2{};

    std::string msg{};    // optional error-message, used to convey various syntax errors
    std::string str_1{};
    std::string str_2{};
    std::optional<SourceLocation> location = std::nullopt;

    SourceManager* src_man = nullptr;
    std::unordered_map<IdentInfo*, Node*>* decl_table = nullptr;
};


enum class ErrCode {
    SYNTAX_ERROR,    // non-specific catch-all for the more edgy syntax errors
    UNEXPECTED_EOF,
    EXPECTED_EXPRESSION,
    COMMA_SEP_REQUIRED,
    UNMATCHED_SQ_BRACKET,  // SQ = square
    UNEXPECTED_KEYWORD,


    // The following error codes are related to types, `type_1` and/or `type_2` shall be set to give context
    // about the types involves
    NO_SUCH_TYPE,             // when the type can't be resolved
    INCOMPATIBLE_TYPES,       // non-specific
    NO_IMPLICIT_CONVERSION,   // non-specific catch-all for the edgy implicit-conversion cases
    INT_AND_FLOAT_CONV,       // no implicit integral-floating conversions
    NO_NARROWING_CONVERSION,  // narrowing conversions not allowed
    NO_SIGNED_UNSIGNED_CONV,  // signed-unsigned conversions shall be explicit
    DISTINCTLY_SIZED_ARR,     // arrays of distinct sizes are incompatible
    CANNOT_ASSIGN_TO_CONST,   // attempt to re-assign a const
    IMMUTABILITY_VIOLATION,   // when immutability rules are violated
    SLICE_NOT_COMPATIBLE,     // when slice types are not compatible with each other
    NOT_DEREFERENCE_ABLE,     // type cannot be dereferenced
    // ----------*----------- //


    // The following error codes are related to the Module System, `path_1`, `path_2` and `str_1` shall be
    // set appropriately to convey context
    NO_DIR_IMPORT,
    DUPLICATE_IMPORT,
    MODULE_NOT_FOUND,         // path_1 in the context shall be set to the import-path
    SYMBOL_NOT_FOUND_IN_MOD,  // when a symbol doesn't exist in a module but usage is attempted
    SYMBOL_NOT_EXPORTED,      // when it exists but is not exported by its parent module, str_1 shall be set
    PACKAGE_NOT_FOUND,        // when no registered package of name `str_1` can be found
    // ----------*----------- //


    // The following error codes are related to symbols
    NO_SYMBOL_IN_NAMESPACE,   // when no symbol with the name exists in the namespace
    NOT_A_NAMESPACE,          // when a symbol doesn't name a namespace but is attempted to be used as one
    UNDEFINED_IDENTIFIER,     // self-explanatory
    SYMBOL_ALREADY_EXISTS,    // when symbol already exists
    // ----------*----------- //


    TOO_FEW_ARGS,
    NON_INTEGRAL_INDICES,
    INDEX_OUT_OF_BOUNDS,
    NON_INT_ARRAY_SIZE,
    INITIALIZER_REQUIRED,      // when initialization is required but not given
    RET_TYPE_REQUIRED,         // when explicitly specifying a return-type is required (e.g. recursive calls)
    QUALIFIER_UNDEFINED,
    NO_SUCH_MEMBER,
    MAIN_REDEFINED,            // when the main function is redefined
    CONFIG_VAR_UNINITIALIZED,  // a config-variable is left uninitialized
    CONFIG_INIT_NOT_LITERAL,   // a config variable is initialized with a non-literal

    NOT_ALLOWED_CT_CTX,        // when a construct isn't allowed in compile-time evaluated context
    NOT_A_CT_VAR,              // referenced ID is not of a comptime var
    OP_NOT_ALLOWED_HERE,       // when an operator isn't allowed in a context
};


class ErrorManager {
public:
    /// Used to report an error, lock-free
    void newError(ErrCode code, const ErrorContext& ctx) const;

    /// Used to report an error, acquires the mutex
    void newErrorLocked(ErrCode code, const ErrorContext& ctx);

    /// Write all the errors and clear the buffer
    void flush() const;

    /// Returns true if any error exists in the buffer
    [[nodiscard]] bool errorOccurred() const;


private:
    std::mutex     m_Mutex;
    ErrorPipeline* m_OutputPipeline = nullptr;

    friend class CompilerInst;

    [[nodiscard]]
    static std::string generateMessage(ErrCode code, const ErrorContext& ctx);
};
