// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "perimortem/core/perimortem.h"

#ifdef PERI_WINDOWS
#ifndef UNICODE
#define UNICODE
#endif
#include <limits.h>
#include <stdio.h>
#include <windows.h>
#include <windowsx.h>

#include "perimortem/memory/dynamic/vector.hpp"

#include "photophore/platform/window.hpp"
#pragma comment(lib, "user32.lib")

using namespace Perimortem;
using namespace Photophore::Platform;

// Each window owns its registration as well as its HWND. Unregistering after
// destruction prevents User32 retaining a callback into an unloaded module.
struct Win32Window {
  ATOM registration = 0;
};

Window::Window(U32 width, U32 height, const char* title) {
  static auto configure = [](Window& window, U32 width, U32 height) {
    if (width != window.logical_width || height != window.logical_height) {
      window.logical_width = width;
      window.logical_height = height;
      window.needs_resize = True;
    }
  };
  initial_width = logical_width = width;
  initial_height = logical_height = height;
  if (width > INT_MAX || height > INT_MAX) {
    failed = True;
    return;
  }
  const int length = MultiByteToWideChar(
      CP_UTF8, MB_ERR_INVALID_CHARS, title ? title : "", -1, nullptr, 0);
  if (!length) {
    failed = True;
    return;
  }
  Memory::Dynamic::Vector<wchar_t> text;
  text.resize(length);
  if (!MultiByteToWideChar(
          CP_UTF8, MB_ERR_INVALID_CHARS, title ? title : "", -1,
          text.get_data(), length)) {
    failed = True;
    return;
  }
  auto* state = new Win32Window();
  native = state;
  connection = GetModuleHandleW(nullptr);
  wchar_t class_name[64];
  swprintf(class_name, 64, L"Photophore.%p", static_cast<void*>(this));
  WNDCLASSEXW descriptor = {};
  descriptor.cbSize = sizeof(descriptor);
  descriptor.hInstance = static_cast<HINSTANCE>(connection);
  descriptor.lpszClassName = class_name;
  descriptor.hCursor = LoadCursorW(nullptr, IDC_ARROW);
  descriptor.lpfnWndProc = [](HWND handle, UINT message, WPARAM wparam,
                              LPARAM lparam) -> LRESULT {
    auto* window =
        reinterpret_cast<Window*>(GetWindowLongPtrW(handle, GWLP_USERDATA));
    // CreateWindowEx sends messages before it returns. Install the receiver
    // during WM_NCCREATE so those early callbacks already reach this Window.
    if (message == WM_NCCREATE) {
      window = static_cast<Window*>(
          reinterpret_cast<CREATESTRUCTW*>(lparam)->lpCreateParams);
      window->surface = handle;
      SetWindowLongPtrW(
          handle, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(window));
    }
    if (!window) {
      return DefWindowProcW(handle, message, wparam, lparam);
    }
    auto& input = window->input_collector;
    switch (message) {
    case WM_CLOSE:
      window->close_requested = True;
      return 0;
    case WM_NCDESTROY:
      window->close_requested = True;
      window->surface = nullptr;
      SetWindowLongPtrW(handle, GWLP_USERDATA, 0);
      break;
    case WM_SIZE: {
      RECT area;
      if (GetClientRect(handle, &area)) {
        configure(*window, U32(area.right), U32(area.bottom));
      }
      return 0;
    }
    case WM_DPICHANGED: {
      const auto* area = reinterpret_cast<const RECT*>(lparam);
      SetWindowPos(
          handle, nullptr, area->left, area->top, area->right - area->left,
          area->bottom - area->top, SWP_NOACTIVATE | SWP_NOZORDER);
      return 0;
    }
    // Focus can leave while a control is down. Clear its physical state here
    // because the matching release may be delivered to another window.
    case WM_KILLFOCUS:
      input.clear_keyboard();
      input.clear_pointer();
      if (GetCapture() == handle) {
        ReleaseCapture();
      }
      break;
    case WM_KEYDOWN:
    case WM_KEYUP:
    case WM_SYSKEYDOWN:
    case WM_SYSKEYUP: {
      // Use the scan code rather than the translated character, preserving
      // physical controls when the user switches keyboard layout. The extended
      // bit distinguishes keys that share a base scan code.
      U32 code = (U64(lparam) >> 16) & 0xff;
      if (U64(lparam) & (U64(1) << 24)) {
        code |= 0xE000;
      }
      const auto key = wparam == VK_PAUSE ? Input::Key::Pause
                                          : Input::translate_keyboard(code);
      input.set_key(
          key, Bool(message == WM_KEYDOWN || message == WM_SYSKEYDOWN));
      break;
    }
    case WM_MOUSEMOVE: {
      if (!input.is_pointer_active()) {
        TRACKMOUSEEVENT track = {sizeof(track), TME_LEAVE, handle, 0};
        TrackMouseEvent(&track);
      }
      input.set_pointer(R32(GET_X_LPARAM(lparam)), R32(GET_Y_LPARAM(lparam)));
      return 0;
    }
    case WM_MOUSELEAVE:
      if (GetCapture() != handle) {
        input.clear_pointer();
      }
      return 0;
    case WM_CAPTURECHANGED: {
      input.clear_pointer();
      POINT point;
      RECT client;
      if (GetCursorPos(&point) && ScreenToClient(handle, &point) &&
          GetClientRect(handle, &client) && PtInRect(&client, point)) {
        input.set_pointer(R32(point.x), R32(point.y));
      }
      return 0;
    }
    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
    case WM_RBUTTONDOWN:
    case WM_RBUTTONUP:
    case WM_MBUTTONDOWN:
    case WM_MBUTTONUP:
    case WM_XBUTTONDOWN:
    case WM_XBUTTONUP: {
      const bool down = message == WM_LBUTTONDOWN ||
                        message == WM_RBUTTONDOWN ||
                        message == WM_MBUTTONDOWN || message == WM_XBUTTONDOWN;
      U32 button = VK_LBUTTON;
      if (message == WM_RBUTTONDOWN || message == WM_RBUTTONUP) {
        button = VK_RBUTTON;
      } else if (message == WM_MBUTTONDOWN || message == WM_MBUTTONUP) {
        button = VK_MBUTTON;
      } else if (message == WM_XBUTTONDOWN || message == WM_XBUTTONUP) {
        button =
            GET_XBUTTON_WPARAM(wparam) == XBUTTON1 ? VK_XBUTTON1 : VK_XBUTTON2;
      }
      input.set_pointer(R32(GET_X_LPARAM(lparam)), R32(GET_Y_LPARAM(lparam)));
      input.set_key(Input::translate_button(button), Bool(down));
      // Capture keeps button release events routed here during a drag outside
      // the client area. Release it after the final button is up, then let
      // WM_CAPTURECHANGED restore ordinary pointer focus.
      if (down) {
        SetCapture(handle);
      } else if (
          !(wparam & (MK_LBUTTON | MK_RBUTTON | MK_MBUTTON | MK_XBUTTON1 |
                      MK_XBUTTON2)) &&
          GetCapture() == handle) {
        ReleaseCapture();
      }
      return message == WM_XBUTTONDOWN || message == WM_XBUTTONUP ? TRUE : 0;
    }
    case WM_MOUSEWHEEL:
    case WM_MOUSEHWHEEL: {
      const R32 delta = R32(GET_WHEEL_DELTA_WPARAM(wparam)) / WHEEL_DELTA;
      input.scroll(
          message == WM_MOUSEHWHEEL ? delta : R32(0),
          message == WM_MOUSEWHEEL ? -delta : R32(0));
      return 0;
    }
    case WM_ERASEBKGND:
      return 1;
    case WM_PAINT: {
      PAINTSTRUCT paint;
      BeginPaint(handle, &paint);
      EndPaint(handle, &paint);
      return 0;
    }
    }
    // System key handling must still reach User32 for Alt shortcuts and its
    // window controls. Ordinary controls have already entered the snapshot.
    return DefWindowProcW(handle, message, wparam, lparam);
  };
  state->registration = RegisterClassExW(&descriptor);
  if (!state->registration) {
    failed = True;
    return;
  }

  // A window inherits the creating thread's DPI context. Use physical client
  // pixels for this window, then restore the host thread's policy immediately.
  const auto awareness =
      SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
  RECT area = {0, 0, LONG(width), LONG(height)};
  const bool adjusted = AdjustWindowRectExForDpi(
      &area, WS_OVERLAPPEDWINDOW, FALSE, 0, USER_DEFAULT_SCREEN_DPI);
  const S64 outer_width = S64(area.right) - area.left;
  const S64 outer_height = S64(area.bottom) - area.top;
  if (adjusted && outer_width <= INT_MAX && outer_height <= INT_MAX) {
    surface = CreateWindowExW(
        0, MAKEINTATOM(state->registration), text.get_data(),
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, int(outer_width),
        int(outer_height), nullptr, nullptr, static_cast<HINSTANCE>(connection),
        this);
  }
  if (awareness) {
    SetThreadDpiAwarenessContext(awareness);
  }
  if (!surface) {
    failed = True;
    return;
  }
  ShowWindow(static_cast<HWND>(surface), SW_SHOW);
  RECT client;
  if (GetClientRect(static_cast<HWND>(surface), &client)) {
    configure(*this, U32(client.right), U32(client.bottom));
  }
}

Window::~Window() {
  if (surface) {
    DestroyWindow(static_cast<HWND>(surface));
  }
  auto* state = static_cast<Win32Window*>(native);
  if (state && state->registration) {
    UnregisterClassW(
        MAKEINTATOM(state->registration), static_cast<HINSTANCE>(connection));
  }
  delete state;
}

auto Window::poll_events() -> EventStatus {
  if (get_event_status() != EventStatus::Ready) {
    return get_event_status();
  }
  // Only this window's messages contribute to its observation. WM_QUIT is
  // thread state, so preserve it for the application pump after marking this
  // window closed.
  MSG message;
  while (PeekMessageW(&message, static_cast<HWND>(surface), 0, 0, PM_REMOVE)) {
    if (message.message == WM_QUIT) {
      close_requested = True;
      PostQuitMessage(int(message.wParam));
      break;
    }
    TranslateMessage(&message);
    DispatchMessageW(&message);
    if (close_requested) {
      break;
    }
  }
  input_snapshot = input_collector.collect(input_mapping);
  return get_event_status();
}

auto Window::get_presentation() const -> Presentation {
  return Presentation::create(Presentation::Kind::Win32, connection, surface);
}
#endif
