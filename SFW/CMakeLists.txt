cmake_minimum_required(VERSION 3.16)

set(APP_VERSION_MAJOR 1)
set(APP_VERSION_MINOR 0)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/SFWConfig.h.in
               ${CMAKE_CURRENT_BINARY_DIR}/include/SFWConfig.h)

set(SFW_SOURCES)
list(APPEND SFW_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/source/SFW_Window.cpp)

if(WIN32)
    list(APPEND SFW_SOURCES
         ${CMAKE_CURRENT_SOURCE_DIR}/source/Windows/SFW_Win32Window.cpp)
elseif(LINUX)
    if(SFW_USE_XCB)
        list(APPEND SFW_SOURCES
             ${CMAKE_CURRENT_SOURCE_DIR}/source/Linux/XCB/SFW_XCBWindow.cpp)
    elseif(SFW_USE_WAYLAND)
        # The Wayland backend talks xdg-shell, whose client glue is generated
        # from protocol XML by wayland-scanner. Locate the scanner and the
        # wayland-protocols data directory up front.
        find_package(PkgConfig REQUIRED)
        pkg_check_modules(WAYLAND_PROTOCOLS REQUIRED wayland-protocols)
        pkg_get_variable(WAYLAND_PROTOCOLS_DIR wayland-protocols pkgdatadir)
        pkg_get_variable(WAYLAND_SCANNER wayland-scanner wayland_scanner)
        if(NOT WAYLAND_SCANNER)
            find_program(WAYLAND_SCANNER NAMES wayland-scanner REQUIRED)
        endif()

        set(SFW_WAYLAND_GEN_DIR ${CMAKE_CURRENT_BINARY_DIR}/wayland-generated)
        file(MAKE_DIRECTORY ${SFW_WAYLAND_GEN_DIR})

        # protocol name -> source XML, relative to the wayland-protocols dir
        set(SFW_WAYLAND_PROTOCOLS
            "xdg-shell=stable/xdg-shell/xdg-shell.xml"
            "xdg-decoration=unstable/xdg-decoration/xdg-decoration-unstable-v1.xml")

        foreach(SFW_WAYLAND_PROTOCOL ${SFW_WAYLAND_PROTOCOLS})
            string(REPLACE "=" ";" SFW_WAYLAND_PROTOCOL_PARTS ${SFW_WAYLAND_PROTOCOL})
            list(GET SFW_WAYLAND_PROTOCOL_PARTS 0 SFW_WAYLAND_PROTOCOL_NAME)
            list(GET SFW_WAYLAND_PROTOCOL_PARTS 1 SFW_WAYLAND_PROTOCOL_XML)

            set(SFW_WAYLAND_PROTOCOL_PATH
                ${WAYLAND_PROTOCOLS_DIR}/${SFW_WAYLAND_PROTOCOL_XML})
            set(SFW_WAYLAND_PROTOCOL_HEADER
                ${SFW_WAYLAND_GEN_DIR}/${SFW_WAYLAND_PROTOCOL_NAME}-client-protocol.h)
            set(SFW_WAYLAND_PROTOCOL_SOURCE
                ${SFW_WAYLAND_GEN_DIR}/${SFW_WAYLAND_PROTOCOL_NAME}-protocol.c)

            add_custom_command(
                OUTPUT ${SFW_WAYLAND_PROTOCOL_HEADER}
                COMMAND ${WAYLAND_SCANNER} client-header
                        ${SFW_WAYLAND_PROTOCOL_PATH} ${SFW_WAYLAND_PROTOCOL_HEADER}
                DEPENDS ${SFW_WAYLAND_PROTOCOL_PATH}
                VERBATIM)
            add_custom_command(
                OUTPUT ${SFW_WAYLAND_PROTOCOL_SOURCE}
                COMMAND ${WAYLAND_SCANNER} private-code
                        ${SFW_WAYLAND_PROTOCOL_PATH} ${SFW_WAYLAND_PROTOCOL_SOURCE}
                DEPENDS ${SFW_WAYLAND_PROTOCOL_PATH}
                VERBATIM)

            list(APPEND SFW_SOURCES
                 ${SFW_WAYLAND_PROTOCOL_HEADER}
                 ${SFW_WAYLAND_PROTOCOL_SOURCE})
        endforeach()

        list(APPEND SFW_SOURCES
             ${CMAKE_CURRENT_SOURCE_DIR}/source/Linux/Wayland/SFW_WaylandWindow.cpp)
    elseif(SFW_USE_X11)
        list(APPEND SFW_SOURCES
             ${CMAKE_CURRENT_SOURCE_DIR}/source/Linux/X11/SFW_X11Window.cpp)
    endif()
elseif(APPLE)
    message(FATAL_ERROR "Apple backend not implement yet.")
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
    target_link_libraries(SFW PRIVATE user32 gdi32)
elseif(LINUX)
    if(SFW_USE_XCB)
        find_package(PkgConfig REQUIRED)
        pkg_check_modules(XCB REQUIRED xcb)

        target_compile_definitions(SFW PRIVATE SFW_USE_XCB)
        target_include_directories(SFW PRIVATE ${XCB_INCLUDE_DIRS})
        target_link_libraries(SFW PRIVATE ${XCB_LIBRARIES})
    elseif(SFW_USE_WAYLAND)
        find_package(PkgConfig REQUIRED)
        pkg_check_modules(WAYLAND_CLIENT REQUIRED wayland-client)

        # _GNU_SOURCE exposes memfd_create, used to back the surface buffer.
        target_compile_definitions(SFW PRIVATE SFW_USE_WAYLAND _GNU_SOURCE)
        target_include_directories(SFW PRIVATE ${WAYLAND_CLIENT_INCLUDE_DIRS}
                                               ${SFW_WAYLAND_GEN_DIR})
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