#include <iostream>

#include "SFW_Window.h"

int32_t
main(int32_t argc, char** argv) {
  std::cout << "Hello World!" << std::endl;

  SFW::WindowCreateInfo createInfo;
  createInfo.title = "SFW Example";
  createInfo.x = 0;
  createInfo.y = 0;
  createInfo.width = 1080;
  createInfo.height = 720;
  createInfo.visible = true;
  createInfo.resizable = false;
  createInfo.decorated = true;

  SFW::Window* window = SFW::Window::create(createInfo);

  if (window == nullptr) {
    std::cerr << "Failed to create window!" << std::endl;
    return -1;
  }

  while (!window->shouldClose()) {
    window->pollEvents();
  }

  window->destroy();

  delete window;

  return 0;
}