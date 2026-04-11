#pragma once

#if defined(SFW_WINDOWS)
  #include "SFW_WindowsWindow.h"

#elif defined(SFW_LINUX)
  #if defined(SFW_USE_WAYLAND)
    #include "SFW_WaylandWindow.h"
  #elif defined(SFW_USE_XCB)
    #include "SFW_XCBWindow.h"
  #else // SFW_USE_X11
    #include "SFW_X11Window.h"
  #endif

#elif defined(SFW_MACOS)
  #include "SFW_MacOSWindow.h"

#else
  #error "Unsupported platform"

#endif