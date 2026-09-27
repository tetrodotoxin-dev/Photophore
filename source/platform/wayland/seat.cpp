// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "perimortem/core/perimortem.h"

#ifdef PERI_LINUX

#include <string.h>
#include <unistd.h>
#include <wayland-client.h>

#include "photophore/platform/wayland/seat.hpp"

using namespace Perimortem;
using namespace Photophore::Platform::Wayland;

// Release requests were added after the original proxy destruction path.
// Select by the negotiated version so an older compositor never receives a
// request that its protocol object cannot interpret.
static auto release(wl_keyboard* keyboard) -> void {
  if (wl_keyboard_get_version(keyboard) >= WL_KEYBOARD_RELEASE_SINCE_VERSION) {
    wl_keyboard_release(keyboard);
  } else {
    wl_keyboard_destroy(keyboard);
  }
}

static auto release(wl_pointer* pointer) -> void {
  if (wl_pointer_get_version(pointer) >= WL_POINTER_RELEASE_SINCE_VERSION) {
    wl_pointer_release(pointer);
  } else {
    wl_pointer_destroy(pointer);
  }
}

static auto release(wl_seat* seat) -> void {
  if (wl_seat_get_version(seat) >= WL_SEAT_RELEASE_SINCE_VERSION) {
    wl_seat_release(seat);
  } else {
    wl_seat_destroy(seat);
  }
}

Seat::~Seat() {
  destroy();
}

auto Seat::attach(void* selected_surface) -> Bool {
  if (!selected_surface || surface) {
    return False;
  }

  surface = selected_surface;
  return True;
}

