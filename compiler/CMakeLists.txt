configure_file(include/SwirlConfig.h.in "${PROJECT_BINARY_DIR}/include/SwirlConfig.h")

find_package(LLVM REQUIRED CONFIG)
find_package(LLD REQUIRED CONFIG)

include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()
option(BUILD_STDLIB "Build the standard library" OFF)

if(BUILD_STDLIB)
    add_subdirectory("../std_lib" build)
endif()

include_directories("include")
include_directories("${PROJECT_BINARY_DIR}")

file(GLOB_RECURSE SOURCES "src/*.cpp")

add_executable(swirl src/swirl.cpp "${SOURCES}")

install(TARGETS swirl 
        RUNTIME DESTINATION bin)

if(WIN32)

    set(LLD_LIBS
        lldCOFF
        lldCommon
        lldMingw
        )

elseif(UNIX)

    set(LLD_LIBS
        lldELF
        lldCommon 
        )

endif()

# only statically link on release
if (CMAKE_BUILD_TYPE STREQUAL "Release")

    if(UNIX)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2 -ffunction-sections -fdata-sections")
        set(CMAKE_EXE_LINKER_FLAGS "-Wl,--gc-sections")
    endif()
    
    llvm_map_components_to_libnames(LLVM_LIBS native lto option)

    target_link_libraries(swirl PRIVATE ${LLD_LIBS} ${LLVM_LIBS})
    add_custom_command(TARGET swirl POST_BUILD
                        COMMAND ${CMAKE_STRIP} $<TARGET_FILE:swirl>
                        COMMENT "Stripping swirl executable")

elseif (CMAKE_BUILD_TYPE STREQUAL "Debug")
    llvm_config(swirl USE_SHARED irreader support core)
    target_link_libraries(swirl PRIVATE ${LLD_LIBS})

endif()

message(STATUS "
Supported build types are Release and Debug (default debug)
You can set the build type with -DCMAKE_BUILD_TYPE=<type>
Release is for statically linking. Debug is for dynamically linking")