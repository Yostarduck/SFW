#include "Linux/X11/SFW_X11Window.h"

namespace SFW
{

namespace Detail
{

inline Atom
internAtom(Display* display, const char* atomName)
{
  return XInternAtom(display, atomName, False);
}

inline void
applyResizableHint(Display* display,
                   ::Window window,
                   const uint32_t width,
                   const uint32_t height,
                   const bool resizable)
{
  if (display == nullptr || window == None) {
    return;
  }

  XSizeHints* hints = XAllocSizeHints();
  if (hints == nullptr) {
    return;
  }

  if (resizable) {
    hints->flags = 0;
  } else {
    hints->flags = PMinSize | PMaxSize;
    hints->min_width = static_cast<int>(width);
    hints->min_height = static_cast<int>(height);
    hints->max_width = static_cast<int>(width);
    hints->max_height = static_cast<int>(height);
  }

  XSetWMNormalHints(display, window, hints);
  XFree(hints);
}

inline void
applyDecoratedHint(Display* display, ::Window window, const bool decorated)
{
  if (display == nullptr || window == None) {
    return;
  }

  const Atom motifHintsAtom = internAtom(display, "_MOTIF_WM_HINTS");
  if (motifHintsAtom == None) {
    return;
  }

  MotifWmHints hints {};
  hints.flags = kMotifHintsDecorations;
  hints.decorations = decorated ? 1u : 0u;

  XChangeProperty(display,
                  window,
                  motifHintsAtom,
                  motifHintsAtom,
                  32,
                  PropModeReplace,
                  reinterpret_cast<const unsigned char*>(&hints),
                  static_cast<int>(sizeof(MotifWmHints) / sizeof(long)));
}

}

#pragma region Lifecycle
void
X11Window::destroy()
{
  if (m_display != nullptr && m_window != None) {
    XDestroyWindow(m_display, m_window);
    m_window = None;
  }

  if (m_display != nullptr) {
    XCloseDisplay(m_display);
    m_display = nullptr;
  }

  m_screen = 0;
  m_rootWindow = None;
  m_wmProtocolsAtom = None;
  m_wmDeleteWindowAtom = None;
  m_netWmStateAtom = None;
  m_netWmStateMaximizedVertAtom = None;
  m_netWmStateMaximizedHorzAtom = None;
  m_netWmNameAtom = None;
  m_utf8StringAtom = None;
  m_title.clear();
  m_x = 0;
  m_y = 0;
  m_width = 0;
  m_height = 0;
  m_framebufferWidth = 0;
  m_framebufferHeight = 0;
  m_isVisible = false;
  m_isResizable = true;
  m_isDecorated = true;
  m_shouldClose = false;
  m_hasFocus = false;
  m_isMaximized = false;
  m_isMinimized = false;
}

bool
X11Window::isCreated() const
{
  return m_display != nullptr && m_window != None;
}

void*
X11Window::getNativeHandle() const
{
  return reinterpret_cast<void*>(m_window);
}
#pragma endregion

#pragma region Event processing
void
X11Window::pollEvents()
{
  if (m_display == nullptr) {
    return;
  }

  while (XPending(m_display) > 0) {
    XEvent event;
    XNextEvent(m_display, &event);

    switch (event.type)
    {
      case ConfigureNotify:
      {
        const XConfigureEvent& configureEvent = event.xconfigure;
        if (configureEvent.window == m_window) {
          m_framebufferWidth = static_cast<uint32_t>(configureEvent.width);
          m_framebufferHeight = static_cast<uint32_t>(configureEvent.height);
        }
        break;
      }

      case ClientMessage:
      {
        const XClientMessageEvent& clientMessageEvent = event.xclient;
        if (clientMessageEvent.message_type == m_wmProtocolsAtom &&
            static_cast<Atom>(clientMessageEvent.data.l[0]) ==
              m_wmDeleteWindowAtom) {
          m_shouldClose = true;
        }
        break;
      }

      case MapNotify:
        m_isVisible = true;
        break;

      case UnmapNotify:
        m_isVisible = false;
        break;

      case FocusIn:
        m_hasFocus = true;
        break;

      case FocusOut:
        m_hasFocus = false;
        break;

      case DestroyNotify:
        m_shouldClose = true;
        break;

      default:
        break;
    }
  }
}

void
X11Window::requestClose()
{
  m_shouldClose = true;
}

bool
X11Window::shouldClose() const
{
  return m_shouldClose;
}
#pragma endregion

#pragma region Visibility and focus
void
X11Window::show()
{
  if (m_display != nullptr && m_window != None && !m_isVisible) {
    XMapWindow(m_display, m_window);
    XFlush(m_display);
    m_isVisible = true;
  }
}

void
X11Window::hide()
{
  if (m_display != nullptr && m_window != None && m_isVisible) {
    XUnmapWindow(m_display, m_window);
    XFlush(m_display);
    m_isVisible = false;
  }
}

bool
X11Window::isVisible() const
{
  return m_isVisible;
}

void
X11Window::focus()
{
  if (m_display != nullptr && m_window != None && m_isVisible) {
    XSetInputFocus(m_display, m_window, RevertToParent, CurrentTime);
    XFlush(m_display);
    m_hasFocus = true;
  }
}

bool
X11Window::hasFocus() const
{
  return m_hasFocus;
}
#pragma endregion

#pragma region Position and size
void
X11Window::setPosition(const int32_t x, const int32_t y)
{
  if (m_display != nullptr && m_window != None) {
    XMoveWindow(m_display, m_window, x, y);
    XFlush(m_display);

    m_x = x;
    m_y = y;
  }
}

void
X11Window::getPosition(int32_t& x, int32_t& y) const
{
  x = m_x;
  y = m_y;
}

void
X11Window::setSize(const uint32_t width, const uint32_t height)
{
  if (m_display != nullptr && m_window != None) {
    // A fixed-size window pins its hints to the new dimensions before the
    // resize so the window manager keeps honouring the non-resizable request.
    if (!m_isResizable) {
      Detail::applyResizableHint(m_display, m_window, width, height, false);
    }

    XResizeWindow(m_display, m_window, width, height);
    XFlush(m_display);

    m_width = width;
    m_height = height;
  }
}

void
X11Window::getSize(uint32_t& width, uint32_t& height) const
{
  width = m_width;
  height = m_height;
}

void
X11Window::getFramebufferSize(uint32_t& width, uint32_t& height) const
{
  width = m_framebufferWidth;
  height = m_framebufferHeight;
}
#pragma endregion

#pragma region Window state
void
X11Window::maximize()
{
  if (m_display != nullptr && m_window != None && !m_isMaximized) {
    sendMaximizedState(true);

    m_isMaximized = true;
    m_isMinimized = false;
  }
}

void
X11Window::minimize()
{
  if (m_display != nullptr && m_window != None && !m_isMinimized) {
    XIconifyWindow(m_display, m_window, m_screen);
    XFlush(m_display);

    m_isMaximized = false;
    m_isMinimized = true;
  }
}

void
X11Window::restore()
{
  if (m_display != nullptr &&
      m_window != None &&
      (m_isMaximized || m_isMinimized)) {
    if (m_isMaximized) {
      sendMaximizedState(false);
    }

    if (m_isMinimized) {
      XMapWindow(m_display, m_window);
    }

    XFlush(m_display);

    m_isMaximized = false;
    m_isMinimized = false;
  }
}

bool
X11Window::isMaximized() const
{
  return m_isMaximized;
}

bool
X11Window::isMinimized() const
{
  return m_isMinimized;
}
#pragma endregion

#pragma region Window attributes
void
X11Window::setTitle(const std::string_view title)
{
  if (m_display != nullptr && m_window != None) {
    m_title = title;
    applyTitle();
    XFlush(m_display);
  }
}

std::string_view
X11Window::getTitle() const
{
  return m_title;
}

void
X11Window::setResizable(const bool resizable)
{
  if (m_display != nullptr && m_window != None) {
    Detail::applyResizableHint(m_display,
                               m_window,
                               m_width,
                               m_height,
                               resizable);
    XFlush(m_display);

    m_isResizable = resizable;
  }
}

bool
X11Window::isResizable() const
{
  return m_isResizable;
}

void
X11Window::setDecorated(const bool decorated)
{
  if (m_display != nullptr && m_window != None) {
    Detail::applyDecoratedHint(m_display, m_window, decorated);
    XFlush(m_display);

    m_isDecorated = decorated;
  }
}

bool
X11Window::isDecorated() const
{
  return m_isDecorated;
}
#pragma endregion

bool
X11Window::createInternal(const WindowCreateInfo& createInfo)
{
  if (m_display != nullptr) {
    destroy();
  }

  m_display = XOpenDisplay(nullptr);
  if (m_display == nullptr) {
    return false;
  }

  m_screen = DefaultScreen(m_display);
  m_rootWindow = RootWindow(m_display, m_screen);

  m_title = createInfo.title;
  m_x = createInfo.x;
  m_y = createInfo.y;
  m_width = createInfo.width;
  m_height = createInfo.height;
  m_framebufferWidth = createInfo.width;
  m_framebufferHeight = createInfo.height;
  m_isVisible = createInfo.visible;
  m_isResizable = createInfo.resizable;
  m_isDecorated = createInfo.decorated;
  m_shouldClose = false;
  m_hasFocus = false;
  m_isMaximized = false;
  m_isMinimized = false;

  XSetWindowAttributes attributes {};
  attributes.background_pixel = BlackPixel(m_display, m_screen);
  attributes.event_mask = ExposureMask |
                          StructureNotifyMask |
                          PropertyChangeMask |
                          FocusChangeMask;

  m_window = XCreateWindow(m_display,
                           m_rootWindow,
                           m_x,
                           m_y,
                           m_width,
                           m_height,
                           0,
                           CopyFromParent,
                           InputOutput,
                           CopyFromParent,
                           CWBackPixel | CWEventMask,
                           &attributes);

  if (m_window == None) {
    XCloseDisplay(m_display);
    m_display = nullptr;
    m_screen = 0;
    m_rootWindow = None;
    return false;
  }

  m_wmProtocolsAtom = Detail::internAtom(m_display, "WM_PROTOCOLS");
  m_wmDeleteWindowAtom = Detail::internAtom(m_display, "WM_DELETE_WINDOW");
  m_netWmStateAtom = Detail::internAtom(m_display, "_NET_WM_STATE");
  m_netWmStateMaximizedVertAtom =
    Detail::internAtom(m_display, "_NET_WM_STATE_MAXIMIZED_VERT");
  m_netWmStateMaximizedHorzAtom =
    Detail::internAtom(m_display, "_NET_WM_STATE_MAXIMIZED_HORZ");
  m_netWmNameAtom = Detail::internAtom(m_display, "_NET_WM_NAME");
  m_utf8StringAtom = Detail::internAtom(m_display, "UTF8_STRING");

  if (m_wmDeleteWindowAtom != None) {
    XSetWMProtocols(m_display, m_window, &m_wmDeleteWindowAtom, 1);
  }

  applyTitle();

  Detail::applyResizableHint(m_display,
                             m_window,
                             m_width,
                             m_height,
                             m_isResizable);
  Detail::applyDecoratedHint(m_display, m_window, m_isDecorated);

  if (m_isVisible) {
    XMapWindow(m_display, m_window);
  }

  XFlush(m_display);
  return true;
}

#pragma region Internal helpers
void
X11Window::applyTitle()
{
  if (m_display == nullptr || m_window == None) {
    return;
  }

  if (m_netWmNameAtom != None && m_utf8StringAtom != None) {
    XChangeProperty(m_display,
                    m_window,
                    m_netWmNameAtom,
                    m_utf8StringAtom,
                    8,
                    PropModeReplace,
                    reinterpret_cast<const unsigned char*>(m_title.data()),
                    static_cast<int>(m_title.size()));
  }

  XStoreName(m_display, m_window, m_title.c_str());
}

void
X11Window::sendMaximizedState(const bool maximized)
{
  if (m_display == nullptr ||
      m_window == None ||
      m_netWmStateAtom == None) {
    return;
  }

  XClientMessageEvent event {};
  event.type = ClientMessage;
  event.window = m_window;
  event.message_type = m_netWmStateAtom;
  event.format = 32;
  event.data.l[0] = maximized ? Detail::kNetWmStateAdd
                              : Detail::kNetWmStateRemove;
  event.data.l[1] = static_cast<long>(m_netWmStateMaximizedVertAtom);
  event.data.l[2] = static_cast<long>(m_netWmStateMaximizedHorzAtom);
  event.data.l[3] = 1;
  event.data.l[4] = 0;

  XSendEvent(m_display,
             m_rootWindow,
             False,
             SubstructureNotifyMask | SubstructureRedirectMask,
             reinterpret_cast<XEvent*>(&event));
  XFlush(m_display);
}
#pragma endregion

}
