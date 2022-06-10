cmake_minimum_required(VERSION 3.5)
project(Swirl VERSION 0.0.1)

# specify the C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

include_directories("include")

add_executable(Swirl src/swirl.cpp)

add_subdirectory(src)
add_subdirectory("../std_lib" build)

install(TARGETS ${CMAKE_PROJECT_NAME} DESTINATION bin)
install(DIRECTORY include/ DESTINATION include)