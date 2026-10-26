// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "photophore/platform/input.hpp"

#ifdef PERI_LINUX
#include <linux/input-event-codes.h>
#elifdef PERI_WINDOWS
#include <windows.h>
#endif

auto Photophore::Platform::Input::set_key(Photophore::Input::Key key, Bool down)
    -> void {
  Count index = Count(key);
  if (key == Photophore::Input::Key::None ||
      index >= Photophore::Input::key_count) {
    return;
  }

  U64 mask = U64(1) << (index % word_bits);
  if (down) {
    current[index / word_bits] |= mask;
  } else {
    current[index / word_bits] &= ~mask;
  }
}

auto Photophore::Platform::Input::clear_keyboard() -> void {
  for (Count index = Count(Photophore::Input::Key::Escape);
       index < Count(Photophore::Input::Key::MousePrimary); index++) {
    set_key(Photophore::Input::Key(index), False);
  }
}

auto Photophore::Platform::Input::clear_pointer() -> void {
  for (Count index = Count(Photophore::Input::Key::MousePrimary);
       index <= Count(Photophore::Input::Key::MouseEight); index++) {
    set_key(Photophore::Input::Key(index), False);
  }
  pointer_focused = False;
}

auto Photophore::Platform::Input::collect(
    const Photophore::Input::Mapping& mapping) -> Photophore::Input::Snapshot {
  Perimortem::Core::Static::Vector<
      Photophore::Input::Key, Photophore::Input::key_count>
      current_keys;
  Perimortem::Core::Static::Vector<
      Photophore::Input::Key, Photophore::Input::key_count>
      previous_keys;
  Count current_count = 0;
  Count previous_count = 0;
  for (Count index = 1; index < Photophore::Input::key_count; index++) {
    U64 mask = U64(1) << (index % word_bits);
    if ((current[index / word_bits] & mask) != 0) {
      current_keys[current_count++] = Photophore::Input::Key(index);
    }
    if ((previous[index / word_bits] & mask) != 0) {
      previous_keys[previous_count++] = Photophore::Input::Key(index);
    }
  }

  // Each focused interval establishes its own pointer motion baseline. The
  // first observation in a new interval therefore begins with a zero delta.
  Photophore::Input::Pointer pointer_state = {
    pointer_x,
    pointer_y,
    pointer_focused && previous_pointer_focused ? pointer_x - previous_pointer_x
                                                : R32(0),
    pointer_focused && previous_pointer_focused ? pointer_y - previous_pointer_y
                                                : R32(0),
    scroll_x,
    scroll_y,
    pointer_focused,
  };
  Photophore::Input::Snapshot output = Photophore::Input::Snapshot::create(
      current_keys.slice(0, current_count),
      previous_keys.slice(0, previous_count), mapping, pointer_state);

  // Both key sets use the current Mapping. Advancing after value construction
  // preserves their shared interpretation, while clearing scroll gives each
  // completed observation exactly the accumulated wheel movement.
  previous = current;
  previous_pointer_x = pointer_x;
  previous_pointer_y = pointer_y;
  previous_pointer_focused = pointer_focused;
  scroll_x = 0;
  scroll_y = 0;
  return output;
}

auto Photophore::Platform::Input::set_pointer(R32 x, R32 y) -> void {
  if (!pointer_focused) {
    previous_pointer_focused = False;
  }
  pointer_x = x;
  pointer_y = y;
  pointer_focused = True;
}

auto Photophore::Platform::Input::scroll(R32 x, R32 y) -> void {
  if (pointer_focused) {
    scroll_x += x;
    scroll_y += y;
  }
}

