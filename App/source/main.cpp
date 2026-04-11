#include "SFW_Window.h"

#include <iostream>
#include <thread>

#include <iostream>
#include <vector>
#include <sstream>
#include <algorithm>

//////////////////////////////// OS Nuetral Headers ////////////////////
#include "OISInputManager.h"
#include "OISException.h"
#include "OISKeyboard.h"
#include "OISMouse.h"
#include "OISJoyStick.h"
#include "OISEvents.h"
////////////////////////////////////////////////////////////////////////

//////////////////////////////////// Advanced Usage ////////////////////
#include "OISForceFeedback.h"
////////////////////////////////////////////////////////////////////////

//////////////////////////////////// Needed Linux Headers //////////////
#if defined OIS_LINUX_PLATFORM
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#endif
////////////////////////////////////////////////////////////////////////

using namespace OIS;

//-- Some local prototypes --//
void doStartup(void* nativeHandle);
void handleNonBufferedKeys();
void handleNonBufferedMouse();
void handleNonBufferedJoy(JoyStick* js);

//-- Easy access globals --//
bool appRunning = true; //Global Exit Flag

const char* g_DeviceType[6] = { "OISUnknown", "OISKeyboard", "OISMouse", "OISJoyStick", "OISTablet", "OISOther" };

InputManager* g_InputManager  = nullptr;								   //Our Input System
Keyboard* g_kb				        = nullptr;								   //Keyboard Device
Mouse* g_m					          = nullptr;								   //Mouse Device
JoyStick* g_joys[4]			      = { nullptr, nullptr, nullptr, nullptr }; //This demo supports up to 4 controllers

//-- OS Specific Globals --//
#if defined OIS_LINUX_PLATFORM
Display* xDisp = 0;
#endif

//////////// Common Event handler class ////////
class EventHandler : public KeyListener, public MouseListener, public JoyStickListener
{
 public:
  EventHandler() { }
	~EventHandler() { }
	bool keyPressed(const KeyEvent& arg)
	{
		std::cout << " KeyPressed {" << std::hex << arg.key << std::dec
				  << "} || Character (" << (char)arg.text << ")" << std::endl;

		if(arg.key == OIS::KeyCode::KC_UP)
		{
			std::cout << "up key!\n";
		}

		return true;
	}
	bool keyReleased(const KeyEvent& arg)
	{
		if(arg.key == KC_ESCAPE || arg.key == KC_Q)
			appRunning = false;
		std::cout << "KeyReleased {" << std::hex << arg.key << std::dec
				  << "}\n";
		return true;
	}
	bool mouseMoved(const MouseEvent& arg)
	{
		const OIS::MouseState& s = arg.state;
		std::cout << "\nMouseMoved: Abs("
				  << s.X.abs << ", " << s.Y.abs << ", " << s.Z.abs << ") Rel("
				  << s.X.rel << ", " << s.Y.rel << ", " << s.Z.rel << ")";
		return true;
	}
	bool mousePressed(const MouseEvent& arg, MouseButtonID id)
	{
		const OIS::MouseState& s = arg.state;
		std::cout << "\nMouse button #" << id << " pressed. Abs("
				  << s.X.abs << ", " << s.Y.abs << ", " << s.Z.abs << ") Rel("
				  << s.X.rel << ", " << s.Y.rel << ", " << s.Z.rel << ")";
		return true;
	}
	bool mouseReleased(const MouseEvent& arg, MouseButtonID id)
	{
		const OIS::MouseState& s = arg.state;
		std::cout << "\nMouse button #" << id << " released. Abs("
				  << s.X.abs << ", " << s.Y.abs << ", " << s.Z.abs << ") Rel("
				  << s.X.rel << ", " << s.Y.rel << ", " << s.Z.rel << ")";
		return true;
	}
	bool buttonPressed(const JoyStickEvent& arg, int button)
	{
		std::cout << std::endl
				  << arg.device->vendor() << ". Button Pressed # " << button;
		return true;
	}
	bool buttonReleased(const JoyStickEvent& arg, int button)
	{
		std::cout << std::endl
				  << arg.device->vendor() << ". Button Released # " << button;
		return true;
	}
	bool axisMoved(const JoyStickEvent& arg, int axis)
	{
		//Provide a little dead zone
		if(arg.state.mAxes[axis].abs > 2500 || arg.state.mAxes[axis].abs < -2500)
			std::cout << std::endl
					  << arg.device->vendor() << ". Axis # " << axis << " Value: " << arg.state.mAxes[axis].abs;
		return true;
	}
	bool sliderMoved(const JoyStickEvent& arg, int index)
	{
		std::cout << std::endl
				  << arg.device->vendor() << ". Slider # " << index
				  << " X Value: " << arg.state.mSliders[index].abX
				  << " Y Value: " << arg.state.mSliders[index].abY;
		return true;
	}
	bool povMoved(const JoyStickEvent& arg, int pov)
	{
		std::cout << std::endl
				  << arg.device->vendor() << ". POV" << pov << " ";

		if(arg.state.mPOV[pov].direction & Pov::North) //Going up
			std::cout << "North";
		else if(arg.state.mPOV[pov].direction & Pov::South) //Going down
			std::cout << "South";

		if(arg.state.mPOV[pov].direction & Pov::East) //Going right
			std::cout << "East";
		else if(arg.state.mPOV[pov].direction & Pov::West) //Going left
			std::cout << "West";

		if(arg.state.mPOV[pov].direction == Pov::Centered) //stopped/centered out
			std::cout << "Centered";
		return true;
	}

