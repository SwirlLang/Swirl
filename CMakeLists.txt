cmake_minimum_required(VERSION 3.20)
project(swirl VERSION 0.0.6)
enable_testing()

add_subdirectory(compiler)

if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    install(FILES "misc/man/swirl.1" DESTINATION share/man/man1)
    install(DIRECTORY "misc/lib/" DESTINATION lib/swirl)
endif (${CMAKE_SYSTEM_NAME} MATCHES "Linux")

include(CPackConfig.cmake)
