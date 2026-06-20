#include "Linux/Wayland/SFW_WaylandWindow.h"

#include <cstdint>
#include <cstring>

#include <poll.h>
#include <sys/mman.h>
#include <unistd.h>

namespace SFW
{

#pragma region Listeners
const wl_registry_listener WaylandWindow::kRegistryListener = {
  &WaylandWindow::handleRegistryGlobal,
  &WaylandWindow::handleRegistryGlobalRemove,
};

const xdg_wm_base_listener WaylandWindow::kXdgWmBaseListener = {
  &WaylandWindow::handleXdgWmBasePing,
};

const xdg_surface_listener WaylandWindow::kXdgSurfaceListener = {
  &WaylandWindow::handleXdgSurfaceConfigure,
};

const xdg_toplevel_listener WaylandWindow::kXdgToplevelListener = {
  &WaylandWindow::handleToplevelConfigure,
  &WaylandWindow::handleToplevelClose,
};

void
WaylandWindow::handleRegistryGlobal(void* data,
                                    wl_registry* registry,
                                    uint32_t name,
                                    const char* interface,
                                    uint32_t version)
{
  WaylandWindow* self = static_cast<WaylandWindow*>(data);

  if (std::strcmp(interface, wl_compositor_interface.name) == 0) {
    self->m_compositor = static_cast<wl_compositor*>(
      wl_registry_bind(registry, name, &wl_compositor_interface, 1));
  } else if (std::strcmp(interface, wl_shm_interface.name) == 0) {
    self->m_shm = static_cast<wl_shm*>(
      wl_registry_bind(registry, name, &wl_shm_interface, 1));
  } else if (std::strcmp(interface, xdg_wm_base_interface.name) == 0) {
    self->m_xdgWmBase = static_cast<xdg_wm_base*>(
      wl_registry_bind(registry, name, &xdg_wm_base_interface, 1));
    xdg_wm_base_add_listener(self->m_xdgWmBase, &kXdgWmBaseListener, self);
  } else if (std::strcmp(interface,
                         zxdg_decoration_manager_v1_interface.name) == 0) {
    self->m_decorationManager = static_cast<zxdg_decoration_manager_v1*>(
      wl_registry_bind(registry,
                       name,
                       &zxdg_decoration_manager_v1_interface,
                       1));
  }
}

void
WaylandWindow::handleRegistryGlobalRemove(void* data,
                                          wl_registry* registry,
                                          uint32_t name)
{
  (void)data;
  (void)registry;
  (void)name;
}

void
WaylandWindow::handleXdgWmBasePing(void* data,
                                   xdg_wm_base* xdgWmBase,
                                   uint32_t serial)
{
  (void)data;
  xdg_wm_base_pong(xdgWmBase, serial);
}

void
WaylandWindow::handleXdgSurfaceConfigure(void* data,
                                         xdg_surface* xdgSurface,
                                         uint32_t serial)
{
  WaylandWindow* self = static_cast<WaylandWindow*>(data);

  xdg_surface_ack_configure(xdgSurface, serial);

  self->m_surfaceConfigured = true;
  self->commitBuffer();
}

void
WaylandWindow::handleToplevelConfigure(void* data,
                                       xdg_toplevel* xdgToplevel,
                                       int32_t width,
                                       int32_t height,
                                       wl_array* states)
{
  (void)xdgToplevel;
  WaylandWindow* self = static_cast<WaylandWindow*>(data);

  bool maximized = false;
  bool activated = false;

  const uint32_t* stateData = static_cast<const uint32_t*>(states->data);
  const size_t stateCount = states->size / sizeof(uint32_t);

  for (size_t index = 0; index < stateCount; ++index) {
    if (stateData[index] == XDG_TOPLEVEL_STATE_MAXIMIZED) {
      maximized = true;
    } else if (stateData[index] == XDG_TOPLEVEL_STATE_ACTIVATED) {
      activated = true;
    }
  }

  // A zero size means the compositor leaves the dimensions up to the client.
  if (width > 0 && height > 0) {
    self->m_width = static_cast<uint32_t>(width);
    self->m_height = static_cast<uint32_t>(height);
    self->m_framebufferWidth = static_cast<uint32_t>(width);
    self->m_framebufferHeight = static_cast<uint32_t>(height);
  }

  self->m_isMaximized = maximized;
  self->m_hasFocus = activated;
}

void
WaylandWindow::handleToplevelClose(void* data, xdg_toplevel* xdgToplevel)
{
  (void)xdgToplevel;
  WaylandWindow* self = static_cast<WaylandWindow*>(data);
  self->m_shouldClose = true;
}
#pragma endregion

#pragma region Buffer helpers
bool
WaylandWindow::refreshBuffer()
{
  if (m_shm == nullptr) {
    return false;
  }

  const uint32_t width = m_framebufferWidth != 0 ? m_framebufferWidth : m_width;
  const uint32_t height =
    m_framebufferHeight != 0 ? m_framebufferHeight : m_height;

  if (width == 0 || height == 0) {
    return false;
  }

  const int32_t stride = static_cast<int32_t>(width * 4);
  const size_t size = static_cast<size_t>(stride) * height;

  releaseBuffer();

  const int32_t fd = static_cast<int32_t>(memfd_create("sfw-shm", MFD_CLOEXEC));
  if (fd < 0) {
    return false;
  }

  if (ftruncate(fd, static_cast<off_t>(size)) < 0) {
    close(fd);
    return false;
  }

  void* data = mmap(nullptr,
                    size,
                    PROT_READ | PROT_WRITE,
                    MAP_SHARED,
                    fd,
                    0);
  if (data == MAP_FAILED) {
    close(fd);
    return false;
  }

  // Fill with opaque black, mirroring the XCB backend's black background.
  uint32_t* pixels = static_cast<uint32_t*>(data);
  const size_t pixelCount = size / sizeof(uint32_t);
  for (size_t index = 0; index < pixelCount; ++index) {
    pixels[index] = 0xFF000000u;
  }

  wl_shm_pool* pool =
    wl_shm_create_pool(m_shm, fd, static_cast<int32_t>(size));
  m_buffer = wl_shm_pool_create_buffer(pool,
                                       0,
                                       static_cast<int32_t>(width),
                                       static_cast<int32_t>(height),
                                       stride,
                                       WL_SHM_FORMAT_XRGB8888);
  wl_shm_pool_destroy(pool);

  if (m_buffer == nullptr) {
    munmap(data, size);
    close(fd);
    return false;
  }

  m_shmFd = fd;
  m_shmData = data;
  m_shmSize = size;

  return true;
}

void
WaylandWindow::commitBuffer()
{
  if (m_surface == nullptr) {
    return;
  }

  if (m_isVisible) {
    if (!refreshBuffer()) {
      return;
    }

    wl_surface_attach(m_surface, m_buffer, 0, 0);
    wl_surface_damage(m_surface,
                      0,
                      0,
                      static_cast<int32_t>(m_framebufferWidth),
                      static_cast<int32_t>(m_framebufferHeight));
  } else {
    wl_surface_attach(m_surface, nullptr, 0, 0);
  }

  wl_surface_commit(m_surface);
}

void
WaylandWindow::releaseBuffer()
{
  if (m_buffer != nullptr) {
    wl_buffer_destroy(m_buffer);
    m_buffer = nullptr;
  }

  if (m_shmData != nullptr) {
    munmap(m_shmData, m_shmSize);
    m_shmData = nullptr;
  }

  if (m_shmFd >= 0) {
    close(m_shmFd);
    m_shmFd = -1;
  }

  m_shmSize = 0;
}
#pragma endregion

#pragma region Lifecycle
bool
WaylandWindow::createInternal(const WindowCreateInfo& createInfo)
{
  if (m_display != nullptr) {
    destroy();
  }

  m_display = wl_display_connect(nullptr);
  if (m_display == nullptr) {
    return false;
  }

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
  m_surfaceConfigured = false;

  m_registry = wl_display_get_registry(m_display);
  if (m_registry == nullptr) {
    destroy();
    return false;
  }

  wl_registry_add_listener(m_registry, &kRegistryListener, this);

  // First round-trip binds the globals advertised by the compositor.
  wl_display_roundtrip(m_display);

  if (m_compositor == nullptr ||
      m_shm == nullptr ||
      m_xdgWmBase == nullptr) {
    destroy();
    return false;
  }

  m_surface = wl_compositor_create_surface(m_compositor);
  if (m_surface == nullptr) {
    destroy();
    return false;
  }

  m_xdgSurface = xdg_wm_base_get_xdg_surface(m_xdgWmBase, m_surface);
  if (m_xdgSurface == nullptr) {
    destroy();
    return false;
  }

  xdg_surface_add_listener(m_xdgSurface, &kXdgSurfaceListener, this);

  m_xdgToplevel = xdg_surface_get_toplevel(m_xdgSurface);
  if (m_xdgToplevel == nullptr) {
    destroy();
    return false;
  }

  xdg_toplevel_add_listener(m_xdgToplevel, &kXdgToplevelListener, this);
  xdg_toplevel_set_title(m_xdgToplevel, m_title.c_str());

  if (!m_isResizable) {
    xdg_toplevel_set_min_size(m_xdgToplevel,
                              static_cast<int32_t>(m_width),
                              static_cast<int32_t>(m_height));
    xdg_toplevel_set_max_size(m_xdgToplevel,
                              static_cast<int32_t>(m_width),
                              static_cast<int32_t>(m_height));
  }

  if (m_decorationManager != nullptr) {
    m_toplevelDecoration =
      zxdg_decoration_manager_v1_get_toplevel_decoration(m_decorationManager,
                                                         m_xdgToplevel);
    zxdg_toplevel_decoration_v1_set_mode(
      m_toplevelDecoration,
      m_isDecorated ? ZXDG_TOPLEVEL_DECORATION_V1_MODE_SERVER_SIDE
                    : ZXDG_TOPLEVEL_DECORATION_V1_MODE_CLIENT_SIDE);
  }

  // An empty commit triggers the initial configure; the buffer is attached
  // from the xdg_surface configure handler during the round-trip below.
  wl_surface_commit(m_surface);
  wl_display_roundtrip(m_display);

  return wl_display_get_error(m_display) == 0;
}

void
WaylandWindow::destroy()
{
  releaseBuffer();

  if (m_toplevelDecoration != nullptr) {
    zxdg_toplevel_decoration_v1_destroy(m_toplevelDecoration);
    m_toplevelDecoration = nullptr;
  }

  if (m_xdgToplevel != nullptr) {
    xdg_toplevel_destroy(m_xdgToplevel);
    m_xdgToplevel = nullptr;
  }

  if (m_xdgSurface != nullptr) {
    xdg_surface_destroy(m_xdgSurface);
    m_xdgSurface = nullptr;
  }

  if (m_surface != nullptr) {
    wl_surface_destroy(m_surface);
    m_surface = nullptr;
  }

  if (m_decorationManager != nullptr) {
    zxdg_decoration_manager_v1_destroy(m_decorationManager);
    m_decorationManager = nullptr;
  }

  if (m_xdgWmBase != nullptr) {
    xdg_wm_base_destroy(m_xdgWmBase);
    m_xdgWmBase = nullptr;
  }

  if (m_shm != nullptr) {
    wl_shm_destroy(m_shm);
    m_shm = nullptr;
  }

  if (m_compositor != nullptr) {
    wl_compositor_destroy(m_compositor);
    m_compositor = nullptr;
  }

  if (m_registry != nullptr) {
    wl_registry_destroy(m_registry);
    m_registry = nullptr;
  }

  if (m_display != nullptr) {
    wl_display_flush(m_display);
    wl_display_disconnect(m_display);
    m_display = nullptr;
  }

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
  m_surfaceConfigured = false;
}

bool
WaylandWindow::isCreated() const
{
  return m_display != nullptr && m_surface != nullptr;
}

void*
WaylandWindow::getNativeHandle() const
{
  return reinterpret_cast<void*>(m_surface);
}
#pragma endregion

#pragma region Event processing
void
WaylandWindow::pollEvents()
{
  if (m_display == nullptr) {
    return;
  }

  wl_display_flush(m_display);

  while (wl_display_prepare_read(m_display) != 0) {
    if (wl_display_dispatch_pending(m_display) < 0) {
      m_shouldClose = true;
      return;
    }
  }

  pollfd pollDescriptor {};
  pollDescriptor.fd = wl_display_get_fd(m_display);
  pollDescriptor.events = POLLIN;

  if (poll(&pollDescriptor, 1, 0) > 0 &&
      (pollDescriptor.revents & POLLIN) != 0) {
    if (wl_display_read_events(m_display) < 0) {
      m_shouldClose = true;
      return;
    }
  } else {
    wl_display_cancel_read(m_display);
  }

  if (wl_display_dispatch_pending(m_display) < 0) {
    m_shouldClose = true;
  }
}

void
WaylandWindow::requestClose()
{
  m_shouldClose = true;
}

bool
WaylandWindow::shouldClose() const
{
  return m_shouldClose;
}
#pragma endregion

#pragma region Visibility and focus
void
WaylandWindow::show()
{
  if (m_surface != nullptr && m_surfaceConfigured && !m_isVisible) {
    m_isVisible = true;
    commitBuffer();
    wl_display_flush(m_display);
  }
}

void
WaylandWindow::hide()
{
  if (m_surface != nullptr && m_isVisible) {
    m_isVisible = false;
    commitBuffer();
    wl_display_flush(m_display);
  }
}

bool
WaylandWindow::isVisible() const
{
  return m_isVisible;
}

void
WaylandWindow::focus()
{
  // Wayland leaves focus entirely to the compositor; a client cannot raise
  // or activate itself, so there is no action to take here.
}

bool
WaylandWindow::hasFocus() const
{
  return m_hasFocus;
}
#pragma endregion

#pragma region Position and size
void
WaylandWindow::setPosition(const int32_t x, const int32_t y)
{
  // Wayland does not let clients position their own surfaces; placement is a
  // compositor decision. The requested values are only mirrored locally.
  m_x = x;
  m_y = y;
}

void
WaylandWindow::getPosition(int32_t& x, int32_t& y) const
{
  x = m_x;
  y = m_y;
}

void
WaylandWindow::setSize(const uint32_t width, const uint32_t height)
{
  if (m_surface == nullptr || width == 0 || height == 0) {
    return;
  }

  m_width = width;
  m_height = height;
  m_framebufferWidth = width;
  m_framebufferHeight = height;

  if (!m_isResizable && m_xdgToplevel != nullptr) {
    xdg_toplevel_set_min_size(m_xdgToplevel,
                              static_cast<int32_t>(width),
                              static_cast<int32_t>(height));
    xdg_toplevel_set_max_size(m_xdgToplevel,
                              static_cast<int32_t>(width),
                              static_cast<int32_t>(height));
  }

  commitBuffer();
  wl_display_flush(m_display);
}

void
WaylandWindow::getSize(uint32_t& width, uint32_t& height) const
{
  width = m_width;
  height = m_height;
}

void
WaylandWindow::getFramebufferSize(uint32_t& width, uint32_t& height) const
{
  width = m_framebufferWidth;
  height = m_framebufferHeight;
}
#pragma endregion

#pragma region Window state
void
WaylandWindow::maximize()
{
  if (m_xdgToplevel != nullptr && !m_isMaximized) {
    xdg_toplevel_set_maximized(m_xdgToplevel);
    wl_display_flush(m_display);

    m_isMaximized = true;
    m_isMinimized = false;
  }
}

void
WaylandWindow::minimize()
{
  if (m_xdgToplevel != nullptr && !m_isMinimized) {
    xdg_toplevel_set_minimized(m_xdgToplevel);
    wl_display_flush(m_display);

    m_isMaximized = false;
    m_isMinimized = true;
  }
}

void
WaylandWindow::restore()
{
  if (m_xdgToplevel != nullptr && (m_isMaximized || m_isMinimized)) {
    xdg_toplevel_unset_maximized(m_xdgToplevel);
    wl_display_flush(m_display);

    m_isMaximized = false;
    m_isMinimized = false;
  }
}

bool
WaylandWindow::isMaximized() const
{
  return m_isMaximized;
}

bool
WaylandWindow::isMinimized() const
{
  return m_isMinimized;
}
#pragma endregion

#pragma region Window attributes
void
WaylandWindow::setTitle(const std::string_view title)
{
  if (m_xdgToplevel == nullptr) {
    return;
  }

  m_title = title;
  xdg_toplevel_set_title(m_xdgToplevel, m_title.c_str());
  wl_display_flush(m_display);
}

std::string_view
WaylandWindow::getTitle() const
{
  return m_title;
}

void
WaylandWindow::setResizable(const bool resizable)
{
  if (m_xdgToplevel == nullptr) {
    return;
  }

  if (resizable) {
    xdg_toplevel_set_min_size(m_xdgToplevel, 0, 0);
    xdg_toplevel_set_max_size(m_xdgToplevel, 0, 0);
  } else {
    xdg_toplevel_set_min_size(m_xdgToplevel,
                              static_cast<int32_t>(m_width),
                              static_cast<int32_t>(m_height));
    xdg_toplevel_set_max_size(m_xdgToplevel,
                              static_cast<int32_t>(m_width),
                              static_cast<int32_t>(m_height));
  }

  wl_display_flush(m_display);
  m_isResizable = resizable;
}

bool
WaylandWindow::isResizable() const
{
  return m_isResizable;
}

void
WaylandWindow::setDecorated(const bool decorated)
{
  if (m_toplevelDecoration != nullptr) {
    zxdg_toplevel_decoration_v1_set_mode(
      m_toplevelDecoration,
      decorated ? ZXDG_TOPLEVEL_DECORATION_V1_MODE_SERVER_SIDE
                : ZXDG_TOPLEVEL_DECORATION_V1_MODE_CLIENT_SIDE);
    wl_display_flush(m_display);
  }

  m_isDecorated = decorated;
}

bool
WaylandWindow::isDecorated() const
{
  return m_isDecorated;
}
#pragma endregion

}
