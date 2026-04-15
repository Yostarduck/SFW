cmake_minimum_required(VERSION 3.16)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

project(SFW VERSION 1.0)

option(SFW_BUILD_EXAMPLE "Build the Example project" OFF)

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(ARCH_DIR "x64")
else()
    set(ARCH_DIR "x86")
endif()

if(WIN32)
    # 
elseif(LINUX)
    # Only set Linux backend defaults when no backend option is explicitly set.
    if(NOT DEFINED SFW_USE_XCB AND NOT DEFINED SFW_USE_WAYLAND AND NOT DEFINED SFW_USE_X11)
        set(SFW_USE_XCB     ON  CACHE BOOL "Use XCB backend on Linux")
        set(SFW_USE_WAYLAND OFF CACHE BOOL "Use Wayland backend on Linux")
        set(SFW_USE_X11     OFF CACHE BOOL "Use X11 backend on Linux")
    endif()
elseif(APPLE)
    #
else()
    message(FATAL_ERROR "Unsupported platform")
endif()

# Normalize common config names to "Capitalized" form
set(CONFIG_DIR "$<IF:$<CONFIG:Debug>,Debug,$<IF:$<CONFIG:Release>,Release,$<IF:$<CONFIG:RelWithDebInfo>,Relwithdebinfo,$<IF:$<CONFIG:MinSizeRel>,Minsizerel,$<CONFIG>>>>>")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${ARCH_DIR}/${CONFIG_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${ARCH_DIR}/${CONFIG_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${ARCH_DIR}/${CONFIG_DIR})

foreach(OUTPUTCONFIG Debug Release RelWithDebInfo MinSizeRel)
    foreach(OUTPUTTYPE RUNTIME LIBRARY ARCHIVE)
        set(CMAKE_${OUTPUTTYPE}_OUTPUT_DIRECTORY_${OUTPUTCONFIG}
            ${CMAKE_BINARY_DIR}/${ARCH_DIR}/${OUTPUTCONFIG})
    endforeach()
endforeach()

if(WIN32)
    add_compile_definitions(SFW_WINDOWS)
elseif(LINUX)
    add_compile_definitions(SFW_LINUX)
elseif(APPLE)
    add_compile_definitions(SFW_MACOS)
endif()

add_compile_definitions($<$<CONFIG:Debug>:DEBUG>
                        $<$<CONFIG:Release>:RELEASE>)

# SFW
add_subdirectory(SFW)

# Example
if(SFW_BUILD_EXAMPLE)
    add_subdirectory(Example)
endif()