auto Seat::register_global(
    void* registry,
    U32 name,
    const char* interface,
    U32 version) -> void {
  if (seat || strcmp(interface, wl_seat_interface.name) != 0) {
    return;
  }

  // The first matching global supplies this window's input. Cap the protocol
  // version at the event set implemented here, so newer events cannot
  // arrive without a corresponding handler.
  U32 selected_version = version < 9 ? version : 9;
  seat = wl_registry_bind(
      static_cast<wl_registry*>(registry), name, &wl_seat_interface,
      selected_version);
  if (!seat) {
    return;
  }

  // Dispatchers receive decoded arguments in protocol declaration order. The
  // event switch calls our handlers directly instead of asking libffi to build
  // a native call from each event signature. The mutable owner supplied at
  // binding remains alive until its proxies have been destroyed.
  static const auto keyboard_dispatch =
      +[](const void* context, void*, uint32_t opcode, const wl_message*,
          wl_argument* arguments) -> int {
    enum class Event : uint32_t {
      Keymap,
      Enter,
      Leave,
      Key,
      Modifiers,
      RepeatInfo
    };
    auto& input = *static_cast<Seat*>(const_cast<void*>(context));
    switch (static_cast<Event>(opcode)) {
    case Event::Keymap: {
      int32_t descriptor = arguments[1].h;
      // Physical control input does not interpret the text keymap, but the
      // client still owns the descriptor delivered with each map change.
      if (descriptor >= 0) {
        close(descriptor);
      }
      return 0;
    }
    case Event::Enter: {
      void* surface = arguments[1].o;
      wl_array* keys = arguments[2].a;
      if (surface != input.surface) {
        return 0;
      }

      // Enter includes controls pressed before this surface gained focus.
      // Seed from that inventory rather than waiting for their original press.
      input.values.clear_keyboard();
      input.keyboard_focused = True;
      const auto* codes = static_cast<const U32*>(keys->data);
      for (Count index = 0; index < keys->size / sizeof(U32); index++) {
        input.values.set_key(
            Photophore::Platform::Input::translate_keyboard(codes[index]),
            True);
      }
      return 0;
    }
    case Event::Leave: {
      void* surface = arguments[1].o;
      if (surface == input.surface) {
        input.keyboard_focused = False;
        input.values.clear_keyboard();
      }
      return 0;
    }
    case Event::Key: {
      U32 key = arguments[2].u;
      U32 state = arguments[3].u;
      if (input.keyboard_focused) {
        input.values.set_key(
            Photophore::Platform::Input::translate_keyboard(key),
            Bool(state != WL_KEYBOARD_KEY_STATE_RELEASED));
      }
      return 0;
    }
    case Event::Modifiers:
    case Event::RepeatInfo:
      return 0;
    }
    return -1;
  };
  static const auto pointer_dispatch =
      +[](const void* context, void*, uint32_t opcode, const wl_message*,
          wl_argument* arguments) -> int {
    enum class Event : uint32_t {
      Enter,
      Leave,
      Motion,
      Button,
      Axis,
      Frame,
      AxisSource,
      AxisStop,
      AxisDiscrete,
      AxisValue120,
      AxisRelativeDirection
    };
    auto& input = *static_cast<Seat*>(const_cast<void*>(context));
    switch (static_cast<Event>(opcode)) {
    case Event::Enter: {
      void* surface = arguments[1].o;
      wl_fixed_t x = arguments[2].f;
      wl_fixed_t y = arguments[3].f;
      if (surface == input.surface) {
        input.values.set_pointer(
            R32(wl_fixed_to_double(x)), R32(wl_fixed_to_double(y)));
      }
      return 0;
    }
    case Event::Leave: {
      void* surface = arguments[1].o;
      if (surface == input.surface) {
        input.values.clear_pointer();
      }
      return 0;
    }
    case Event::Motion: {
      wl_fixed_t x = arguments[1].f;
      wl_fixed_t y = arguments[2].f;
      if (input.values.is_pointer_active()) {
        input.values.set_pointer(
            R32(wl_fixed_to_double(x)), R32(wl_fixed_to_double(y)));
      }
      return 0;
    }
    case Event::Button: {
      U32 button = arguments[2].u;
      U32 state = arguments[3].u;
      if (input.values.is_pointer_active()) {
        input.values.set_key(
            Photophore::Platform::Input::translate_button(button),
            Bool(state == WL_POINTER_BUTTON_STATE_PRESSED));
      }
      return 0;
    }
    case Event::Axis: {
      U32 axis = arguments[1].u;
      wl_fixed_t value = arguments[2].f;
      if (!input.values.is_pointer_active()) {
        return 0;
      }

      R32 amount = R32(wl_fixed_to_double(value));
      if (axis == WL_POINTER_AXIS_HORIZONTAL_SCROLL) {
        input.values.scroll(amount, 0);
      } else if (axis == WL_POINTER_AXIS_VERTICAL_SCROLL) {
        input.values.scroll(0, amount);
      }
      return 0;
    }
    case Event::Frame:
    case Event::AxisSource:
    case Event::AxisStop:
    case Event::AxisDiscrete:
    case Event::AxisValue120:
    case Event::AxisRelativeDirection:
      return 0;
    }
    return -1;
  };
  static const auto seat_dispatch =
      +[](const void* context, void* target, uint32_t opcode, const wl_message*,
          wl_argument* arguments) -> int {
    enum class Event : uint32_t { Capabilities, Name };
    auto& input = *static_cast<Seat*>(const_cast<void*>(context));
    switch (static_cast<Event>(opcode)) {
    case Event::Capabilities: {
      auto* seat = static_cast<wl_seat*>(target);
      U32 capabilities = arguments[0].u;
      Bool has_keyboard =
          Bool((capabilities & WL_SEAT_CAPABILITY_KEYBOARD) != 0);
      Bool has_pointer = Bool((capabilities & WL_SEAT_CAPABILITY_POINTER) != 0);
      // Capabilities may disappear while the surface remains alive. Clear
      // held state when releasing a proxy so its next snapshot reports the
      // releases that the removed device can no longer deliver.
      if (has_keyboard && !input.keyboard) {
        input.keyboard = wl_seat_get_keyboard(seat);
        if (input.keyboard) {
          wl_proxy_add_dispatcher(
              static_cast<wl_proxy*>(input.keyboard), keyboard_dispatch, &input,
              &input);
        }
      } else if (!has_keyboard && input.keyboard) {
        release(static_cast<wl_keyboard*>(input.keyboard));
        input.keyboard = nullptr;
        input.keyboard_focused = False;
        input.values.clear_keyboard();
      }

      if (has_pointer && !input.pointer) {
        input.pointer = wl_seat_get_pointer(seat);
        if (input.pointer) {
          wl_proxy_add_dispatcher(
              static_cast<wl_proxy*>(input.pointer), pointer_dispatch, &input,
              &input);
        }
      } else if (!has_pointer && input.pointer) {
        release(static_cast<wl_pointer*>(input.pointer));
        input.pointer = nullptr;
        input.values.clear_pointer();
      }
      return 0;
    }
    case Event::Name:
      return 0;
    }
    return -1;
  };
  seat_name = name;
  wl_proxy_add_dispatcher(
      static_cast<wl_proxy*>(seat), seat_dispatch, this, this);
}

auto Seat::remove_global(U32 name) -> void {
  if (seat_name != name) {
    return;
  }

  if (keyboard) {
    release(static_cast<wl_keyboard*>(keyboard));
    keyboard = nullptr;
  }

  if (pointer) {
    release(static_cast<wl_pointer*>(pointer));
    pointer = nullptr;
  }

  if (seat) {
    release(static_cast<wl_seat*>(seat));
    seat = nullptr;
  }

  seat_name = 0;
  keyboard_focused = False;
  values.clear_keyboard();
  values.clear_pointer();
}

auto Seat::destroy() -> void {
  remove_global(seat_name);
  surface = nullptr;
}

#endif
