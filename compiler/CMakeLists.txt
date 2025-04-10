configure_file(include/SwirlConfig.h.in "${PROJECT_BINARY_DIR}/include/SwirlConfig.h")

find_package(LLVM REQUIRED CONFIG)
include_directories(${LLVM_INCLUDE_DIRS})
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

file(GLOB_RECURSE SOURCES "src/*/*.cpp")

add_executable(swirl src/swirl.cpp
        "${SOURCES}"
)
target_link_libraries(swirl PUBLIC runtime)

llvm_config(swirl USE_SHARED irreader support core)

