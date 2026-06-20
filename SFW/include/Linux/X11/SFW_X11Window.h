#pragma once

#include "SFW_Window.h"

#include <X11/Xlib.h>
#include <X11/Xutil.h>

namespace SFW
{

namespace Detail
{

constexpr uint32_t kMotifHintsDecorations = 1u << 1;

constexpr long kNetWmStateRemove = 0;
constexpr long kNetWmStateAdd    = 1;

// Motif window-manager hints. Xlib transfers format-32 properties as arrays of
// `long`, so the members must be `long`-sized rather than fixed-width 32-bit.
struct MotifWmHints
{
  unsigned long flags {0};
  unsigned long functions {0};
  unsigned long decorations {0};
  long inputMode {0};
  unsigned long status {0};
};

inline Atom
internAtom(Display* display, const char* atomName);

inline void
applyResizableHint(Display* display,
                   ::Window window,
                   const uint32_t width,
                   const uint32_t height,
                   const bool resizable);

inline void
applyDecoratedHint(Display* display, ::Window window, const bool decorated);

}

class X11Window : public Window
{
 public:

  ~X11Window() = default;

#pragma region Lifecycle
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
  bool
  createInternal(const WindowCreateInfo& createInfo) override;

 private:
  // Pushes m_title to the window via _NET_WM_NAME (UTF-8) and WM_NAME.
  void
  applyTitle();

  // Adds or removes the EWMH maximized state through the window manager.
  void
  sendMaximizedState(const bool maximized);

  Display* m_display {nullptr};
  int32_t m_screen {0};
  ::Window m_window {None};
  ::Window m_rootWindow {None};

  Atom m_wmProtocolsAtom {None};
  Atom m_wmDeleteWindowAtom {None};
  Atom m_netWmStateAtom {None};
  Atom m_netWmStateMaximizedVertAtom {None};
  Atom m_netWmStateMaximizedHorzAtom {None};
  Atom m_netWmNameAtom {None};
  Atom m_utf8StringAtom {None};

  std::string m_title;

  int32_t m_x {0};
  int32_t m_y {0};

  uint32_t m_width {0};
  uint32_t m_height {0};

  uint32_t m_framebufferWidth {0};
  uint32_t m_framebufferHeight {0};

  bool m_isVisible {false};
  bool m_isResizable {true};
  bool m_isDecorated {true};
  bool m_shouldClose {false};
  bool m_hasFocus {false};

  bool m_isMaximized {false};
  bool m_isMinimized {false};
};

}
