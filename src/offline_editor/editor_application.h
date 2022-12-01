#pragma once

#include <rad/core/types.h>

namespace Rad::Editor {

class EditorApplicationImpl;

class EditorApplication {
 public:
  EditorApplication(int argc, char** argv);

  void Run();

 private:
  Share<EditorApplicationImpl> _impl;
};

}  // namespace Rad::Editor
