cmake_minimum_required(VERSION 3.16)

set(APP_VERSION_MAJOR 1)
set(APP_VERSION_MINOR 0)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/SFWConfig.h.in
               ${CMAKE_CURRENT_BINARY_DIR}/include/SFWConfig.h)

option(SFW_BUILD_SHARED "Build SFW as a shared library (ON) or static library (OFF)" OFF)

option(SFW_USE_XCB "Use the XCB backend on Linux (requires xcb library)" OFF)
option(SFW_USE_WAYLAND "Use the Wayland backend on Linux (requires wayland-client library)" OFF)
option(SFW_USE_X11 "Use the X11 backend on Linux (requires X11 library)" OFF)

option(SFW_USE_WIN "Use the Win32 backend on Windows (requires Windows SDK)" OFF)

option(SFW_USE_COCOA "Use the Cocoa backend on macOS (requires Cocoa framework)" OFF)

set(SFW_SOURCES)

if(WIN32)
    # TODO: Add Win32 sources when backend is implemented.
elseif(LINUX)
    if(SFW_USE_XCB)
        list(APPEND SFW_SOURCES
             ${CMAKE_CURRENT_SOURCE_DIR}/source/Linux/XCB/SFW_XCBWindow.cpp)
    elseif(SFW_USE_WAYLAND)
        message(FATAL_ERROR "SFW_USE_WAYLAND is enabled, but Wayland backend sources are not added yet.")
    elseif(SFW_USE_X11)
        message(FATAL_ERROR "SFW_USE_X11 is enabled, but X11 backend sources are not added yet.")
    endif()
elseif(APPLE)
    # TODO: Add Cocoa sources when backend is implemented.
endif()

set(SFW_LIBRARY_TYPE STATIC)
if(SFW_BUILD_SHARED)
    set(SFW_LIBRARY_TYPE SHARED)
endif()

add_library(SFW ${SFW_LIBRARY_TYPE}
            ${SFW_SOURCES})

target_include_directories(SFW PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include
                                       ${CMAKE_CURRENT_BINARY_DIR}/include)

if(WIN32)
    #
elseif(LINUX)
    # Only one backend can be used at a time on Linux
    # Default to XCB if none are set
    if (NOT SFW_USE_XCB AND NOT SFW_USE_WAYLAND AND NOT SFW_USE_X11)
      set(SFW_USE_XCB ON)
    endif()

    if(SFW_USE_XCB)
        find_package(PkgConfig REQUIRED)
        pkg_check_modules(XCB REQUIRED xcb)

        target_compile_definitions(SFW PRIVATE SFW_USE_XCB)
        target_include_directories(SFW PRIVATE ${XCB_INCLUDE_DIRS})
        target_link_libraries(SFW PRIVATE ${XCB_LIBRARIES})
    elseif(SFW_USE_WAYLAND)
        find_package(PkgConfig REQUIRED)
        pkg_check_modules(WAYLAND_CLIENT REQUIRED wayland-client)

        target_compile_definitions(SFW PRIVATE SFW_USE_WAYLAND)
        target_include_directories(SFW PRIVATE ${WAYLAND_CLIENT_INCLUDE_DIRS})
        target_link_libraries(SFW PRIVATE ${WAYLAND_CLIENT_LIBRARIES})
    elseif(SFW_USE_X11)
        find_package(X11 REQUIRED)

        target_compile_definitions(SFW PRIVATE SFW_USE_X11)
        target_include_directories(SFW PRIVATE ${X11_INCLUDE_DIR})
        target_link_libraries(SFW PRIVATE ${X11_LIBRARIES})
    else()
        message(FATAL_ERROR "On Linux, set one backend: -DSFW_USE_WAYLAND=ON, -DSFW_USE_XCB=ON, or -DSFW_USE_X11=ON")
    endif()
elseif(APPLE)
    #
endif()

install(TARGETS SFW
        ARCHIVE DESTINATION lib
        LIBRARY DESTINATION lib
        RUNTIME DESTINATION bin)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/include/SFWConfig.h DESTINATION include)