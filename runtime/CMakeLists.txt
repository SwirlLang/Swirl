add_library(runtime src/runtime.c)
target_include_directories(runtime PUBLIC "include")
install(TARGETS runtime
        LIBRARY DESTINATION lib)