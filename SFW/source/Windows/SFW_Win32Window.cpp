#include "Windows/SFW_Win32Window.h"

namespace SFW
{

namespace Detail
{

inline DWORD
windowStyle(const bool resizable, const bool decorated) {
  if (!decorated) {
    return WS_POPUP;
  }

  DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
  if (resizable) {
    style |= WS_THICKFRAME | WS_MAXIMIZEBOX;
  }
  return style;
}

inline std::wstring
toWideString(const std::string_view str) {
  if (str.empty()) {
    return {};
  }

  const int len = MultiByteToWideChar(CP_UTF8,
                                      0,
                                      str.data(),
                                      static_cast<int>(str.size()),
                                      nullptr,
                                      0);
  std::wstring result(static_cast<size_t>(len), L'\0');
  MultiByteToWideChar(CP_UTF8,
                      0,
                      str.data(),
                      static_cast<int>(str.size()),
                      result.data(),
                      len);
  return result;
}

}

#pragma region Lifecycle
bool
Win32Window::createInternal(const WindowCreateInfo& createInfo) {
  if (m_hwnd != nullptr) {
    destroy();
  }

  m_hInstance = GetModuleHandleW(nullptr);
  if (m_hInstance == nullptr) {
    return false;
  }

  WNDCLASSEXW wc {};
  wc.cbSize        = sizeof(WNDCLASSEXW);
  wc.style         = CS_HREDRAW | CS_VREDRAW;
  wc.lpfnWndProc   = windowProc;
  wc.hInstance     = m_hInstance;
  wc.hCursor       = LoadCursor(nullptr, IDC_ARROW);
  wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
  wc.lpszClassName = Detail::kWindowClassName;

  if (!RegisterClassExW(&wc)) {
    if (GetLastError() != ERROR_CLASS_ALREADY_EXISTS) {
      m_hInstance = nullptr;
      return false;
    }
  }

  m_title       = createInfo.title;
  m_x           = createInfo.x;
  m_y           = createInfo.y;
  m_width       = createInfo.width;
  m_height      = createInfo.height;
  m_isVisible   = createInfo.visible;
  m_isResizable = createInfo.resizable;
  m_isDecorated = createInfo.decorated;
  m_shouldClose = false;
  m_hasFocus    = false;
  m_isMaximized = false;
  m_isMinimized = false;

  const DWORD style = Detail::windowStyle(m_isResizable, m_isDecorated);

  RECT rect = {0,
               0,
               static_cast<LONG>(m_width),
               static_cast<LONG>(m_height)};
  AdjustWindowRectEx(&rect, style, FALSE, 0);

  const int32_t windowWidth  = static_cast<int32_t>(rect.right - rect.left);
  const int32_t windowHeight = static_cast<int32_t>(rect.bottom - rect.top);
  const std::wstring wideTitle = Detail::toWideString(m_title);

  m_hwnd = CreateWindowExW(0,
                           Detail::kWindowClassName,
                           wideTitle.c_str(),
                           style,
                           m_x, m_y,
                           windowWidth, windowHeight,
                           nullptr,
                           nullptr,
                           m_hInstance,
                           this);

  if (m_hwnd == nullptr) {
    m_hInstance = nullptr;
    return false;
  }

  if (m_isVisible) {
    ShowWindow(m_hwnd, SW_SHOW);
    UpdateWindow(m_hwnd);
  }

  return true;
}

void
Win32Window::destroy() {
  if (m_hwnd != nullptr) {
    DestroyWindow(m_hwnd);
    m_hwnd = nullptr;
  }

  m_hInstance   = nullptr;
  m_title.clear();
  m_x           = 0;
  m_y           = 0;
  m_width       = 0;
  m_height      = 0;
  m_isVisible   = false;
  m_isResizable = true;
  m_isDecorated = true;
  m_shouldClose = false;
  m_hasFocus    = false;
  m_isMaximized = false;
  m_isMinimized = false;
}

bool
Win32Window::isCreated() const {
  return m_hwnd != nullptr;
}

void*
Win32Window::getNativeHandle() const {
  return m_hwnd;
}
#pragma endregion

#pragma region Event processing
void
Win32Window::pollEvents() {
  if (m_hwnd == nullptr) {
    return;
  }

  MSG msg {};
  while (PeekMessageW(&msg, m_hwnd, 0, 0, PM_REMOVE)) {
    TranslateMessage(&msg);
    DispatchMessageW(&msg);
  }
}

void
Win32Window::requestClose() {
  m_shouldClose = true;
}

bool
Win32Window::shouldClose() const {
  return m_shouldClose;
}
#pragma endregion

#pragma region Visibility and focus
void
Win32Window::show() {
  if (m_hwnd != nullptr && !m_isVisible) {
    ShowWindow(m_hwnd, SW_SHOW);
    m_isVisible = true;
  }
}

void
Win32Window::hide() {
  if (m_hwnd != nullptr && m_isVisible) {
    ShowWindow(m_hwnd, SW_HIDE);
    m_isVisible = false;
  }
}

bool
Win32Window::isVisible() const {
  return m_isVisible;
}

void
Win32Window::focus() {
  if (m_hwnd != nullptr) {
    SetForegroundWindow(m_hwnd);
    SetFocus(m_hwnd);
  }
}

bool
Win32Window::hasFocus() const {
  return m_hasFocus;
}
#pragma endregion

#pragma region Position and size
void
Win32Window::setPosition(const int32_t x, const int32_t y) {
  if (m_hwnd == nullptr) {
    return;
  }

  SetWindowPos(m_hwnd, nullptr, x, y, 0, 0,
               SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
  m_x = x;
  m_y = y;
}

void
Win32Window::getPosition(int32_t& x, int32_t& y) const {
  x = m_x;
  y = m_y;
}

void
Win32Window::setSize(const uint32_t width, const uint32_t height) {
  if (m_hwnd == nullptr) {
    return;
  }

  const DWORD style =
    static_cast<DWORD>(GetWindowLongPtrW(m_hwnd, GWL_STYLE));
  RECT rect = {0, 0,
               static_cast<LONG>(width),
               static_cast<LONG>(height)};
  AdjustWindowRectEx(&rect, style, FALSE, 0);

  SetWindowPos(m_hwnd,
               nullptr,
               0,
               0,
               rect.right - rect.left,
               rect.bottom - rect.top,
               SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
  m_width  = width;
  m_height = height;
}

void
Win32Window::getSize(uint32_t& width, uint32_t& height) const {
  width  = m_width;
  height = m_height;
}

void
Win32Window::getFramebufferSize(uint32_t& width, uint32_t& height) const {
  if (m_hwnd == nullptr) {
    width  = 0;
    height = 0;
    return;
  }

  RECT rect {};
  GetClientRect(m_hwnd, &rect);
  width  = static_cast<uint32_t>(rect.right);
  height = static_cast<uint32_t>(rect.bottom);
}
#pragma endregion

#pragma region Window state
void
Win32Window::maximize() {
  if (m_hwnd != nullptr && !m_isMaximized) {
    ShowWindow(m_hwnd, SW_MAXIMIZE);
    m_isMaximized = true;
    m_isMinimized = false;
  }
}

void
Win32Window::minimize() {
  if (m_hwnd != nullptr && !m_isMinimized) {
    ShowWindow(m_hwnd, SW_MINIMIZE);
    m_isMaximized = false;
    m_isMinimized = true;
  }
}

void
Win32Window::restore() {
  if (m_hwnd != nullptr && (m_isMaximized || m_isMinimized)) {
    ShowWindow(m_hwnd, SW_RESTORE);
    m_isMaximized = false;
    m_isMinimized = false;
  }
}

bool
Win32Window::isMaximized() const {
  return m_isMaximized;
}

bool
Win32Window::isMinimized() const {
  return m_isMinimized;
}
#pragma endregion

#pragma region Window attributes
void
Win32Window::setTitle(const std::string_view title) {
  if (m_hwnd == nullptr) {
    return;
  }

  const std::wstring wideTitle = Detail::toWideString(title);
  SetWindowTextW(m_hwnd, wideTitle.c_str());
  m_title = title;
}

std::string_view
Win32Window::getTitle() const {
  return m_title;
}

void
Win32Window::setResizable(const bool resizable) {
  if (m_hwnd == nullptr) {
    return;
  }

  const DWORD style = Detail::windowStyle(resizable, m_isDecorated);
  SetWindowLongPtrW(m_hwnd, GWL_STYLE, static_cast<LONG_PTR>(style));
  SetWindowPos(m_hwnd,
               nullptr,
               0,
               0,
               0,
               0,
               SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
  m_isResizable = resizable;
}

bool
Win32Window::isResizable() const {
  return m_isResizable;
}

void
Win32Window::setDecorated(const bool decorated) {
  if (m_hwnd == nullptr) {
    return;
  }

  const DWORD style = Detail::windowStyle(m_isResizable, decorated);
  SetWindowLongPtrW(m_hwnd, GWL_STYLE, static_cast<LONG_PTR>(style));
  SetWindowPos(m_hwnd,
               nullptr,
               0,
               0,
               0,
               0,
               SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
  m_isDecorated = decorated;
}

bool
Win32Window::isDecorated() const {
  return m_isDecorated;
}
#pragma endregion

LRESULT CALLBACK
Win32Window::windowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
  Win32Window* self = reinterpret_cast<Win32Window*>(
    GetWindowLongPtrW(hwnd, GWLP_USERDATA));

  if (msg == WM_NCCREATE) {
    auto* cs = reinterpret_cast<CREATESTRUCTW*>(lParam);
    self = reinterpret_cast<Win32Window*>(cs->lpCreateParams);
    SetWindowLongPtrW(hwnd,
                      GWLP_USERDATA,
                      reinterpret_cast<LONG_PTR>(self));
  }

  if (self == nullptr) {
    return DefWindowProcW(hwnd, msg, wParam, lParam);
  }

  switch (msg)
  {
    case WM_CLOSE:
      self->m_shouldClose = true;
      return 0;

    case WM_DESTROY:
      return 0;

    case WM_SETFOCUS:
      self->m_hasFocus = true;
      break;

    case WM_KILLFOCUS:
      self->m_hasFocus = false;
      break;

    default:
      break;
  }

  return DefWindowProcW(hwnd, msg, wParam, lParam);
}

}