	bool vector3Moved(const JoyStickEvent& arg, int index)
	{
		std::cout.precision(2);
		std::cout.flags(std::ios::fixed | std::ios::right);
		std::cout << std::endl
				  << arg.device->vendor() << ". Orientation # " << index
				  << " X Value: " << arg.state.mVectors[index].x
				  << " Y Value: " << arg.state.mVectors[index].y
				  << " Z Value: " << arg.state.mVectors[index].z;
		std::cout.precision();
		std::cout.flags();
		return true;
	}
};

//Create a global instance
EventHandler handler;

int32_t
main() {
  SFW::BaseWindow::WindowCreateInfo createInfo;
  createInfo.title = "SFW Example Window";
  createInfo.x = 100;
  createInfo.y = 100;
  createInfo.width = 800;
  createInfo.height = 600;
  createInfo.visible = true;
  createInfo.resizable = false;
  createInfo.decorated = true;

  SFW::BaseWindow* window = new SFW::XCBWindow();
  if (!window->create(createInfo)) {
    delete window;
    return -1;
  }

  doStartup(window->getNativeHandle());

  while (!window->shouldClose()) {
    std::this_thread::yield();
    
    window->pollEvents();
    
    if (g_kb) {
      g_kb->capture();
      if (!g_kb->buffered())
        handleNonBufferedKeys();
    }
    
    if (g_m) {
      g_m->capture();
      if (!g_m->buffered())
        handleNonBufferedMouse();
    }
    
    for (int i = 0; i < 4; ++i) {
      if (g_joys[i]) {
        g_joys[i]->capture();
        if (!g_joys[i]->buffered())
          handleNonBufferedJoy(g_joys[i]);
      }
    }
  }

  window->destroy();
  delete window;
  
  std::cout << "Window closed successfully." << std::endl;
  return 0;
}

