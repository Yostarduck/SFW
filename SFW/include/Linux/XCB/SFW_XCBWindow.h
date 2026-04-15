#pragma once

#include "SFW_Window.h"

#include <xcb/xcb.h>
#include <xcb/xproto.h>

namespace SFW
{

namespace Detail
{

constexpr uint32_t NormalHintsMinSize			= 1u << 4;
constexpr uint32_t NormalHintsMaxSize			= 1u << 5;
constexpr uint32_t MotifHintsDecorations	= 1u << 1;

struct NormalSizeHints
{
	uint32_t	flags					{0};
	int32_t		x							{0};
	int32_t		y							{0};
	int32_t		width					{0};
	int32_t		height				{0};
	int32_t		minWidth			{0};
	int32_t		minHeight			{0};
	int32_t		maxWidth			{0};
	int32_t		maxHeight			{0};
	int32_t		widthInc			{0};
	int32_t		heightInc			{0};
	int32_t		minAspectNum	{0};
	int32_t		minAspectDen	{0};
	int32_t		maxAspectNum	{0};
	int32_t		maxAspectDen	{0};
	int32_t		baseWidth			{0};
	int32_t		baseHeight		{0};
	uint32_t	winGravity		{0};
};

struct MotifWmHints
{
	uint32_t	flags				{0};
	uint32_t	functions		{0};
	uint32_t	decorations	{0};
	int32_t		inputMode		{0};
	uint32_t	status			{0};
};

inline void
applyResizableHint(xcb_connection_t* connection,
									 xcb_window_t window,
									 const uint32_t width,
									 const uint32_t height,
									 const bool resizable);

inline void
applyDecoratedHint(xcb_connection_t* connection,
									 xcb_window_t window,
									 const bool decorated);

inline xcb_atom_t
internAtom(xcb_connection_t* connection, const char* atomName);

}

class XCBWindow : public Window
{
 public:

	~XCBWindow() = default;

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

/*
#pragma region Icon
	bool
  setIcon(const IconData& icon) override;
#pragma endregion
	*/

 protected:

 private:
	xcb_connection_t* m_connection {nullptr};
	xcb_screen_t* m_screen {nullptr};
	xcb_window_t m_window {XCB_WINDOW_NONE};
	xcb_atom_t m_wmProtocolsAtom {XCB_ATOM_NONE};
	xcb_atom_t m_wmDeleteWindowAtom {XCB_ATOM_NONE};

	std::string m_title;

	int32_t m_x {0};
	int32_t m_y {0};

	uint32_t m_width	{0};
	uint32_t m_height	{0};

	bool m_isVisible		{false};
	bool m_isResizable	{true};
	bool m_isDecorated	{true};
	bool m_shouldClose	{false};
	bool m_hasFocus			{false};

	bool m_isMaximized	{false};
	bool m_isMinimized	{false};
};

};