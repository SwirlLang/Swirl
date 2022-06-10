file(GLOB src_files os/*.cpp math/*.cpp)

add_library(std_lib STATIC ${src_files})

install(TARGETS std_lib DESTINATION lib)
install(FILES os/os.h math/math.h DESTINATION include)

target_include_directories(std_lib PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/os" "${CMAKE_CURRENT_SOURCE_DIR}/math")