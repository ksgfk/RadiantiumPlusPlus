#pragma once

#include <rad/core/types.h>

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
void RadShowWindowGlfw();
bool RadShouldCloseWindowGlfw();
void RadPollEventGlfw();
void RadSwapBuffersGlfw();

}  // namespace Rad
