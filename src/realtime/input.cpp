#include <rad/realtime/input.h>

#include <GLFW/glfw3.h>

namespace Rad {

static int RadKeyToGlfwKey(KeyCode keyCode);

void Input::Update(const Window& window) {
#if defined(RAD_REALTIME_BUILD_OPENGL)
  GLFWwindow* glfw = reinterpret_cast<GLFWwindow*>(window.GetHandler());
  double x, y;
  glfwGetCursorPos(glfw, &x, &y);
  _mousePosition = Vector2f((float)x, (float)y);

  std::array<bool, KeyCodeSize> nowKeyState{};
  for (size_t i = 0; i < nowKeyState.size(); i++) {
    int state = glfwGetKey(glfw, RadKeyToGlfwKey((KeyCode)i));
    if (state == GLFW_PRESS) {
      nowKeyState[i] = true;
    } else {
      nowKeyState[i] = false;
    }
  }
  for (size_t i = 0; i < nowKeyState.size(); i++) {
    auto p = _keyNow[i];
    auto n = nowKeyState[i];
    _keyDown[i] = false;
    _keyUp[i] = false;
    if (!p && n) {
      _keyDown[i] = true;
    }
    if (p && !n) {
      _keyUp[i] = true;
    }
    _keyNow[i] = n;
  }

  std::array<bool, MouseButtonSize> nowBtnState{};
  for (size_t i = 0; i < nowBtnState.size(); i++) {
    int state = glfwGetMouseButton(glfw, (int)i);
    if (state == GLFW_PRESS) {
      nowBtnState[i] = true;
    } else {
      nowBtnState[i] = false;
    }
  }
  for (size_t i = 0; i < nowBtnState.size(); i++) {
    bool p = _mouseNow[i];
    bool n = nowBtnState[i];
    _mouseDown[i] = false;
    _mouseUp[i] = false;
    if (!p && n) {
      _mouseDown[i] = true;
    }
    if (p && !n) {
      _mouseUp[i] = true;
    }
    _mouseNow[i] = n;
  }

  _nowScroll = _scrollDelta;
  _scrollDelta = Vector2f::Zero();
#endif
}

void Input::OnScroll(const Vector2f& scroll) {
  _scrollDelta += scroll;
}

int RadKeyToGlfwKey(KeyCode keyCode) {
  switch (keyCode) {
    case KeyCode::Backspace:
      return GLFW_KEY_BACKSPACE;
    case KeyCode::Delete:
      return GLFW_KEY_DELETE;
    case KeyCode::Tab:
      return GLFW_KEY_TAB;
    case KeyCode::Pause:
      return GLFW_KEY_PAUSE;
    case KeyCode::Escape:
      return GLFW_KEY_ESCAPE;
    case KeyCode::Space:
      return GLFW_KEY_SPACE;
    case KeyCode::KeypadPeriod:
      return GLFW_KEY_PERIOD;
    case KeyCode::KeypadDivide:
      return GLFW_KEY_KP_DIVIDE;
    case KeyCode::KeypadMultiply:
      return GLFW_KEY_KP_MULTIPLY;
    case KeyCode::KeypadMinus:
      return GLFW_KEY_KP_SUBTRACT;
    case KeyCode::KeypadPlus:
      return GLFW_KEY_KP_ADD;
    case KeyCode::KeypadEnter:
      return GLFW_KEY_KP_ENTER;
    case KeyCode::KeypadEquals:
      return GLFW_KEY_KP_EQUAL;
    case KeyCode::UpArrow:
      return GLFW_KEY_UP;
    case KeyCode::DownArrow:
      return GLFW_KEY_DOWN;
    case KeyCode::RightArrow:
      return GLFW_KEY_RIGHT;
    case KeyCode::LeftArrow:
      return GLFW_KEY_LEFT;
    case KeyCode::Insert:
      return GLFW_KEY_INSERT;
    case KeyCode::Home:
      return GLFW_KEY_HOME;
    case KeyCode::End:
      return GLFW_KEY_END;
    case KeyCode::PageUp:
      return GLFW_KEY_PAGE_DOWN;
    case KeyCode::PageDown:
      return GLFW_KEY_PAGE_UP;
    case KeyCode::F1:
      return GLFW_KEY_F1;
    case KeyCode::F2:
      return GLFW_KEY_F2;
    case KeyCode::F3:
      return GLFW_KEY_F3;
    case KeyCode::F4:
      return GLFW_KEY_F4;
    case KeyCode::F5:
      return GLFW_KEY_F5;
    case KeyCode::F6:
      return GLFW_KEY_F6;
    case KeyCode::F7:
      return GLFW_KEY_F7;
    case KeyCode::F8:
      return GLFW_KEY_F8;
    case KeyCode::F9:
      return GLFW_KEY_F9;
    case KeyCode::F10:
      return GLFW_KEY_F10;
    case KeyCode::F11:
      return GLFW_KEY_F11;
    case KeyCode::F12:
      return GLFW_KEY_F12;
    case KeyCode::A:
      return GLFW_KEY_A;
    case KeyCode::B:
      return GLFW_KEY_B;
    case KeyCode::C:
      return GLFW_KEY_C;
    case KeyCode::D:
      return GLFW_KEY_D;
    case KeyCode::E:
      return GLFW_KEY_E;
    case KeyCode::F:
      return GLFW_KEY_F;
    case KeyCode::G:
      return GLFW_KEY_G;
    case KeyCode::H:
      return GLFW_KEY_H;
    case KeyCode::I:
      return GLFW_KEY_I;
    case KeyCode::J:
      return GLFW_KEY_J;
    case KeyCode::K:
      return GLFW_KEY_K;
    case KeyCode::L:
      return GLFW_KEY_L;
    case KeyCode::M:
      return GLFW_KEY_M;
    case KeyCode::N:
      return GLFW_KEY_N;
    case KeyCode::O:
      return GLFW_KEY_O;
    case KeyCode::P:
      return GLFW_KEY_P;
    case KeyCode::Q:
      return GLFW_KEY_Q;
    case KeyCode::R:
      return GLFW_KEY_R;
    case KeyCode::S:
      return GLFW_KEY_S;
    case KeyCode::T:
      return GLFW_KEY_T;
    case KeyCode::U:
      return GLFW_KEY_U;
    case KeyCode::V:
      return GLFW_KEY_V;
    case KeyCode::W:
      return GLFW_KEY_W;
    case KeyCode::X:
      return GLFW_KEY_X;
    case KeyCode::Y:
      return GLFW_KEY_Y;
    case KeyCode::Z:
      return GLFW_KEY_Z;
    case KeyCode::LeftCurlyBracket:
      return GLFW_KEY_LEFT_BRACKET;
    case KeyCode::RightCurlyBracket:
      return GLFW_KEY_RIGHT_BRACKET;
    case KeyCode::Numlock:
      return GLFW_KEY_NUM_LOCK;
    case KeyCode::CapsLock:
      return GLFW_KEY_CAPS_LOCK;
    case KeyCode::ScrollLock:
      return GLFW_KEY_SCROLL_LOCK;
    case KeyCode::RightShift:
      return GLFW_KEY_RIGHT_SHIFT;
    case KeyCode::LeftShift:
      return GLFW_KEY_LEFT_SHIFT;
    case KeyCode::RightControl:
      return GLFW_KEY_RIGHT_CONTROL;
    case KeyCode::LeftControl:
      return GLFW_KEY_LEFT_CONTROL;
    case KeyCode::RightAlt:
      return GLFW_KEY_RIGHT_ALT;
    case KeyCode::LeftAlt:
      return GLFW_KEY_LEFT_ALT;
    default:
      return GLFW_KEY_UNKNOWN;
  }
}

}  // namespace Rad
