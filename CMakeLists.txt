cmake_minimum_required(VERSION 3.20)
project(swirl VERSION 0.0.5)

add_subdirectory(compiler)
add_subdirectory(runtime)

if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    install(FILES "misc/man/swirl.1" DESTINATION share/man/man1)
endif (${CMAKE_SYSTEM_NAME} MATCHES "Linux")

include(CPackConfig.cmake)
