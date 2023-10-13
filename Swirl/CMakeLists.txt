cmake_minimum_required(VERSION 3.5)
project(swirl VERSION 0.0.5)

find_package(LLVM REQUIRED CONFIG)
include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})


configure_file(include/SwirlConfig.h.in include/SwirlConfig.h)

# specify the C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# for windows
if (WIN32)
    set(CMAKE_CXX_STANDARD_LIBRARIES "-static-libgcc -static-libstdc++ -static")
endif (WIN32)

option(BUILD_STDLIB "Build the standard library" OFF)
include_directories("include")
include_directories("${PROJECT_BINARY_DIR}")

add_executable(swirl src/swirl.cpp)
add_compile_options(-O2)

llvm_map_components_to_libnames(llvm_libs support core irreader)
target_link_libraries(swirl ${llvm-libs})

add_subdirectory(src)
if(BUILD_STDLIB)
    add_subdirectory("../std_lib" build)
endif()

install(TARGETS ${CMAKE_PROJECT_NAME} DESTINATION bin)

if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    install(FILES "man/swirl.1" DESTINATION share/man/man1)
endif (${CMAKE_SYSTEM_NAME} MATCHES "Linux")

include(CPackConfig.cmake)