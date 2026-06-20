#pragma once

#include "SFW_Window.h"

#include <wayland-client.h>

#include "xdg-shell-client-protocol.h"
#include "xdg-decoration-client-protocol.h"

namespace SFW
{

class WaylandWindow : public Window
{
 public:

  ~WaylandWindow() = default;

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
  // Wayland clients only become visible once a buffer is attached and
  // committed; this (re)creates a shared-memory buffer at the current size.
  bool
  refreshBuffer();

  // Attaches the current buffer (or a null buffer when hidden) and commits.
  void
  commitBuffer();

  // Frees the shared-memory buffer backing the surface.
  void
  releaseBuffer();

  static void
  handleRegistryGlobal(void* data,
                       wl_registry* registry,
                       uint32_t name,
                       const char* interface,
                       uint32_t version);

  static void
  handleRegistryGlobalRemove(void* data,
                             wl_registry* registry,
                             uint32_t name);

  static void
  handleXdgWmBasePing(void* data, xdg_wm_base* xdgWmBase, uint32_t serial);

  static void
  handleXdgSurfaceConfigure(void* data,
                            xdg_surface* xdgSurface,
                            uint32_t serial);

  static void
  handleToplevelConfigure(void* data,
                          xdg_toplevel* xdgToplevel,
                          int32_t width,
                          int32_t height,
                          wl_array* states);

  static void
  handleToplevelClose(void* data, xdg_toplevel* xdgToplevel);

  static const wl_registry_listener kRegistryListener;
  static const xdg_wm_base_listener kXdgWmBaseListener;
  static const xdg_surface_listener kXdgSurfaceListener;
  static const xdg_toplevel_listener kXdgToplevelListener;

  wl_display* m_display {nullptr};
  wl_registry* m_registry {nullptr};
  wl_compositor* m_compositor {nullptr};
  wl_shm* m_shm {nullptr};
  xdg_wm_base* m_xdgWmBase {nullptr};
  zxdg_decoration_manager_v1* m_decorationManager {nullptr};

  wl_surface* m_surface {nullptr};
  xdg_surface* m_xdgSurface {nullptr};
  xdg_toplevel* m_xdgToplevel {nullptr};
  zxdg_toplevel_decoration_v1* m_toplevelDecoration {nullptr};

  wl_buffer* m_buffer {nullptr};
  void* m_shmData {nullptr};
  size_t m_shmSize {0};
  int32_t m_shmFd {-1};

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

  bool m_surfaceConfigured {false};
};

}
