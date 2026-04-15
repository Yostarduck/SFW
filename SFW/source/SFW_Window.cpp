#include "SFW_Window.h"

#if defined(SFW_WINDOWS)
  #include "Windows/SFW_WindowsWindow.h"
#elif defined(SFW_LINUX)
    #if defined(SFW_USE_XCB)
      #include "Linux/XCB/SFW_XCBWindow.h"
    #elif defined(SFW_USE_WAYLAND)
      #include "Linux/Wayland/SFW_WaylandWindow.h"
    #elif defined(SFW_USE_X11)
      #include "Linux/X11/SFW_X11Window.h"
    #endif
#elif defined(SFW_MACOS)
  #include "MacOS/SFW_MacOSWindow.h"
#endif

namespace SFW
{

Window*
Window::create(const WindowCreateInfo& createInfo) {

#if defined(SFW_WINDOWS)
  // TODO: Implement Windows window creation
#elif defined(SFW_LINUX)
    #if defined(SFW_USE_XCB)
      XCBWindow* window = new XCBWindow();
      if (!window->createInternal(createInfo)) {
        delete window;
        return nullptr;
      }

      return window;
    #elif defined(SFW_USE_WAYLAND)
      // TODO: Implement Wayland window creation
    #elif defined(SFW_USE_X11)
      // TODO: Implement X11 window creation
    #endif
#elif defined(SFW_MACOS)
  // TODO: Implement MacOS window creation
#endif

  return nullptr;
}

}