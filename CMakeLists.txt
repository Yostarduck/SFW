cmake_minimum_required(VERSION 3.16)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

project(SFW VERSION 1.0)

option(SFW_BUILD_EXAMPLE    "Build the Example project" OFF)
option(SFW_BUILD_SHARED     "Build SFW as a shared library (ON) or static library (OFF)" OFF)

option(SFW_USE_XCB          "Use the XCB backend on Linux (requires xcb library)" OFF)
option(SFW_USE_WAYLAND      "Use the Wayland backend on Linux (requires wayland-client library)" OFF)
option(SFW_USE_X11          "Use the X11 backend on Linux (requires X11 library)" OFF)

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(ARCH_DIR "x64")
else()
    set(ARCH_DIR "x86")
endif()

if(WIN32)
    # 
elseif(LINUX)
    # Exactly one Linux backend must be active; default to XCB when the user has
    # not enabled one explicitly.
    if(NOT SFW_USE_XCB AND NOT SFW_USE_WAYLAND AND NOT SFW_USE_X11)
        set(SFW_USE_XCB ON)
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