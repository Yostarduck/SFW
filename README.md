# SFW — Soulless Frame Windowing

**SFW (Soulless Frame Windowing)** is an open-source, cross-platform C++17 library
that does exactly one thing: it creates and manages operating-system windows.
Nothing else.

There is no renderer, no input abstraction, no audio, no asset system, and no
hidden event loop running behind your back. SFW hands you a window and a handful
of explicit calls to manage it — then gets out of the way. Graphics (Vulkan,
OpenGL, Direct3D, Metal, …), input, audio, networking, and every other part of
your application are entirely yours to build however you like, with the
confidence that the windowing layer isn't doing anything "soulful" under the
hood.

## Philosophy

- **Does one thing.** Window creation, destruction, and state management — full stop.
- **No magic.** You own the main loop. SFW only acts when you call it.
- **No dependencies you didn't ask for.** No graphics or audio libraries are pulled in.
- **Bring your own everything else.** Use `getNativeHandle()` to wire up your
  renderer or input system against the real OS handle.

## What it gives you

The entire public API is the abstract [`SFW::Window`](SFW/include/SFW_Window.h)
class:

- **Lifecycle** — `create()` / `createInternal()`, `destroy()`, `isCreated()`, `getNativeHandle()`
- **Events** — `pollEvents()`, `requestClose()`, `shouldClose()`
- **Visibility & focus** — `show()`, `hide()`, `isVisible()`, `focus()`, `hasFocus()`
- **Position & size** — `setPosition()`, `getPosition()`, `setSize()`, `getSize()`, `getFramebufferSize()`
- **Window state** — `maximize()`, `minimize()`, `restore()`, `isMaximized()`, `isMinimized()`
- **Attributes** — `setTitle()`, `getTitle()`, `setResizable()`, `isResizable()`, `setDecorated()`, `isDecorated()`

## Platforms & backends

The backend is selected entirely at compile time — there is no runtime dispatch.

| Platform | Backend      | Status   |
| -------- | ------------ | -------- |
| Windows  | Win32        | Complete |
| Linux    | XCB          | Complete |
| Linux    | Wayland      | Complete |
| Linux    | X11 (Xlib)   | Complete |
| macOS    | Cocoa        | Planned  |

## Requirements

- A C++17 compiler
- CMake ≥ 3.16
- Linux only, depending on the chosen backend:
  - **XCB:** `xcb`
  - **Wayland:** `wayland-client`, `wayland-protocols`, `wayland-scanner`
  - **X11:** `libX11`

## Building

### Helper scripts

The repository ships small wrappers around CMake. The `build`/`rebuild` scripts
configure and compile; the `start` scripts launch the example.

| Script                    | What it does                                            | Arguments                                   |
| ------------------------- | ------------------------------------------------------- | ------------------------------------------- |
| `build.sh` / `build.bat`  | Wipes `Build/`, then configures and compiles from scratch | `Debug` (default) \| `Release`            |
| `rebuild.sh` / `rebuild.bat` | Incremental recompile (no reconfigure, no wipe)      | `Debug` (default) \| `Release`              |
| `start.sh` / `start.bat`  | Runs the built `Example`                                | `Debug`\|`Release` and `x64`\|`x86` (any order) |

```bash
# Linux / macOS                      # Windows
./build.sh Release                   build.bat Release
./rebuild.sh Debug                   rebuild.bat Debug
./start.sh Release x64               start.bat Release x64
```

`build.sh` uses the Ninja generator; `build.bat` uses the default (Visual
Studio) generator. Output binaries land in `Build/<x64|x86>/<Config>/`.

> **Note:** the `build`/`rebuild` scripts do **not** enable the example by
> default. To get a runnable `Example` for `start` to launch, configure once
> with `-DSFW_BUILD_EXAMPLE=ON` (see below), then use `start`.

### Manual CMake

```bash
# Configure (with the example enabled) and build
cmake -B Build -G Ninja -DCMAKE_BUILD_TYPE=Debug -DSFW_BUILD_EXAMPLE=ON
cmake --build Build --config Debug
```

Useful flags (passed on the command line as `-D<flag>=ON`):

| Flag                 | Default | Purpose                                   |
| -------------------- | ------- | ----------------------------------------- |
| `SFW_BUILD_EXAMPLE`  | `OFF`   | Build the `Example` executable            |
| `SFW_BUILD_SHARED`   | `OFF`   | Build a shared library instead of static  |
| `SFW_USE_XCB`        | on Linux when none set | Use the XCB backend         |
| `SFW_USE_WAYLAND`    | `OFF`   | Use the Wayland backend                   |
| `SFW_USE_X11`        | `OFF`   | Use the X11 backend                       |

On Linux, exactly one backend is active; XCB is used when none is selected
explicitly.

## Using SFW in your project

SFW builds as a static (default) or shared CMake library target named `SFW`.
Drop the repository into your tree (e.g. as a submodule under `external/SFW`)
and wire it up:

```cmake
add_subdirectory(external/SFW)

target_link_libraries(MyApp PRIVATE SFW)
target_include_directories(MyApp PRIVATE external/SFW/SFW/include)
```

Then include the public header (or the `SFW.h` umbrella header, which pulls in
the right platform window header for you):

```cpp
#include "SFW_Window.h"
```

## Example

The snippet below mirrors [`Example/source/main.cpp`](Example/source/main.cpp):
SFW manages the window, and everything inside the loop is yours.

```cpp
#include "SFW_Window.h"

int main()
{
  SFW::WindowCreateInfo createInfo;
  createInfo.title     = "My Application";
  createInfo.x         = 0;
  createInfo.y         = 0;
  createInfo.width     = 1280;
  createInfo.height    = 720;
  createInfo.visible   = true;
  createInfo.resizable = true;
  createInfo.decorated = true;

  SFW::Window* window = SFW::Window::create(createInfo);
  if (window == nullptr) {
    return -1;
  }

  // SFW only owns the window. Hook up your own renderer, input, audio, etc.
  // against the real OS handle when you need it:
  //   void* nativeHandle = window->getNativeHandle();
  //   (HWND on Windows; xcb_window_t / X11 Window / wl_surface* on Linux)

  while (!window->shouldClose()) {
    window->pollEvents();

    // Your frame goes here: update, render, present, mix audio, ...
  }

  window->destroy();
  delete window;

  return 0;
}
```

The application owns the window's lifetime: when `shouldClose()` becomes true
(for example, the user clicks the close button), you call `destroy()` and then
`delete` the window — SFW never tears it down behind your back.

## License

SFW is open source.
