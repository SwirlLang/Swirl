#include "errors/ErrorManager.h"
#include "errors/ErrorPipeline.h"
#include "types/SwTypes.h"


void ErrorManager::newError(const ErrCode code, const ErrorContext &ctx) const {
    m_OutputPipeline->write(generateMessage(code, ctx), ctx);
}


void ErrorManager::newErrorLocked(const ErrCode code, const ErrorContext &ctx) {
    std::lock_guard guard(m_Mutex);
    m_OutputPipeline->write(generateMessage(code, ctx), ctx);
}


void ErrorManager::flush() const {
    m_OutputPipeline->flush();
}


[[nodiscard]]
bool ErrorManager::errorOccurred() const {
    return m_OutputPipeline->errorOccurred();
}


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
        case ErrCode::UNEXPECTED_KEYWORD:
            return std::format("Unexpected keyword `{}`.", ctx.str_1);


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
                "Swirl doesn't allow implicit narrowing conversions."
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
        case ErrCode::NOT_DEREFERENCE_ABLE:
            return std::format(
                "The type {} cannot be dereferenced.",
                ctx.type_1->toString()
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

        case ErrCode::NOT_ALLOWED_CT_CTX:
            return "This construct is not allowed in compile-time evaluated context.";
        case ErrCode::NOT_A_CT_VAR:
            return "Only other comptime variables' IDs can be written in this context.";
        case ErrCode::OP_NOT_ALLOWED_HERE:
            return std::format("The operator '{}' isn't allowed in this context.", ctx.str_1);
        default:
            throw std::runtime_error("Undefined error code");
    }
}