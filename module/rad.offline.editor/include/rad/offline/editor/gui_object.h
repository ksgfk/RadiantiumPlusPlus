#pragma once

#include <string>
#include <utility>

#include <rad/core/types.h>

namespace Rad {

class Application;

template <class Functor>
struct ScopeGuard {
  ScopeGuard(Functor&& t) : _func(std::move(t)) {}
  ~ScopeGuard() noexcept { _func(); }

 private:
  Functor _func;
};

class GuiObject {
 public:
  virtual ~GuiObject() noexcept = default;

  virtual void OnStart() {}
  virtual void OnGui() = 0;

  bool IsAlive() const { return _isAlive; }
  int GetPriority() const { return _priority; }
  const std::string& GetName() const { return _name; }

  void DrawPSR(Vector3f* pos, Vector3f* sca, Eigen::Quaternionf* rot);

 protected:
  GuiObject(Application* app, int priority, bool isAlive, const std::string name);

  Application* _app;
  int _priority;
  bool _isAlive;
  std::string _name;
};

}  // namespace Rad
