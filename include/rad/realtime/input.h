#pragma once

#include "window.h"

#include <rad/core/types.h>

#include <array>

namespace Rad {

enum class KeyCode : size_t {
  Backspace,
  Delete,
  Tab,
  Pause,
  Escape,
  Space,
  KeypadPeriod,
  KeypadDivide,
  KeypadMultiply,
  KeypadMinus,
  KeypadPlus,
  KeypadEnter,
  KeypadEquals,
  UpArrow,
  DownArrow,
  RightArrow,
  LeftArrow,
  Insert,
  Home,
  End,
  PageUp,
  PageDown,
  F1,
  F2,
  F3,
  F4,
  F5,
  F6,
  F7,
  F8,
  F9,
  F10,
  F11,
  F12,
  A,
  B,
  C,
  D,
  E,
  F,
  G,
  H,
  I,
  J,
  K,
  L,
  M,
  N,
  O,
  P,
  Q,
  R,
  S,
  T,
  U,
  V,
  W,
  X,
  Y,
  Z,
  LeftCurlyBracket,
  RightCurlyBracket,
  Numlock,
  CapsLock,
  ScrollLock,
  RightShift,
  LeftShift,
  RightControl,
  LeftControl,
  RightAlt,
  LeftAlt,
  KEY_CODE_SIZE
};

class Input {
 public:
  static constexpr size_t KeyCodeSize = (size_t)KeyCode::KEY_CODE_SIZE;
  static constexpr size_t MouseButtonSize = 3;

  void Update(const Window& window);
  void OnScroll(const Vector2f& scroll);

  bool GetKey(KeyCode keyCode) const { return _keyNow[(size_t)keyCode]; }
  bool GetKeyUp(KeyCode keyCode) const { return _keyUp[(size_t)keyCode]; }
  bool GetKeyDown(KeyCode keyCode) const { return _keyDown[(size_t)keyCode]; }
  bool GetMouseButton(int mouseButton) const { return _mouseNow[mouseButton]; }
  bool GetMouseButtonUp(int mouseButton) const { return _mouseUp[mouseButton]; }
  bool GetMouseButtonDown(int mouseButton) const { return _mouseDown[mouseButton]; }
  const Vector2f& MousePosition() const { return _mousePosition; }
  const Vector2f& Scroll() const { return _nowScroll; }

 private:
  std::array<bool, KeyCodeSize> _keyNow;
  std::array<bool, KeyCodeSize> _keyDown;
  std::array<bool, KeyCodeSize> _keyUp;
  std::array<bool, MouseButtonSize> _mouseNow;
  std::array<bool, MouseButtonSize> _mouseDown;
  std::array<bool, MouseButtonSize> _mouseUp;
  Vector2f _mousePosition;
  Vector2f _scrollDelta;
  Vector2f _nowScroll;
};

}  // namespace Rad
