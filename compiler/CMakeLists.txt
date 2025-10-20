configure_file(include/SwirlConfig.h.in "${PROJECT_BINARY_DIR}/include/SwirlConfig.h")

find_package(LLVM REQUIRED CONFIG)
find_package(LLD REQUIRED CONFIG)

include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

option(LLVM_LINK_SHARED ON)

set(LLVM_LINK_MODE "SHARED" CACHE STRING "How to link LLVM: STATIC or SHARED")
set_property(CACHE LLVM_LINK_MODE PROPERTY STRINGS STATIC SHARED)

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
    if(MSVC)
        set(LLD_LIBS
            lldCOFF
            lldCommon
            )
    else() # MinGW
        set(LLD_LIBS
            lldMinGW
            lldCommon
            )
    endif()

elseif(UNIX)

    set(LLD_LIBS
        lldELF
        lldCommon 
        )

endif()

# only statically link
if (LLVM_LINK_STATIC)

    llvm_map_components_to_libnames(LLVM_LIBS native lto option)
    target_link_libraries(swirl PRIVATE ${LLD_LIBS} ${LLVM_LIBS})
    
elseif (LLVM_LINK_SHARED)

    llvm_config(swirl USE_SHARED irreader support core)
    target_link_libraries(swirl PRIVATE ${LLD_LIBS})

endif()

if( CMAKE_BUILD_TYPE STREQUAL "Release" )
    if(UNIX)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2 -ffunction-sections -fdata-sections")
        set(CMAKE_EXE_LINKER_FLAGS "-Wl,--gc-sections")
    endif()
    add_custom_command(TARGET swirl POST_BUILD
                        COMMAND ${CMAKE_STRIP} $<TARGET_FILE:swirl>
                        COMMENT "Stripping swirl executable")
endif()

message(STATUS "
Supported llvm linking options are:
 - LLVM_LINK_STATIC (default off)
 - LLVM_LINK_SHARED (default on)
You can set the linking option with -DLLVM_LINK_<type>=ON/OFF")