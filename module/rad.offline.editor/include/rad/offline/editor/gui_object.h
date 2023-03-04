#pragma once

namespace Rad {

class Application;

class GuiObject {
 public:
  virtual ~GuiObject() noexcept = default;

  virtual void OnStart() {}
  virtual void OnGui() = 0;

  bool IsAlive() const { return _isAlive; }
  int GetPriority() const { return _priority; }

 protected:
  GuiObject(Application* app, int priority, bool isAlive);

  Application* _app;
  int _priority;
  bool _isAlive;
};

}  // namespace Rad
