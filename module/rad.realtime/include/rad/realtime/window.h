#pragma once

#include "fwd.h"

#include <string>

namespace Rad {

struct GlfwWindowOptions {
  Vector2i Size;
  std::string Title;
  bool EnableOpenGL;
  bool EnableOpenGLDebugLayer;
};

void RadInitGlfw();
void RadShutdownGlfw();

void RadCreateWindowGlfw(const GlfwWindowOptions& opts);
void RadDestroyWindowGlfw();
bool RadIsWindowActiveGlfw();
GLFWwindow* RadWindowHandlerGlfw();
void RadShowWindowGlfw();
bool RadShouldCloseWindowGlfw();
void RadPollEventGlfw();
void RadSwapBuffersGlfw();
Vector2i RadGetFrameBufferSizeGlfw();

}  // namespace Rad
