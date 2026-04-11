cmake_minimum_required(VERSION 3.16)

set(APP_VERSION_MAJOR 1)
set(APP_VERSION_MINOR 0)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/SFWConfig.h.in
               ${CMAKE_CURRENT_BINARY_DIR}/include/SFWConfig.h)

add_executable(App
               source/main.cpp)

target_include_directories(App PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include
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

        target_compile_definitions(App PRIVATE SFW_USE_XCB)
        target_include_directories(App PRIVATE ${XCB_INCLUDE_DIRS})
        target_link_libraries(App PRIVATE ${XCB_LIBRARIES})
    elseif(SFW_USE_WAYLAND)
        find_package(PkgConfig REQUIRED)
        pkg_check_modules(WAYLAND_CLIENT REQUIRED wayland-client)

        target_compile_definitions(App PRIVATE SFW_USE_WAYLAND)
        target_include_directories(App PRIVATE ${WAYLAND_CLIENT_INCLUDE_DIRS})
        target_link_libraries(App PRIVATE ${WAYLAND_CLIENT_LIBRARIES})
    elseif(SFW_USE_X11)
        find_package(X11 REQUIRED)

        target_compile_definitions(App PRIVATE SFW_USE_X11)
        target_include_directories(App PRIVATE ${X11_INCLUDE_DIR})
        target_link_libraries(App PRIVATE ${X11_LIBRARIES})
    else()
        message(FATAL_ERROR "On Linux, set one backend: -DSFW_USE_WAYLAND=ON, -DSFW_USE_XCB=ON, or -DSFW_USE_X11=ON")
    endif()
elseif(APPLE)
    #
endif()

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/include/SFWConfig.h DESTINATION include)