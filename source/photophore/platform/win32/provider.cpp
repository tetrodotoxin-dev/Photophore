// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "photophore/platform/win32/provider.hpp"

#ifdef TOOLCHAIN_WINDOWS

#include <windowsx.h>

#include "photophore/platform/state.hpp"

#define VK_USE_PLATFORM_WIN32_KHR

#include <vulkan/vulkan.h>

#include "perimortem/core/data.hpp"
#include "perimortem/core/synchronization/mutex.hpp"
#include "perimortem/memory/thread/vector.hpp"

#pragma comment(lib, "user32.lib")

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Photophore::Input;
using namespace Photophore::Platform;

struct WindowClassRegistration {
  ATOM atom = 0;
  HINSTANCE module = nullptr;

  constexpr auto is_valid() const -> Bool { return atom && module; }
};

static Synchronization::Mutex class_mutex;
static ATOM class_atom = 0;
static HINSTANCE class_module = nullptr;
static Count class_windows = 0;

// All Windows use one procedure and class policy. Acquisition registers that
// class for the first Window and keeps it available while any Window remains.
static auto acquire_window_class(WNDPROC process) -> WindowClassRegistration {
  Synchronization::Mutex::Lock lock(class_mutex);
  if (class_atom) {
    ++class_windows;
    return {class_atom, class_module};
  }

  // Local Win32 classes are identified by their name and module. Every Window
  // passes this same executable module to `CreateWindowExW`, while the direct
  // procedure pointer continues to enter Photophore's implementation.
  class_module = GetModuleHandleW(nullptr);
  if (!class_module) {
    return {};
  }

  WNDCLASSEXW descriptor = {};
  descriptor.cbSize = sizeof(descriptor);
  descriptor.hInstance = class_module;
  descriptor.lpszClassName = L"Tetrodotoxin.Photophore.Window";
  descriptor.hCursor = LoadCursorW(nullptr, IDC_ARROW);
  descriptor.lpfnWndProc = process;
  class_atom = RegisterClassExW(&descriptor);
  if (!class_atom) {
    class_module = nullptr;
    return {};
  }

  class_windows = 1;
  return {class_atom, class_module};
}

static auto release_window_class() -> void {
  Synchronization::Mutex::Lock lock(class_mutex);
  if (!class_windows || --class_windows) {
    return;
  }

  if (UnregisterClassW(MAKEINTATOM(class_atom), class_module)) {
    class_atom = 0;
    class_module = nullptr;
  }
}

auto Win32::Provider::create(U32 width, U32 height, View::Bytes title)
    -> Win32::Provider* {
  auto* window =
      create_state<Win32::Provider>([] { return Win32::Provider(); });
  if (!window) {
    return nullptr;
  }

  window->initialize(width, height, title);
  if (window->failed || !window->surface) {
    window->destroy();
    return nullptr;
  }

  return window;
}

