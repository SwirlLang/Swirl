file(GLOB_RECURSE TEST_SOURCES "../src/*.cpp")
list(REMOVE_ITEM TEST_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/../src/swirl.cpp")

add_executable(compiler_tests
    ${TEST_SOURCES}
    test_main.cpp
    test_lexer.cpp
)

target_link_libraries(compiler_tests PUBLIC Catch2::Catch2)

# Link LLVM (mirrors the main executable setup)
if(LLVM_LINK_STATIC)
    target_link_libraries(compiler_tests PRIVATE ${LLD_LIBS} ${LLVM_LIBS})
elseif(LLVM_LINK_SHARED)
    llvm_config(compiler_tests USE_SHARED irreader support core)
    target_link_libraries(compiler_tests PRIVATE ${LLD_LIBS})
endif()

# Enable Catch2 test discovery
list(PREPEND CMAKE_MODULE_PATH "${catch2_SOURCE_DIR}/extras")
include(Catch)
catch_discover_tests(compiler_tests)
