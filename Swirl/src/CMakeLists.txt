file(GLOB src
        transpiler/builtins.txt
    exception_handler/ExceptionHandler.cpp
        parser/class/ClassParser.cpp
        parser/functions/FunctionParser.cpp
    pre-processor/pre-processor.cpp
    swirl.complex-nums/Complex.cpp
    swirl.integer/Int.cpp
        swirl.map/Map.c
    swirl.string/String.cpp
        tokenizer/TokenStream.cpp
        tokenizer/InputStream.cpp

#    utils/logger.cpp
    utils/utils.cpp
    builtins/builtins.txt
    transpiler/transpiler.cpp
        parser/parser.cpp
        exception/exception.cpp
)

target_sources(${PROJECT_NAME} PRIVATE ${src})