auto Win32::Provider::initialize(U32 width, U32 height, View::Bytes title)
    -> void {
  static auto configure = [](Win32::Provider& window, U32 width, U32 height) {
    if (width != window.logical_width || height != window.logical_height) {
      window.logical_width = width;
      window.logical_height = height;
      window.needs_resize = True;
    }
  };

  initial_width = logical_width = width;
  initial_height = logical_height = height;
  if (width > S32(U32(-1) >> 1) || height > S32(U32(-1) >> 1)) {
    failed = True;
    return;
  }

  if (title.get_size() > Count(U32(-1) >> 1)) {
    failed = True;
    return;
  }

  for (Count index = 0; index < title.get_size(); index++) {
    if (title[index] == 0) {
      failed = True;
      return;
    }
  }

  const S32 title_size = S32(title.get_size());
  S32 length = 0;
  const char* utf8 = nullptr;
  if (title_size) {
    utf8 = Data::cast<const char>(title.get_data());
    length = MultiByteToWideChar(
        CP_UTF8, MB_ERR_INVALID_CHARS, utf8, title_size, nullptr, 0);
    if (!length) {
      failed = True;
      return;
    }
  }

  Thread::Vector<wchar_t> text;
  text.resize(Count(length) + 1);
  if (title_size && MultiByteToWideChar(
                        CP_UTF8, MB_ERR_INVALID_CHARS, utf8, title_size,
                        text.get_data(), length) != length) {
    failed = True;
    return;
  }

  text.get_data()[length] = L'\0';

  const WNDPROC process = [](HWND handle, UINT message, WPARAM wparam,
                             LPARAM lparam) -> LRESULT {
    auto* window = Data::bit_cast<Win32::Provider*>(
        GetWindowLongPtrW(handle, GWLP_USERDATA));
    // CreateWindowEx sends messages before it returns. Install the receiver
    // during WM_NCCREATE so those early callbacks already reach this Window.
    if (message == WM_NCCREATE) {
      auto* creation = Data::bit_cast<CREATESTRUCTW*>(lparam);
      window = Data::cast<Win32::Provider>(creation->lpCreateParams);
      window->surface = handle;
      SetWindowLongPtrW(
          handle, GWLP_USERDATA, Data::bit_cast<LONG_PTR>(window));
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
      const auto* area = Data::bit_cast<const RECT*>(lparam);
      SetWindowPos(
          handle, nullptr, area->left, area->top, area->right - area->left,
          area->bottom - area->top, SWP_NOACTIVATE | SWP_NOZORDER);
      return 0;
    }

    // A focus transition completes this Window's held control interval. Clear
    // its physical state here so the next snapshot publishes those releases.
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

      const auto key = wparam == VK_PAUSE ? Collector::Key::Pause
                                          : Collector::translate_keyboard(code);
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
      const Bool down = message == WM_LBUTTONDOWN ||
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
      input.set_key(Collector::translate_button(button), Bool(down));
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

    // Forwarding unconsumed WM_SYSKEY messages preserves User32 Alt shortcuts
    // and window controls after physical controls have entered the snapshot.

    return DefWindowProcW(handle, message, wparam, lparam);
  };

  const auto registration = acquire_window_class(process);
  if (!registration.is_valid()) {
    failed = True;
    return;
  }

  class_acquired = True;
  module = registration.module;

  // CreateWindowEx interprets dimensions according to the calling thread's DPI
  // context. Per monitor awareness makes the requested client extent physical
  // pixels. Restoring the prior context immediately after creation keeps later
  // host work under its original policy.
  const auto awareness =
      SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
  RECT area = {0, 0, LONG(width), LONG(height)};
  const Bool adjusted = AdjustWindowRectExForDpi(
      &area, WS_OVERLAPPEDWINDOW, FALSE, 0, USER_DEFAULT_SCREEN_DPI);
  const S64 outer_width = S64(area.right) - area.left;
  const S64 outer_height = S64(area.bottom) - area.top;
  if (adjusted && outer_width <= S32(U32(-1) >> 1) &&
      outer_height <= S32(U32(-1) >> 1)) {
    surface = CreateWindowExW(
        0, MAKEINTATOM(registration.atom), text.get_data(), WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, S32(outer_width), S32(outer_height),
        nullptr, nullptr, module, this);
  }

  if (awareness) {
    SetThreadDpiAwarenessContext(awareness);
  }

  if (!surface) {
    failed = True;
    return;
  }

  ShowWindow(surface, SW_SHOW);

  RECT client;
  if (GetClientRect(surface, &client)) {
    configure(*this, U32(client.right), U32(client.bottom));
  }
}

auto Win32::Provider::destroy() -> void {
  if (surface) {
    DestroyWindow(surface);
  }

  if (class_acquired) {
    release_window_class();
  }

  destroy_state(this);
}

auto Win32::Provider::poll_events() -> EventStatus {
  if (get_event_status() != EventStatus::Ready) {
    return get_event_status();
  }

  // Only this window's messages contribute to its observation. WM_QUIT is
  // thread state, so preserve it for the application pump after marking this
  // window closed.

  MSG message;
  while (PeekMessageW(&message, surface, 0, 0, PM_REMOVE)) {
    if (message.message == WM_QUIT) {
      close_requested = True;
      PostQuitMessage(S32(message.wParam));
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

auto Win32::Provider::get_presentation() -> Presentation {
  static const Presentation::Operations operations = {
    .get_instance_extension =
        [](void*) {
          return View::Bytes(
              VK_KHR_WIN32_SURFACE_EXTENSION_NAME[0],
              sizeof(VK_KHR_WIN32_SURFACE_EXTENSION_NAME) - 1);
        },

    .create_surface =
        [](void* context, VkInstance instance, PFN_vkGetInstanceProcAddr load,
           VkSurfaceKHR* output) {
          auto* window = Data::cast<Win32::Provider>(context);
          if (!window->module || !window->surface) {
            return VK_ERROR_INITIALIZATION_FAILED;
          }

          auto create = Data::bit_cast<PFN_vkCreateWin32SurfaceKHR>(
              load(instance, "vkCreateWin32SurfaceKHR"));
          if (!create) {
            return VK_ERROR_EXTENSION_NOT_PRESENT;
          }

          VkWin32SurfaceCreateInfoKHR info = {
            VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR};

          info.hinstance = window->module;
          info.hwnd = window->surface;
          return create(instance, &info, nullptr, output);
        },
  };

  return Presentation::create(this, operations);
}

#endif
