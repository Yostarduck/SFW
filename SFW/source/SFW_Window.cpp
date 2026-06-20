#include "SFW_Window.h"

#if defined(SFW_WINDOWS)
  #include "Windows/SFW_Win32Window.h"
#elif defined(SFW_LINUX)
    #if defined(SFW_USE_XCB)
      #include "Linux/XCB/SFW_XCBWindow.h"
    #elif defined(SFW_USE_WAYLAND)
      #include "Linux/Wayland/SFW_WaylandWindow.h"
    #elif defined(SFW_USE_X11)
      #include "Linux/X11/SFW_X11Window.h"
    #endif
#elif defined(SFW_MACOS)
  //#include "MacOS/SFW_MacOSWindow.h"
#endif

namespace SFW
{

Window*
Window::create(const WindowCreateInfo& createInfo)
{
  Window* window = nullptr;

#if defined(SFW_WINDOWS)
  window = new Win32Window();
#elif defined(SFW_LINUX)
    #if defined(SFW_USE_XCB)
      window = new XCBWindow();
    #elif defined(SFW_USE_WAYLAND)
      window = new WaylandWindow();
    #elif defined(SFW_USE_X11)
      window = new X11Window();
    #endif
#elif defined(SFW_MACOS)
  // TODO: Implement MacOS window creation
#endif

  if (!window->createInternal(createInfo)) {
    delete window;
    window = nullptr;
  }

  return window;
}

}
