#pragma once

#include "fwd.h"

#include <string>
#include <functional>

namespace Rad {

struct GlfwWindowOptions {
  Vector2i Size;
  std::string Title;
  bool EnableOpenGL;
  bool EnableOpenGLDebugLayer;
  bool IsMaximize;
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
void RadSetWindowSizeGlfw(const Vector2i& size);

void AddWindowFocusEventGlfw(std::function<void(GLFWwindow*, int)>);
void AddCursorEnterEventGlfw(std::function<void(GLFWwindow*, int)>);
void AddCursorPosEventGlfw(std::function<void(GLFWwindow*, Vector2d)>);
void AddMouseButtonEventGlfw(std::function<void(GLFWwindow*, int, int, int)>);
void AddScrollEventGlfw(std::function<void(GLFWwindow*, Vector2d)>);
void AddKeyEventGlfw(std::function<void(GLFWwindow*, int, int, int, int)>);
void AddCharEventGlfw(std::function<void(GLFWwindow*, unsigned int)>);
void AddMonitorEventGlfw(std::function<void(GLFWmonitor*, int)>);

}  // namespace Rad
