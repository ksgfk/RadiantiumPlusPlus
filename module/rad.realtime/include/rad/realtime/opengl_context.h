#pragma once

#include <rad/core/types.h>

#include <glad/gl.h>
#include <imgui.h>

#include <string>

namespace Rad {

struct ImguiInitOptions {
  std::string FontsPath;
  Float32 FontsSize;
  Float32 DpiScale;
};

void RadInitContextOpenGL();
void RadShutdownContextOpenGL();
void RadInitImguiOpenGL(const ImguiInitOptions& opts);
void RadShutdownImguiOpenGL();
void RadImguiNewFrame();
void RadImguiRender();

}  // namespace Rad
