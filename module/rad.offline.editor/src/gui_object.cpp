#include <rad/offline/editor/gui_object.h>

namespace Rad {

GuiObject::GuiObject(Application* app, int priority, bool isAlive)
    : _app(app), _priority(priority), _isAlive(isAlive) {}

}  // namespace Rad
