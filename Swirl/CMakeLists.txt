cmake_minimum_required(VERSION 3.5)
project(Swirl VERSION 0.0.4)
configure_file(include/SwirlConfig.h.in include/SwirlConfig.h)

# specify the C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

if (WIN32)
    set(CMAKE_CXX_STANDARD_LIBRARIES "-static-libgcc -static-libstdc++ -static")
endif (WIN32)

option(BUILD_STDLIB "Build the standard library" OFF)
include_directories("include")
include_directories("${PROJECT_BINARY_DIR}")

add_executable(Swirl src/swirl.cpp)
add_compile_options(-O3)

add_subdirectory(src)
if(BUILD_STDLIB)
    add_subdirectory("../std_lib" build)
endif()

install(TARGETS ${CMAKE_PROJECT_NAME} DESTINATION bin)
install(DIRECTORY include/ DESTINATION include)