auto Photophore::Platform::Input::translate_keyboard(U32 code) -> Key {
#ifdef PERI_LINUX

  // Wayland supplies Linux evdev key codes directly. Mapping physical positions
  // before text interpretation keeps controls stable across character layouts.
  switch (code) {
  case KEY_ESC:
    return Key::Escape;
  case KEY_F1:
    return Key::F1;
  case KEY_F2:
    return Key::F2;
  case KEY_F3:
    return Key::F3;
  case KEY_F4:
    return Key::F4;
  case KEY_F5:
    return Key::F5;
  case KEY_F6:
    return Key::F6;
  case KEY_F7:
    return Key::F7;
  case KEY_F8:
    return Key::F8;
  case KEY_F9:
    return Key::F9;
  case KEY_F10:
    return Key::F10;
  case KEY_F11:
    return Key::F11;
  case KEY_F12:
    return Key::F12;
  case KEY_F13:
    return Key::F13;
  case KEY_F14:
    return Key::F14;
  case KEY_F15:
    return Key::F15;
  case KEY_F16:
    return Key::F16;
  case KEY_F17:
    return Key::F17;
  case KEY_F18:
    return Key::F18;
  case KEY_F19:
    return Key::F19;
  case KEY_F20:
    return Key::F20;
  case KEY_F21:
    return Key::F21;
  case KEY_F22:
    return Key::F22;
  case KEY_F23:
    return Key::F23;
  case KEY_F24:
    return Key::F24;
  case KEY_SYSRQ:
    return Key::PrintScreen;
  case KEY_SCROLLLOCK:
    return Key::ScrollLock;
  case KEY_PAUSE:
    return Key::Pause;
  case KEY_GRAVE:
    return Key::Grave;
  case KEY_1:
    return Key::Digit1;
  case KEY_2:
    return Key::Digit2;
  case KEY_3:
    return Key::Digit3;
  case KEY_4:
    return Key::Digit4;
  case KEY_5:
    return Key::Digit5;
  case KEY_6:
    return Key::Digit6;
  case KEY_7:
    return Key::Digit7;
  case KEY_8:
    return Key::Digit8;
  case KEY_9:
    return Key::Digit9;
  case KEY_0:
    return Key::Digit0;
  case KEY_MINUS:
    return Key::Minus;
  case KEY_EQUAL:
    return Key::Equal;
  case KEY_BACKSPACE:
    return Key::Backspace;
  case KEY_TAB:
    return Key::Tab;
  case KEY_Q:
    return Key::Q;
  case KEY_W:
    return Key::W;
  case KEY_E:
    return Key::E;
  case KEY_R:
    return Key::R;
  case KEY_T:
    return Key::T;
  case KEY_Y:
    return Key::Y;
  case KEY_U:
    return Key::U;
  case KEY_I:
    return Key::I;
  case KEY_O:
    return Key::O;
  case KEY_P:
    return Key::P;
  case KEY_LEFTBRACE:
    return Key::LeftBracket;
  case KEY_RIGHTBRACE:
    return Key::RightBracket;
  case KEY_BACKSLASH:
    return Key::Backslash;
  case KEY_CAPSLOCK:
    return Key::CapsLock;
  case KEY_A:
    return Key::A;
  case KEY_S:
    return Key::S;
  case KEY_D:
    return Key::D;
  case KEY_F:
    return Key::F;
  case KEY_G:
    return Key::G;
  case KEY_H:
    return Key::H;
  case KEY_J:
    return Key::J;
  case KEY_K:
    return Key::K;
  case KEY_L:
    return Key::L;
  case KEY_SEMICOLON:
    return Key::Semicolon;
  case KEY_APOSTROPHE:
    return Key::Apostrophe;
  case KEY_ENTER:
    return Key::Enter;
  case KEY_LEFTSHIFT:
    return Key::LeftShift;
  case KEY_Z:
    return Key::Z;
  case KEY_X:
    return Key::X;
  case KEY_C:
    return Key::C;
  case KEY_V:
    return Key::V;
  case KEY_B:
    return Key::B;
  case KEY_N:
    return Key::N;
  case KEY_M:
    return Key::M;
  case KEY_COMMA:
    return Key::Comma;
  case KEY_DOT:
    return Key::Period;
  case KEY_SLASH:
    return Key::Slash;
  case KEY_RIGHTSHIFT:
    return Key::RightShift;
  case KEY_LEFTCTRL:
    return Key::LeftControl;
  case KEY_LEFTMETA:
    return Key::LeftSuper;
  case KEY_LEFTALT:
    return Key::LeftAlt;
  case KEY_SPACE:
    return Key::Space;
  case KEY_RIGHTALT:
    return Key::RightAlt;
  case KEY_RIGHTMETA:
    return Key::RightSuper;
  case KEY_MENU:
    return Key::Menu;
  case KEY_RIGHTCTRL:
    return Key::RightControl;
  case KEY_INSERT:
    return Key::Insert;
  case KEY_DELETE:
    return Key::Delete;
  case KEY_HOME:
    return Key::Home;
  case KEY_END:
    return Key::End;
  case KEY_PAGEUP:
    return Key::PageUp;
  case KEY_PAGEDOWN:
    return Key::PageDown;
  case KEY_UP:
    return Key::ArrowUp;
  case KEY_DOWN:
    return Key::ArrowDown;
  case KEY_LEFT:
    return Key::ArrowLeft;
  case KEY_RIGHT:
    return Key::ArrowRight;
  case KEY_NUMLOCK:
    return Key::NumLock;
  case KEY_KPSLASH:
    return Key::NumpadDivide;
  case KEY_KPASTERISK:
    return Key::NumpadMultiply;
  case KEY_KPMINUS:
    return Key::NumpadSubtract;
  case KEY_KPPLUS:
    return Key::NumpadAdd;
  case KEY_KPENTER:
    return Key::NumpadEnter;
  case KEY_KP0:
    return Key::Numpad0;
  case KEY_KP1:
    return Key::Numpad1;
  case KEY_KP2:
    return Key::Numpad2;
  case KEY_KP3:
    return Key::Numpad3;
  case KEY_KP4:
    return Key::Numpad4;
  case KEY_KP5:
    return Key::Numpad5;
  case KEY_KP6:
    return Key::Numpad6;
  case KEY_KP7:
    return Key::Numpad7;
  case KEY_KP8:
    return Key::Numpad8;
  case KEY_KP9:
    return Key::Numpad9;
  case KEY_KPDOT:
    return Key::NumpadDecimal;
  case KEY_KPEQUAL:
    return Key::NumpadEqual;
  case KEY_KPCOMMA:
    return Key::NumpadComma;
  case KEY_102ND:
    return Key::NonUsBackslash;
  case KEY_RO:
    return Key::InternationalRo;
  case KEY_YEN:
    return Key::InternationalYen;
  case KEY_KATAKANAHIRAGANA:
    return Key::Kana;
  case KEY_HENKAN:
    return Key::Convert;
  case KEY_MUHENKAN:
    return Key::NonConvert;
  case KEY_MUTE:
    return Key::Mute;
  case KEY_VOLUMEDOWN:
    return Key::VolumeDown;
  case KEY_VOLUMEUP:
    return Key::VolumeUp;
  case KEY_PLAYPAUSE:
    return Key::MediaPlayPause;
  case KEY_STOPCD:
    return Key::MediaStop;
  case KEY_PREVIOUSSONG:
    return Key::MediaPrevious;
  case KEY_NEXTSONG:
    return Key::MediaNext;
  case KEY_BACK:
    return Key::BrowserBack;
  case KEY_FORWARD:
    return Key::BrowserForward;
  case KEY_REFRESH:
    return Key::BrowserRefresh;
  case KEY_HOMEPAGE:
    return Key::BrowserHome;
  default:
    return Key::None;
  }

#elifdef PERI_WINDOWS

  // Scan 1 identifies physical positions independently of the active keyboard
  // layout. E0 prefixes distinguish keys that share their base code.
  switch (code) {
  case 0x0001:
    return Key::Escape;
  case 0x0002:
    return Key::Digit1;
  case 0x0003:
    return Key::Digit2;
  case 0x0004:
    return Key::Digit3;
  case 0x0005:
    return Key::Digit4;
  case 0x0006:
    return Key::Digit5;
  case 0x0007:
    return Key::Digit6;
  case 0x0008:
    return Key::Digit7;
  case 0x0009:
    return Key::Digit8;
  case 0x000A:
    return Key::Digit9;
  case 0x000B:
    return Key::Digit0;
  case 0x000C:
    return Key::Minus;
  case 0x000D:
    return Key::Equal;
  case 0x000E:
    return Key::Backspace;
  case 0x000F:
    return Key::Tab;
  case 0x0010:
    return Key::Q;
  case 0x0011:
    return Key::W;
  case 0x0012:
    return Key::E;
  case 0x0013:
    return Key::R;
  case 0x0014:
    return Key::T;
  case 0x0015:
    return Key::Y;
  case 0x0016:
    return Key::U;
  case 0x0017:
    return Key::I;
  case 0x0018:
    return Key::O;
  case 0x0019:
    return Key::P;
  case 0x001A:
    return Key::LeftBracket;
  case 0x001B:
    return Key::RightBracket;
  case 0x001C:
    return Key::Enter;
  case 0x001D:
    return Key::LeftControl;
  case 0x001E:
    return Key::A;
  case 0x001F:
    return Key::S;
  case 0x0020:
    return Key::D;
  case 0x0021:
    return Key::F;
  case 0x0022:
    return Key::G;
  case 0x0023:
    return Key::H;
  case 0x0024:
    return Key::J;
  case 0x0025:
    return Key::K;
  case 0x0026:
    return Key::L;
  case 0x0027:
    return Key::Semicolon;
  case 0x0028:
    return Key::Apostrophe;
  case 0x0029:
    return Key::Grave;
  case 0x002A:
    return Key::LeftShift;
  case 0x002B:
    return Key::Backslash;
  case 0x002C:
    return Key::Z;
  case 0x002D:
    return Key::X;
  case 0x002E:
    return Key::C;
  case 0x002F:
    return Key::V;
  case 0x0030:
    return Key::B;
  case 0x0031:
    return Key::N;
  case 0x0032:
    return Key::M;
  case 0x0033:
    return Key::Comma;
  case 0x0034:
    return Key::Period;
  case 0x0035:
    return Key::Slash;
  case 0x0036:
    return Key::RightShift;
  case 0x0037:
    return Key::NumpadMultiply;
  case 0x0038:
    return Key::LeftAlt;
  case 0x0039:
    return Key::Space;
  case 0x003A:
    return Key::CapsLock;
  case 0x003B:
    return Key::F1;
  case 0x003C:
    return Key::F2;
  case 0x003D:
    return Key::F3;
  case 0x003E:
    return Key::F4;
  case 0x003F:
    return Key::F5;
  case 0x0040:
    return Key::F6;
  case 0x0041:
    return Key::F7;
  case 0x0042:
    return Key::F8;
  case 0x0043:
    return Key::F9;
  case 0x0044:
    return Key::F10;
  case 0x0045:
    return Key::NumLock;
  case 0x0046:
    return Key::ScrollLock;
  case 0x0047:
    return Key::Numpad7;
  case 0x0048:
    return Key::Numpad8;
  case 0x0049:
    return Key::Numpad9;
  case 0x004A:
    return Key::NumpadSubtract;
  case 0x004B:
    return Key::Numpad4;
  case 0x004C:
    return Key::Numpad5;
  case 0x004D:
    return Key::Numpad6;
  case 0x004E:
    return Key::NumpadAdd;
  case 0x004F:
    return Key::Numpad1;
  case 0x0050:
    return Key::Numpad2;
  case 0x0051:
    return Key::Numpad3;
  case 0x0052:
    return Key::Numpad0;
  case 0x0053:
    return Key::NumpadDecimal;
  case 0x0054:
    return Key::PrintScreen;
  case 0x0056:
    return Key::NonUsBackslash;
  case 0x0057:
    return Key::F11;
  case 0x0058:
    return Key::F12;
  case 0x0059:
    return Key::NumpadEqual;
  case 0x0064:
    return Key::F13;
  case 0x0065:
    return Key::F14;
  case 0x0066:
    return Key::F15;
  case 0x0067:
    return Key::F16;
  case 0x0068:
    return Key::F17;
  case 0x0069:
    return Key::F18;
  case 0x006A:
    return Key::F19;
  case 0x006B:
    return Key::F20;
  case 0x006C:
    return Key::F21;
  case 0x006D:
    return Key::F22;
  case 0x006E:
    return Key::F23;
  case 0x0070:
    return Key::Kana;
  case 0x0073:
    return Key::InternationalRo;
  case 0x0076:
    return Key::F24;
  case 0x0079:
    return Key::Convert;
  case 0x007B:
    return Key::NonConvert;
  case 0x007D:
    return Key::InternationalYen;
  case 0x007E:
    return Key::NumpadComma;
  case 0xE010:
    return Key::MediaPrevious;
  case 0xE019:
    return Key::MediaNext;
  case 0xE01C:
    return Key::NumpadEnter;
  case 0xE01D:
    return Key::RightControl;
  case 0xE020:
    return Key::Mute;
  case 0xE022:
    return Key::MediaPlayPause;
  case 0xE024:
    return Key::MediaStop;
  case 0xE02E:
    return Key::VolumeDown;
  case 0xE030:
    return Key::VolumeUp;
  case 0xE032:
    return Key::BrowserHome;
  case 0xE035:
    return Key::NumpadDivide;
  case 0xE037:
    return Key::PrintScreen;
  case 0xE038:
    return Key::RightAlt;
  case 0xE045:
    return Key::NumLock;
  case 0xE046:
    return Key::Pause;
  case 0xE047:
    return Key::Home;
  case 0xE048:
    return Key::ArrowUp;
  case 0xE049:
    return Key::PageUp;
  case 0xE04B:
    return Key::ArrowLeft;
  case 0xE04D:
    return Key::ArrowRight;
  case 0xE04F:
    return Key::End;
  case 0xE050:
    return Key::ArrowDown;
  case 0xE051:
    return Key::PageDown;
  case 0xE052:
    return Key::Insert;
  case 0xE053:
    return Key::Delete;
  case 0xE05B:
    return Key::LeftSuper;
  case 0xE05C:
    return Key::RightSuper;
  case 0xE05D:
    return Key::Menu;
  case 0xE067:
    return Key::BrowserRefresh;
  case 0xE069:
    return Key::BrowserForward;
  case 0xE06A:
    return Key::BrowserBack;
  default:
    return Key::None;
  }

#else

  return Key::None;

#endif
}

auto Photophore::Platform::Input::translate_button(U32 code) -> Key {
#ifdef PERI_LINUX

  switch (code) {
  case BTN_LEFT:
    return Key::MousePrimary;
  case BTN_RIGHT:
    return Key::MouseSecondary;
  case BTN_MIDDLE:
    return Key::MouseMiddle;
  case BTN_SIDE:
  case BTN_BACK:
    return Key::MouseBack;
  case BTN_EXTRA:
  case BTN_FORWARD:
    return Key::MouseForward;
  case BTN_TASK:
    return Key::MouseSix;
  case BTN_0:
    return Key::MouseSeven;
  case BTN_1:
    return Key::MouseEight;
  default:
    return Key::None;
  }

#elifdef PERI_WINDOWS

  switch (code) {
  case VK_LBUTTON:
    return Key::MousePrimary;
  case VK_RBUTTON:
    return Key::MouseSecondary;
  case VK_MBUTTON:
    return Key::MouseMiddle;
  case VK_XBUTTON1:
    return Key::MouseBack;
  case VK_XBUTTON2:
    return Key::MouseForward;
  default:
    return Key::None;
  }

#else

  return Key::None;

#endif
}
