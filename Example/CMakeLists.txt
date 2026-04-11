cmake_minimum_required(VERSION 3.16)

set(APP_VERSION_MAJOR 1)
set(APP_VERSION_MINOR 0)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/ExampleConfig.h.in
               ${CMAKE_CURRENT_BINARY_DIR}/include/ExampleConfig.h)

add_executable(Example
               source/main.cpp)

target_include_directories(Example PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include
                                           ${CMAKE_CURRENT_BINARY_DIR}/include)


install(FILES ${CMAKE_CURRENT_BINARY_DIR}/include/ExampleConfig.h DESTINATION include)