file(GLOB src
        cli/cli.cpp
        tokenizer/TokenStream.cpp
        tokenizer/InputStream.cpp
        utils/utils.cpp
        backend/LLVMBackend.cpp
        parser/Parser.cpp
)

target_sources(${PROJECT_NAME} PRIVATE ${src})