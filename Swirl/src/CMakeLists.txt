file(GLOB src
        transpiler/builtins.txt
    exception_handler/ExceptionHandler.cpp
    parsers/class/ClassParser.cpp
    parsers/functions/FunctionParser.cpp
    pre-processor/pre-processor.cpp
    swirl.complex-nums/Complex.cpp
    swirl.integer/Int.cpp
    swirl.map/Map.cpp
    swirl.string/String.cpp
    tokenizer/tokenizer.cpp
    utils/logger.cpp
    utils/utils.cpp
        builtins/builtins.txt
#        transpiler/transpiler.cpp
)

target_sources(${PROJECT_NAME} PRIVATE ${src})