#pragma once
#include <mutex>
#include <print>
#include <optional>
#include <filesystem>

#include <lexer/TokenStream.h>


struct Type;
struct Node;
class IdentInfo;
class SourceManager;


struct ErrorContext {
    IdentInfo* ident  = nullptr;
    Type*      type_1 = nullptr;
    Type*      type_2 = nullptr;

    std::filesystem::path path_1;
    std::filesystem::path path_2;

    std::string msg;    // optional error-message, used to convey various syntax errors
    std::string str_1;
    std::string str_2;
    std::optional<StreamState> location = std::nullopt;

    const SourceManager* src_man = nullptr;
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
    CANNOT_ASSIGN_TO_CONST,
    // ----------*----------- //


    // The following error codes are related to the Module System, `path_1`, `path_2` and `str_1` shall be
    // set appropriately to convey context
    NO_DIR_IMPORT,
    DUPLICATE_IMPORT,
    MODULE_NOT_FOUND,         // path_1 in the context shall be set to the import-path
    SYMBOL_NOT_FOUND_IN_MOD,  // when a symbol doesn't exist in a module but usage is attempted
    SYMBOL_NOT_EXPORTED,      // when it exists but is not exported by its parent module, str_1 shall be set
    // ----------*----------- //


    // The following error codes are related to symbols
    NO_SYMBOL_IN_NAMESPACE,   // when no symbol with the name exists in the namespace
    NOT_A_NAMESPACE,          // when a symbol doesn't name a namespace but is attempted to be used as one
    UNDEFINED_IDENTIFIER,     // self-explanatory
    SYMBOL_ALREADY_EXISTS,    // when symbol already exists
    // ----------*----------- //


    TOO_FEW_ARGS,
    INDEX_OUT_OF_BOUNDS,
    NON_INT_ARRAY_SIZE,
    INITIALIZER_REQUIRED,     // when initialization is required but not given
    RET_TYPE_REQUIRED,        // when explicitly specifying a return-type is required (e.g. recursive calls)
    QUALIFIER_UNDEFINED,
    NO_SUCH_MEMBER,
};


class ErrorManager {
public:
    /// Used to report an error, lock-free
    void newError(ErrCode code, const ErrorContext& ctx) {
        m_ErrorCounter++;
        writeToBuffer(generateMessage(code, ctx), ctx);
    }

    /// Used to report an error, acquires the mutex
    void newErrorLocked(ErrCode code, const ErrorContext& ctx) {
        m_ErrorCounter++;
        std::lock_guard guard(m_Mutex);
        writeToBuffer(generateMessage(code, ctx), ctx);
    }

    /// Write all the errors and clear the buffer
    void flush() {
        std::println(
            stderr,
            "{}\n{} error(s) in total reported.\n",
            m_ErrorBuffer,
            m_ErrorCounter
            );
        m_ErrorBuffer.clear();
    }

    /// Returns true if any error exists in the buffer
    [[nodiscard]] bool errorOccurred() const {
        return !m_ErrorBuffer.empty();
    }


private:
    std::string m_ErrorBuffer;
    std::mutex  m_Mutex;

    uint32_t   m_ErrorCounter = 0;


    void writeToBuffer(const std::string& str, const ErrorContext& ctx) {
        std::string backticks;
        backticks.resize(std::format("{} ", ctx.location->Line).size());
        std::fill(backticks.begin(), backticks.end(), ' ');

        backticks.append("|\t");
        backticks.append_range(std::views::repeat(' ') | std::views::take(ctx.location->Col));
        backticks.append("^");

        m_ErrorBuffer += std::format(
            "At {}:{}:{}\n"
            "\n{} |\t{}"  // line no., line
            "{}\n"        // spaces and the `^`
            "\n\tError: {}\n\n",
            ctx.src_man->getSourcePath().string(),
            ctx.location->Line,
            ctx.location->Col,
            ctx.location->Line,
            ctx.src_man->getLineAt(ctx.location->Line),
            backticks,
            str
        );
    }

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
            return "Incompatible types!";
        case ErrCode::NO_IMPLICIT_CONVERSION:
            return "No implicit conversion is defined for the involved types.";
        case ErrCode::INT_AND_FLOAT_CONV:
            return "Implicit conversion between integral and floating-point types are not allowed.";
        case ErrCode::NO_NARROWING_CONVERSION:
            return "Swirl forbids any implicit narrowing conversions.";
        case ErrCode::NO_SIGNED_UNSIGNED_CONV:
            return "Conversions between signed and unsigned types must be explicit in Swirl, "
                   "use the `as` operator if you intend a conversion.";
        case ErrCode::DISTINCTLY_SIZED_ARR:
            return "Arrays involved are incompatible due to size difference.";


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

        case ErrCode::INITIALIZER_REQUIRED:
            return "Initialization is required here.";
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
            return "Cannot reassign a value a variable declared with `let`.";
        default:
            throw std::runtime_error("Undefined error code");
    }
}

