#pragma once

#include "SFW_BaseWindow.h"

#include <xcb/xcb.h>
#include <xcb/xproto.h>

namespace SFW
{

class XCBWindow : public BaseWindow
{
 public:

	~XCBWindow() = default;

#pragma region Lifecycle
	bool
  create(const WindowCreateInfo& createInfo) override;

	void
  destroy() override;

	bool
  isCreated() const override;
#pragma endregion

#pragma region Event processing
	void
  pollEvents() override;

	void
  requestClose() override;

	bool
  shouldClose() const override;
#pragma endregion

/*
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

#pragma region Icon
	bool
  setIcon(const IconData& icon) override;

	bool
	setIconFromFile(const std::string_view filePath) override;
#pragma endregion
	*/

	void*
	getNativeHandle() const override;

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

	uint32_t m_width {0};
	uint32_t m_height {0};

	bool m_isVisible {false};
	bool m_isResizable {true};
	bool m_isDecorated {true};
	bool m_shouldClose {false};
	bool m_hasFocus {false};

	bool m_isMaximized {false};
	bool m_isMinimized {false};
};

namespace Detail
{

inline xcb_atom_t
internAtom(xcb_connection_t* connection, const char* atomName) {
	xcb_intern_atom_cookie_t cookie = xcb_intern_atom(connection, 0, std::char_traits<char>::length(atomName), atomName);
	xcb_intern_atom_reply_t* reply = xcb_intern_atom_reply(connection, cookie, nullptr);

	if (reply == nullptr) {
		return XCB_ATOM_NONE;
	}

	const xcb_atom_t atom = reply->atom;
	free(reply);

	return atom;
}

}

inline bool
XCBWindow::create(const WindowCreateInfo& createInfo) {
	if (m_connection != nullptr || m_window != XCB_WINDOW_NONE) {
		destroy();
	}

	int screenIndex = 0;
	m_connection = xcb_connect(nullptr, &screenIndex);

	if (m_connection == nullptr || xcb_connection_has_error(m_connection) != 0) {
		if (m_connection != nullptr) {
			xcb_disconnect(m_connection);
			m_connection = nullptr;
		}

		return false;
	}

	xcb_setup_t const* setup = xcb_get_setup(m_connection);
	xcb_screen_iterator_t screenIterator = xcb_setup_roots_iterator(setup);

	for (int32_t index = 0; index < screenIndex; ++index) {
		xcb_screen_next(&screenIterator);
	}

	m_screen = screenIterator.data;
	if (m_screen == nullptr) {
		xcb_disconnect(m_connection);
		m_connection = nullptr;
		return false;
	}

	m_window = xcb_generate_id(m_connection);
	m_title = createInfo.title;
	m_x = createInfo.x;
	m_y = createInfo.y;
	m_width = createInfo.width;
	m_height = createInfo.height;
	m_isVisible = createInfo.visible;
	m_isResizable = createInfo.resizable;
	m_isDecorated = createInfo.decorated;
	m_shouldClose = false;
	m_hasFocus = false;
	m_isMaximized = false;
	m_isMinimized = false;

	const uint32_t eventMask = XCB_EVENT_MASK_EXPOSURE |
														 XCB_EVENT_MASK_STRUCTURE_NOTIFY |
														 XCB_EVENT_MASK_PROPERTY_CHANGE |
														 XCB_EVENT_MASK_FOCUS_CHANGE;

	const uint32_t valueMask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
	const uint32_t valueList[] = {m_screen->black_pixel, eventMask};

	const xcb_void_cookie_t createCookie = xcb_create_window(m_connection,
																													 XCB_COPY_FROM_PARENT,
																													 m_window,
																													 m_screen->root,
																													 static_cast<int16_t>(m_x),
																													 static_cast<int16_t>(m_y),
																													 static_cast<uint16_t>(m_width),
																													 static_cast<uint16_t>(m_height),
																													 0,
																													 XCB_WINDOW_CLASS_INPUT_OUTPUT,
																													 m_screen->root_visual,
																													 valueMask,
																													 valueList);

	if (xcb_request_check(m_connection, createCookie) != nullptr) {
		xcb_disconnect(m_connection);
		m_connection = nullptr;
		m_screen = nullptr;
		m_window = XCB_WINDOW_NONE;

		return false;
	}

	if (!m_title.empty()) {
		xcb_change_property(m_connection,
												XCB_PROP_MODE_REPLACE,
												m_window,
												XCB_ATOM_WM_NAME,
												XCB_ATOM_STRING,
												8,
												static_cast<uint32_t>(m_title.size()),
												m_title.data());
	}

	m_wmProtocolsAtom = Detail::internAtom(m_connection, "WM_PROTOCOLS");
	m_wmDeleteWindowAtom = Detail::internAtom(m_connection, "WM_DELETE_WINDOW");

	if (m_wmProtocolsAtom != XCB_ATOM_NONE && m_wmDeleteWindowAtom != XCB_ATOM_NONE) {
		xcb_change_property(m_connection,
												XCB_PROP_MODE_REPLACE,
												m_window,
												m_wmProtocolsAtom,
												XCB_ATOM_ATOM,
												32,
												1,
												&m_wmDeleteWindowAtom);
	}

	if (m_isVisible) {
		xcb_map_window(m_connection, m_window);
	}

	xcb_flush(m_connection);
	return xcb_connection_has_error(m_connection) == 0;
}

inline void
XCBWindow::destroy() {
	if (m_connection != nullptr && m_window != XCB_WINDOW_NONE) {
		xcb_destroy_window(m_connection, m_window);
		xcb_flush(m_connection);
		m_window = XCB_WINDOW_NONE;
	}

	if (m_connection != nullptr) {
		xcb_disconnect(m_connection);
		m_connection = nullptr;
	}

	m_screen = nullptr;
	m_wmProtocolsAtom = XCB_ATOM_NONE;
	m_wmDeleteWindowAtom = XCB_ATOM_NONE;
	m_title.clear();
	m_x = 0;
	m_y = 0;
	m_width = 0;
	m_height = 0;
	m_isVisible = false;
	m_isResizable = true;
	m_isDecorated = true;
	m_shouldClose = false;
	m_hasFocus = false;
	m_isMaximized = false;
	m_isMinimized = false;
}

bool
XCBWindow::isCreated() const {
	return m_connection != nullptr && m_window != XCB_WINDOW_NONE;
}

void
XCBWindow::pollEvents() {
	if (m_connection == nullptr) {
		return;
	}
	
	xcb_generic_event_t* event;
	while ((event = xcb_poll_for_event(m_connection)) != nullptr) {
		switch (event->response_type & ~0x80)
		{
			case XCB_EXPOSE:
				break;

			case XCB_CLIENT_MESSAGE:
			{
				xcb_client_message_event_t* clientMessageEvent = reinterpret_cast<xcb_client_message_event_t*>(event);
				if (clientMessageEvent->window == m_window && clientMessageEvent->type == m_wmProtocolsAtom) {
					if (clientMessageEvent->data.data32[0] == m_wmDeleteWindowAtom) {
						m_shouldClose = true;
					}
				}
				break;
			}

			case XCB_DESTROY_NOTIFY:
				m_shouldClose = true;
				break;

			default:
				break;
		}

		free(event);
	}
}

void
XCBWindow::requestClose() {
	m_shouldClose = true;
}

bool
XCBWindow::shouldClose() const {
	return m_shouldClose;
}

void*
XCBWindow::getNativeHandle() const {
	return reinterpret_cast<void*>(m_window);
}

};