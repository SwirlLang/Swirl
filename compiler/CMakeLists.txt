configure_file(include/SwirlConfig.h.in "${PROJECT_BINARY_DIR}/include/SwirlConfig.h")

find_package(LLVM REQUIRED CONFIG)

include_directories(${LLVM_INCLUDE_DIRS})
include_directories(${CLANG_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})


set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
option(BUILD_STDLIB "Build the standard library" OFF)

if(BUILD_STDLIB)
    add_subdirectory("../std_lib" build)
endif()


# for windows
if (WIN32)
    set(CMAKE_CXX_STANDARD_LIBRARIES "-static-libgcc -static-libstdc++ -static")
endif (WIN32)

include_directories("include")
include_directories("${PROJECT_BINARY_DIR}")

file(GLOB_RECURSE SOURCES "src/*.cpp")

add_executable(swirl src/swirl.cpp
        "${SOURCES}"
)

install(TARGETS swirl 
        RUNTIME DESTINATION bin)

# only statically link on release
if (CMAKE_BUILD_TYPE STREQUAL "Release")
    find_package(ZLIB REQUIRED)
    find_package(LibXml2 REQUIRED)

    llvm_map_components_to_libnames(LLVM_LIBS native)

    target_link_libraries(swirl PRIVATE ${LLVM_LIBS} ${ZLIB_LIBRARIES} ${FFI_LIBRARIES} ${LIBXML2_LIBRARIES})
        add_custom_command(TARGET swirl POST_BUILD
                            COMMAND ${CMAKE_STRIP} $<TARGET_FILE:swirl>
                            COMMENT "Stripping swirl executable")
endif()

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    llvm_config(swirl USE_SHARED irreader support core)
endif()


    