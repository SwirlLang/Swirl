cmake_minimum_required(VERSION 3.5)
project(swirl VERSION 0.0.3)

# specify the C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

if (WIN32)
    set(CMAKE_CXX_STANDARD_LIBRARIES "-static-libgcc -static-libstdc++ -static")
endif (WIN32)
option(BUILD_STDLIB "Build the standard library" OFF)
include_directories("include")
add_executable(swirl src/swirl.cpp include/tokens/Tokens.h)

add_subdirectory(src)
if(BUILD_STDLIB)
    add_subdirectory("../std_lib" build)
endif()

install(TARGETS ${CMAKE_PROJECT_NAME} DESTINATION bin)
install(DIRECTORY include/ DESTINATION include)
