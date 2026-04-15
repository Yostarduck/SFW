#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace SFW
{

struct WindowCreateInfo
{
  std::string title;

  int32_t x {0};
  int32_t y {0};

  uint32_t width {0};
  uint32_t height {0};

  bool visible {true};
  bool resizable {true};
  bool decorated {true};
};

struct IconData
{
  uint32_t width {0};
  uint32_t height {0};

  std::vector<uint8_t> rgba8;
};

class Window
{
 public:
	virtual ~Window() = default;

#pragma region Lifecycle
	static Window*
	create(const WindowCreateInfo& createInfo);

	virtual bool
  createInternal(const WindowCreateInfo& createInfo) = 0;

	virtual void
  destroy() = 0;

	virtual bool
  isCreated() const = 0;

  virtual void*
	getNativeHandle() const = 0;
#pragma endregion

#pragma region Event processing
	virtual void
  pollEvents() = 0;

	virtual void
  requestClose() = 0;

	virtual bool
  shouldClose() const = 0;
#pragma endregion

#pragma region Visibility and focus
	virtual void
  show() = 0;

	virtual void
  hide() = 0;

	virtual bool
  isVisible() const = 0;

	virtual void
  focus() = 0;

	virtual bool
  hasFocus() const = 0;
#pragma endregion

#pragma region Position and size
	virtual void
  setPosition(const int32_t x, const int32_t y) = 0;
  
	virtual void
  getPosition(int32_t& x, int32_t& y) const = 0;
  
	virtual void
  setSize(const uint32_t width, const uint32_t height) = 0;
  
	virtual void
  getSize(uint32_t& width, uint32_t& height) const = 0;

	virtual void
  getFramebufferSize(uint32_t& width, uint32_t& height) const = 0;
#pragma endregion

#pragma region Window state
	virtual void
  maximize() = 0;

	virtual void
  minimize() = 0;

	virtual void
  restore() = 0;

	virtual bool
  isMaximized() const = 0;

	virtual bool
  isMinimized() const = 0;
#pragma endregion

#pragma region Window attributes
	virtual void
	setTitle(const std::string_view title) = 0;

	virtual std::string_view
  getTitle() const = 0;

	virtual void
	setResizable(const bool resizable) = 0;

	virtual bool
  isResizable() const = 0;

	virtual void
	setDecorated(const bool decorated) = 0;

	virtual bool
  isDecorated() const = 0;
  
  /*
  virtual bool
  setIcon(const IconData& icon) = 0;
  */
#pragma endregion

 protected:
  Window() = default;

 private:

};

}