#include <rad/offline/editor/gui_object.h>

namespace Rad {

GuiObject::GuiObject(Application* app, int priority, bool isAlive, const std::string name)
    : _app(app), _priority(priority), _isAlive(isAlive), _name(name) {}

}  // namespace Rad
