#include "Linux/XCB/SFW_XCBWindow.h"

namespace SFW
{

namespace Detail
{

inline void
applyResizableHint(xcb_connection_t* connection,
									 xcb_window_t window,
									 const uint32_t width,
									 const uint32_t height,
									 const bool resizable) {
	if (connection == nullptr || window == XCB_WINDOW_NONE) {
		return;
	}

	const xcb_atom_t wmNormalHintsAtom = internAtom(connection, "WM_NORMAL_HINTS");
	const xcb_atom_t wmSizeHintsAtom = internAtom(connection, "WM_SIZE_HINTS");

	if (wmNormalHintsAtom == XCB_ATOM_NONE ||
			wmSizeHintsAtom == XCB_ATOM_NONE) {
		return;
	}

	NormalSizeHints hints {};
	hints.width = static_cast<int32_t>(width);
	hints.height = static_cast<int32_t>(height);

	if (!resizable) {
		hints.flags = NormalHintsMinSize | NormalHintsMaxSize;
		hints.minWidth = static_cast<int32_t>(width);
		hints.minHeight = static_cast<int32_t>(height);
		hints.maxWidth = static_cast<int32_t>(width);
		hints.maxHeight = static_cast<int32_t>(height);
	}

	xcb_change_property(connection,
											XCB_PROP_MODE_REPLACE,
											window,
											wmNormalHintsAtom,
											wmSizeHintsAtom,
											32,
											static_cast<uint32_t>(sizeof(NormalSizeHints) / sizeof(uint32_t)),
											&hints);
}

inline void
applyDecoratedHint(xcb_connection_t* connection,
									 xcb_window_t window,
									 const bool decorated) {
	if (connection == nullptr || window == XCB_WINDOW_NONE) {
		return;
	}

	const xcb_atom_t motifHintsAtom = internAtom(connection, "_MOTIF_WM_HINTS");
	if (motifHintsAtom == XCB_ATOM_NONE) {
		return;
	}

	MotifWmHints hints {};
	hints.flags = MotifHintsDecorations;
	hints.decorations = decorated ? 1u : 0u;

	xcb_change_property(connection,
											XCB_PROP_MODE_REPLACE,
											window,
											motifHintsAtom,
											motifHintsAtom,
											32,
											static_cast<uint32_t>(sizeof(MotifWmHints) / sizeof(uint32_t)),
											&hints);
}

inline xcb_atom_t
internAtom(xcb_connection_t* connection, const char* atomName) {
	xcb_intern_atom_cookie_t cookie = xcb_intern_atom(connection,
																										0,
																										std::char_traits<char>::length(atomName),
																										atomName);
	
	xcb_intern_atom_reply_t* reply = xcb_intern_atom_reply(connection,
																												 cookie,
																												 nullptr);

	if (reply == nullptr) {
		return XCB_ATOM_NONE;
	}

	const xcb_atom_t atom = reply->atom;
	free(reply);

	return atom;
}

}

#pragma region Lifecycle
inline bool
XCBWindow::createInternal(const WindowCreateInfo& createInfo) {
	if (m_connection != nullptr || m_window != XCB_WINDOW_NONE) {
		destroy();
	}

	int32_t screenIndex = 0;
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

	Detail::applyResizableHint(m_connection,
														 m_window,
														 m_width,
														 m_height,
														 m_isResizable);
	Detail::applyDecoratedHint(m_connection, m_window, m_isDecorated);

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

void*
XCBWindow::getNativeHandle() const {
	return reinterpret_cast<void*>(m_window);
}
#pragma endregion

#pragma region Event processing
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
				if (clientMessageEvent->window == m_window &&
						clientMessageEvent->type == m_wmProtocolsAtom) {
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
#pragma endregion

#pragma region Visibility and focus
void
XCBWindow::show() {
	if (m_connection != nullptr &&
			m_window != XCB_WINDOW_NONE && !m_isVisible) {
		xcb_map_window(m_connection, m_window);
		xcb_flush(m_connection);
		m_isVisible = true;
	}
}

void
XCBWindow::hide() {
	if (m_connection != nullptr &&
			m_window != XCB_WINDOW_NONE &&
			m_isVisible) {
		xcb_unmap_window(m_connection, m_window);
		xcb_flush(m_connection);
		m_isVisible = false;
	}
}

bool
XCBWindow::isVisible() const {
	return m_isVisible;
}

void
XCBWindow::focus() {
	if (m_connection != nullptr &&
			m_window != XCB_WINDOW_NONE) {
		xcb_set_input_focus(m_connection,
												XCB_INPUT_FOCUS_POINTER_ROOT,
												m_window,
												XCB_CURRENT_TIME);
		xcb_flush(m_connection);
		m_hasFocus = true;
	}
}

bool
XCBWindow::hasFocus() const {
	return m_hasFocus;
}
#pragma endregion

#pragma region Position and size
void
XCBWindow::setPosition(const int32_t x, const int32_t y) {
	if (m_connection != nullptr &&
			m_window != XCB_WINDOW_NONE) {
		uint32_t position[2] = {
			static_cast<uint32_t>(x),
			static_cast<uint32_t>(y)
		};

		xcb_configure_window(m_connection,
												 m_window,
												 XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y,
												 position);
		xcb_flush(m_connection);

		m_x = x;
		m_y = y;
	}
}

void
XCBWindow::getPosition(int32_t& x, int32_t& y) const {
	x = m_x;
	y = m_y;
}

void
XCBWindow::setSize(const uint32_t width, const uint32_t height) {
	if (m_connection != nullptr &&
			m_window != XCB_WINDOW_NONE) {
		uint32_t size[2] = {
			width,
			height
		};

		xcb_configure_window(m_connection,
												 m_window,
												 XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT,
												 size);
		xcb_flush(m_connection);

		m_width = width;
		m_height = height;
	}
}

void
XCBWindow::getSize(uint32_t& width, uint32_t& height) const {
	width = m_width;
	height = m_height;
}

void
XCBWindow::getFramebufferSize(uint32_t& width, uint32_t& height) const {
	width = m_width;
	height = m_height;
}
#pragma endregion

#pragma region Window state
void
XCBWindow::maximize() {
	if (m_connection != nullptr &&
			m_window != XCB_WINDOW_NONE &&
			!m_isMaximized) {
		const uint32_t stateMaximized = 1;

		xcb_client_message_event_t event{};
		event.response_type = XCB_CLIENT_MESSAGE;
		event.window = m_window;
		event.type = m_wmProtocolsAtom;
		event.format = 32;
		event.data.data32[0] = stateMaximized;
		event.data.data32[1] = 0;
		event.data.data32[2] = 0;

		xcb_send_event(m_connection,
									 false,
									 m_window,
									 XCB_EVENT_MASK_STRUCTURE_NOTIFY,
									 reinterpret_cast<const char*>(&event));
		xcb_flush(m_connection);

		m_isMaximized = true;
		m_isMinimized = false;
	}
}

void
XCBWindow::minimize() {
	if (m_connection != nullptr &&
			m_window != XCB_WINDOW_NONE &&
			!m_isMinimized) {
		const uint32_t stateMinimized = 1;

		xcb_client_message_event_t event{};
		event.response_type = XCB_CLIENT_MESSAGE;
		event.window = m_window;
		event.type = m_wmProtocolsAtom;
		event.format = 32;
		event.data.data32[0] = stateMinimized;
		event.data.data32[1] = 0;
		event.data.data32[2] = 0;

		xcb_send_event(m_connection,
									 false,
									 m_window,
									 XCB_EVENT_MASK_STRUCTURE_NOTIFY,
									 reinterpret_cast<const char*>(&event));
		xcb_flush(m_connection);

		m_isMaximized = false;
		m_isMinimized = true;
	}
}

void
XCBWindow::restore() {
	if (m_connection != nullptr &&
			m_window != XCB_WINDOW_NONE &&
			(m_isMaximized || m_isMinimized)) {
		const uint32_t stateRestored = 0;

		xcb_client_message_event_t event{};
		event.response_type = XCB_CLIENT_MESSAGE;
		event.window = m_window;
		event.type = m_wmProtocolsAtom;
		event.format = 32;
		event.data.data32[0] = stateRestored;
		event.data.data32[1] = 0;
		event.data.data32[2] = 0;

		xcb_send_event(m_connection,
									 false,
									 m_window,
									 XCB_EVENT_MASK_STRUCTURE_NOTIFY,
									 reinterpret_cast<const char*>(&event));
		xcb_flush(m_connection);

		m_isMaximized = false;
		m_isMinimized = false;
	}
}

bool
XCBWindow::isMaximized() const {
	return m_isMaximized;
}

bool
XCBWindow::isMinimized() const {
	return m_isMinimized;
}
#pragma endregion

#pragma region Window attributes
void
XCBWindow::setTitle(const std::string_view title) {
	if (m_connection != nullptr &&
			m_window != XCB_WINDOW_NONE) {
		xcb_change_property(m_connection,
												XCB_PROP_MODE_REPLACE,
												m_window,
												XCB_ATOM_WM_NAME,
												XCB_ATOM_STRING,
												8,
												static_cast<uint32_t>(title.size()),
												title.data());
		xcb_flush(m_connection);

		m_title = title;
	}
}

std::string_view
XCBWindow::getTitle() const {
	return m_title;
}

void
XCBWindow::setResizable(const bool resizable) {
	if (m_connection != nullptr &&
			m_window != XCB_WINDOW_NONE) {
		Detail::applyResizableHint(m_connection,
															 m_window,
															 m_width,
															 m_height,
															 resizable);
		xcb_flush(m_connection);

		m_isResizable = resizable;
	}
}

bool
XCBWindow::isResizable() const {
	return m_isResizable;
}

void
XCBWindow::setDecorated(const bool decorated) {
	if (m_connection != nullptr &&
			m_window != XCB_WINDOW_NONE) {
		Detail::applyDecoratedHint(m_connection, m_window, decorated);
		xcb_flush(m_connection);
		
		m_isDecorated = decorated;
	}
}

bool
XCBWindow::isDecorated() const {
	return m_isDecorated;
}
#pragma endregion

};