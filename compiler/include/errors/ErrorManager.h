#pragma once
#include <mutex>
#include <optional>
#include <filesystem>

#include "parser/Nodes.h"
#include "types/SwTypes.h"


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


    // The following error codes are related to types, `type_1` and `type_2` shall be set to give context
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
    CONFIG_INIT_NOT_LITERAL    // a config variable is initialized with a non-literal
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
    std::mutex      m_Mutex;
    ErrorPipeline* m_OutputPipeline = nullptr;

    friend class CompilerInst;

    [[nodiscard]]
    static std::string generateMessage(ErrCode code, const ErrorContext& ctx);
};


inline std::string ErrorManager::generateMessage(const ErrCode code, const ErrorContext& ctx) {
    switch (code) {
        case ErrCode::SYNTAX_ERROR:
            return std::format("Syntax error! {}", ctx.msg);
        case ErrCode::UNEXPECTED_EOF:
            return "Unexpected EOF (end of file)!";
        case ErrCode::EXPECTED_EXPRESSION:
            return "An expression was expected here.";
        case ErrCode::COMMA_SEP_REQUIRED:
            return "Comma-separation is required here.";
        case ErrCode::UNMATCHED_SQ_BRACKET:
            return "Square bracket(s) are unmatched (missing a matching ']' or '[').";


        case ErrCode::TOO_FEW_ARGS:
            return "Too few arguments passed to the function call!";
        case ErrCode::INDEX_OUT_OF_BOUNDS:
            return "The requested index is out of bounds.";
        case ErrCode::NON_INT_ARRAY_SIZE:
            return "Arrays sizes must be integral-constants";


        case ErrCode::NO_SUCH_TYPE:
            return "No such type.";
        case ErrCode::INCOMPATIBLE_TYPES:
            return std::format(
                "Incompatible types! Cannot convert from `{}` to `{}`.",
                ctx.type_1->toString(),
                ctx.type_2->toString()
                );

        case ErrCode::NO_IMPLICIT_CONVERSION:
            return "No implicit conversion is defined for the involved types.";
        case ErrCode::INT_AND_FLOAT_CONV:
            return "Implicit conversion between integral and floating-point types are not allowed.";
        case ErrCode::NO_NARROWING_CONVERSION:
            return std::format(
                "Swirl doesn't support implicit narrowing conversions."
                " Conversion from `{}` to `{}` is narrowing.",
                ctx.type_1->toString(),
                ctx.type_2->toString()
                );
        case ErrCode::NO_SIGNED_UNSIGNED_CONV:
            return std::format(
                "Conversions between signed (`{}`) and unsigned (`{}`) types must be explicit in Swirl, "
                   "use the `as` operator if you intend a conversion.",
                   ctx.type_1->toString(),
                   ctx.type_2->toString()
                   );
        case ErrCode::DISTINCTLY_SIZED_ARR:
            return std::format(
                "The array-types (`{}` and `{}`) are incompatible due to their size difference.",
                ctx.type_1->toString(),
                ctx.type_2->toString()
                );
        case ErrCode::IMMUTABILITY_VIOLATION:
            return std::format(
                "Immutability violation. Cannot convert from `{}` to `{}`.",
                ctx.type_1->toString(),
                ctx.type_2->toString()
                );


        case ErrCode::NO_DIR_IMPORT:
            return "Importing directories is not allowed yet.";
        case ErrCode::DUPLICATE_IMPORT:
            return "Duplicate import here.";
        case ErrCode::MODULE_NOT_FOUND:
            return std::format("Failed to find the module (inferred path: '{}').", ctx.path_1.string());
        case ErrCode::SYMBOL_NOT_FOUND_IN_MOD:
            return std::format("The symbol '{}' doesn't exist in the module.", ctx.str_1);
        case ErrCode::SYMBOL_NOT_EXPORTED:
            return std::format("The symbol '{}' exists, but isn't exported by its parent module.", ctx.str_1);
        case ErrCode::SYMBOL_ALREADY_EXISTS:
            return std::format("The symbol '{}' already exists.", ctx.str_1);
        case ErrCode::NO_SYMBOL_IN_NAMESPACE:
            return std::format(
                "The symbol '{}' does not exist in the namespace defined by '{}'.",
                ctx.str_1,
                ctx.str_2
                );
        case ErrCode::PACKAGE_NOT_FOUND:
            return std::format("No package with the name `{}` is registered.", ctx.str_1);


        case ErrCode::INITIALIZER_REQUIRED:
            return "Initialization is required here.";
        case ErrCode::NON_INTEGRAL_INDICES:
            return std::format("Type `{}` is not integral.", ctx.type_1->toString());
        case ErrCode::RET_TYPE_REQUIRED:
            return "You will have to explicitly specify a return type here.";
        case ErrCode::QUALIFIER_UNDEFINED:
            return std::format("The qualifier '{}' is undefined.", ctx.str_1);
        case ErrCode::UNDEFINED_IDENTIFIER:
            return std::format("The identifier '{}' is undefined.", ctx.str_1);
        case ErrCode::NOT_A_NAMESPACE:
            return std::format("The symbol '{}' doesn't name a namespace.", ctx.str_1);
        case ErrCode::NO_SUCH_MEMBER:
            return std::format("No member called '{}' in struct '{}'.", ctx.str_1, ctx.str_2);
        case ErrCode::CANNOT_ASSIGN_TO_CONST:
            return "Cannot reassign an immutable value.";
        case ErrCode::MAIN_REDEFINED:
            return "Redefinition of the main function!";
        case ErrCode::CONFIG_VAR_UNINITIALIZED:
            return "Configuration variables require initialization.";
        case ErrCode::CONFIG_INIT_NOT_LITERAL:
            return "Configuration variables must be initialized with literals.";
        case ErrCode::SLICE_NOT_COMPATIBLE:
            return "Incompatible slice.";  // TODO
        default:
            throw std::runtime_error("Undefined error code");
    }
}