void
doStartup(void* nativeHandle) {
	ParamList pl;

#if defined OIS_LINUX_PLATFORM
	if (!(xDisp = XOpenDisplay(nullptr)))
		OIS_EXCEPT(E_General, "Error opening X!");

	// XCBWindow already created, mapped, and registered WM_DELETE_WINDOW.
	// xcb_window_t and Xlib Window are both X resource IDs — same value, safe cast.
	Window xWin = static_cast<Window>(reinterpret_cast<uintptr_t>(nativeHandle));

	// Let OIS receive structural events on its Xlib connection.
	XSelectInput(xDisp, xWin, StructureNotifyMask | SubstructureNotifyMask);

	std::ostringstream wnd;
	wnd << xWin;
	pl.insert(std::make_pair(std::string("WINDOW"), wnd.str()));
  
  pl.insert(std::make_pair(std::string("x11_mouse_grab"), std::string("false")));
  pl.insert(std::make_pair(std::string("x11_mouse_hide"), std::string("false")));
#endif

	//This never returns null.. it will raise an exception on errors
	g_InputManager = InputManager::createInputSystem(pl);

	//Lets enable all addons that were compiled in:
	g_InputManager->enableAddOnFactory(InputManager::AddOn_All);

	//Print debugging information
	unsigned int v = g_InputManager->getVersionNumber();
	std::cout << "OIS Version: " << (v >> 16) << "." << ((v >> 8) & 0x000000FF) << "." << (v & 0x000000FF)
			  << "\nRelease Name: " << g_InputManager->getVersionName()
			  << "\nManager: " << g_InputManager->inputSystemName()
			  << "\nTotal Keyboards: " << g_InputManager->getNumberOfDevices(OISKeyboard)
			  << "\nTotal Mice: " << g_InputManager->getNumberOfDevices(OISMouse)
			  << "\nTotal JoySticks: " << g_InputManager->getNumberOfDevices(OISJoyStick);

	//List all devices
	DeviceList list = g_InputManager->listFreeDevices();
	for(DeviceList::iterator i = list.begin(); i != list.end(); ++i)
		std::cout << "\n\tDevice: " << g_DeviceType[i->first] << " Vendor: " << i->second;

	g_kb = (Keyboard*)g_InputManager->createInputObject(OISKeyboard, true);
	g_kb->setEventCallback(&handler);

	g_m = (Mouse*)g_InputManager->createInputObject(OISMouse, true);
	g_m->setEventCallback(&handler);
	const MouseState& ms = g_m->getMouseState();
	ms.width			 = 100;
	ms.height			 = 100;

	try
	{
		//This demo uses at most 4 joysticks - use old way to create (i.e. disregard vendor)
		int numSticks = std::min(g_InputManager->getNumberOfDevices(OISJoyStick), 4);
		for(int i = 0; i < numSticks; ++i)
		{
			g_joys[i] = (JoyStick*)g_InputManager->createInputObject(OISJoyStick, true);
			g_joys[i]->setEventCallback(&handler);
			std::cout << "\n\nCreating Joystick " << (i + 1)
					  << "\n\tAxes: " << g_joys[i]->getNumberOfComponents(OIS_Axis)
					  << "\n\tSliders: " << g_joys[i]->getNumberOfComponents(OIS_Slider)
					  << "\n\tPOV/HATs: " << g_joys[i]->getNumberOfComponents(OIS_POV)
					  << "\n\tButtons: " << g_joys[i]->getNumberOfComponents(OIS_Button)
					  << "\n\tVector3: " << g_joys[i]->getNumberOfComponents(OIS_Vector3);
		}
	}
	catch(OIS::Exception& ex)
	{
		std::cout << "\nException raised on joystick creation: " << ex.eText << std::endl;
	}
}

void handleNonBufferedKeys()
{
	if(g_kb->isKeyDown(KC_ESCAPE) || g_kb->isKeyDown(KC_Q))
		appRunning = false;

	if(g_kb->isModifierDown(Keyboard::Shift))
		std::cout << "Shift is down..\n";
	if(g_kb->isModifierDown(Keyboard::Alt))
		std::cout << "Alt is down..\n";
	if(g_kb->isModifierDown(Keyboard::Ctrl))
		std::cout << "Ctrl is down..\n";
}

void handleNonBufferedMouse()
{
	//Just dump the current mouse state
	const MouseState& ms = g_m->getMouseState();
	std::cout << "\nMouse: Abs(" << ms.X.abs << " " << ms.Y.abs << " " << ms.Z.abs
			  << ") B: " << ms.buttons << " Rel(" << ms.X.rel << " " << ms.Y.rel << " " << ms.Z.rel << ")";
}

void handleNonBufferedJoy(JoyStick* js)
{
	//Just dump the current joy state
	const JoyStickState& joy = js->getJoyStickState();
	for(unsigned int i = 0; i < joy.mAxes.size(); ++i)
		std::cout << "\nAxis " << i << " X: " << joy.mAxes[i].abs;
}