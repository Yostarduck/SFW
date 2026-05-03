#pragma once

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#include "SFW_Window.h"

namespace SFW
{

namespace Detail
{

constexpr wchar_t kWindowClassName[] = L"SFW_Win32Window";

inline DWORD
windowStyle(const bool resizable, const bool decorated);

inline std::wstring
toWideString(const std::string_view str);

}

class Win32Window : public Window
{
 public:

  ~Win32Window() = default;

#pragma region Lifecycle
  bool
  createInternal(const WindowCreateInfo& createInfo) override;

  void
  destroy() override;

  bool
  isCreated() const override;

  void*
  getNativeHandle() const override;
#pragma endregion

#pragma region Event processing
  void
  pollEvents() override;

  void
  requestClose() override;

  bool
  shouldClose() const override;
#pragma endregion

#pragma region Visibility and focus
  void
  show() override;

  void
  hide() override;

  bool
  isVisible() const override;

  void
  focus() override;

  bool
  hasFocus() const override;
#pragma endregion

#pragma region Position and size
  void
  setPosition(const int32_t x, const int32_t y) override;

  void
  getPosition(int32_t& x, int32_t& y) const override;

  void
  setSize(const uint32_t width, const uint32_t height) override;

  void
  getSize(uint32_t& width, uint32_t& height) const override;

  void
  getFramebufferSize(uint32_t& width, uint32_t& height) const override;
#pragma endregion

#pragma region Window state
  void
  maximize() override;

  void
  minimize() override;

  void
  restore() override;

  bool
  isMaximized() const override;

  bool
  isMinimized() const override;
#pragma endregion

#pragma region Window attributes
  void
  setTitle(const std::string_view title) override;

  std::string_view
  getTitle() const override;

  void
  setResizable(const bool resizable) override;

  bool
  isResizable() const override;

  void
  setDecorated(const bool decorated) override;

  bool
  isDecorated() const override;
#pragma endregion

 protected:

 private:
  static LRESULT CALLBACK
  windowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

  HWND      m_hwnd      {nullptr};
  HINSTANCE m_hInstance {nullptr};

  std::string m_title;

  int32_t m_x {0};
  int32_t m_y {0};

  uint32_t m_width  {0};
  uint32_t m_height {0};

  bool m_isVisible   {false};
  bool m_isResizable {true};
  bool m_isDecorated {true};
  bool m_shouldClose {false};
  bool m_hasFocus    {false};
  bool m_isMaximized {false};
  bool m_isMinimized {false};
};

}
