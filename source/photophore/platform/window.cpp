// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "photophore/platform/window.hpp"

#include "toolchain/memory/lifetime.hpp"

#include "perimortem/core/data.hpp"
#include "perimortem/memory/allocator/global.hpp"

#ifdef TOOLCHAIN_LINUX

#include <errno.h>
#include <poll.h>
#include <stddef.h>
#include <string.h>
#include <unistd.h>

#include "perimortem/core/view/null_terminated.hpp"
#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/system/process/library.hpp"

#elifdef TOOLCHAIN_WINDOWS

#ifndef UNICODE
#define UNICODE
#endif
#include <limits.h>
#include <stdio.h>
#include <windows.h>
#include <windowsx.h>

#include "perimortem/memory/thread/vector.hpp"

#pragma comment(lib, "user32.lib")

#endif

using namespace Toolchain::Memory;
using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Photophore::Input;
using namespace Photophore::Platform;

template <typename Type, typename Factory>
static auto create_native(Factory factory) -> Type* {
  static_assert(alignof(Type) <= Allocator::Global::allocation_alignment);
  auto allocation = Allocator::Global::allocate(sizeof(Type));
  if (allocation.get_capacity() < sizeof(Type)) {
    return nullptr;
  }

  return &Lifetime::construct<Type>(allocation.get_data(), factory);
}

template <typename Type>
static auto destroy_native(Type* value) -> void {
  if (!value) {
    return;
  }

  value->~Type();
  Allocator::Global::release(Data::cast<U8>(value));
}

#ifdef TOOLCHAIN_LINUX

using namespace Perimortem::System;

struct wl_proxy {};
struct wl_display {};

struct wl_array {
  CppSize size;
  CppSize allocation;
  void* data;
};

using wl_fixed_t = S32;

union wl_argument {
  S32 i;
  U32 u;
  wl_fixed_t f;
  const char* s;
  void* o;
  U32 n;
  wl_array* a;
  S32 h;
};

struct wl_message {
  const char* name;
  const char* signature;
  const void** types;
};

struct wl_interface {
  const char* name;
  S32 version;
  S32 request_count;
  const wl_message* requests;
  S32 event_count;
  const wl_message* events;
};

using WaylandDispatcher =
    auto(const void*, void*, U32, const wl_message*, wl_argument*) -> S32;

// Wayland declares dispatcher metadata as const even when that metadata names
// mutable callback state. This record keeps the ABI pointer const while its
// stored context retains the provider's original access.
struct CallbackContext {
  void* value;
};

template <typename Type>
static auto callback_value(const void* context) -> Type& {
  const auto* callback = Data::cast<CallbackContext>(context);
  return *Data::cast<Type>(callback->value);
}

static const wl_message xdg_toplevel_requests[] = {
  {"destroy", "", nullptr},
  {"set_parent", "?o", nullptr},
  {"set_title", "s", nullptr},
  {"set_app_id", "s", nullptr},
};

static const wl_message xdg_toplevel_events[] = {
  {"configure", "iia", nullptr},
  {"close", "", nullptr},
};

static const wl_interface photophore_xdg_toplevel_interface = {
  "xdg_toplevel", 1, 4, xdg_toplevel_requests, 2, xdg_toplevel_events,
};

static const void* xdg_get_toplevel_types[] = {
  &photophore_xdg_toplevel_interface,
};

static const wl_message xdg_surface_requests[] = {
  {"destroy", "", nullptr},
  {"get_toplevel", "n", xdg_get_toplevel_types},
  {"get_popup", "n?oo", nullptr},
  {"set_window_geometry", "iiii", nullptr},
  {"ack_configure", "u", nullptr},
};

static const wl_message xdg_surface_events[] = {
  {"configure", "u", nullptr},
};

static const wl_interface photophore_xdg_surface_interface = {
  "xdg_surface", 1, 5, xdg_surface_requests, 1, xdg_surface_events,
};

// WaylandClient is the private ABI connection to the host's Wayland runtime.
// Window owns one instance beside every proxy created through it, keeping the
// library loaded through native destruction and Vulkan surface use.
class WaylandClient {
 public:
  static auto create() -> WaylandClient*;
  ~WaylandClient();
  WaylandClient(WaylandClient&&) = delete;
  auto operator=(WaylandClient&&) -> WaylandClient& = delete;
  WaylandClient(const WaylandClient&) = delete;
  auto operator=(const WaylandClient&) -> WaylandClient& = delete;

  auto connect() const -> wl_display*;
  auto disconnect(wl_display* display) const -> void;
  auto get_registry(wl_display* display) const -> wl_proxy*;
  auto bind(
      wl_proxy* registry,
      U32 name,
      const wl_interface* interface,
      U32 version) const -> wl_proxy*;
  auto create_surface(wl_proxy* compositor) const -> wl_proxy*;
  auto get_keyboard(wl_proxy* seat) const -> wl_proxy*;
  auto get_pointer(wl_proxy* seat) const -> wl_proxy*;
  auto release_keyboard(wl_proxy* keyboard) const -> void;
  auto release_pointer(wl_proxy* pointer) const -> void;
  auto release_seat(wl_proxy* seat) const -> void;
  auto destroy_proxy(wl_proxy* proxy) const -> void;
  auto destroy_request(wl_proxy* proxy, U32 opcode) const -> void;
  auto commit(wl_proxy* surface) const -> void;
  auto set_buffer_scale(wl_proxy* surface, S32 scale) const -> void;
  auto add_dispatcher(
      wl_proxy* proxy,
      WaylandDispatcher* dispatcher,
      const void* dispatcher_data,
      void* data) const -> S32;
  auto marshal(
      wl_proxy* proxy,
      U32 opcode,
      const wl_interface* interface,
      U32 flags) const -> wl_proxy*;
  auto marshal(
      wl_proxy* proxy,
      U32 opcode,
      const wl_interface* interface,
      U32 flags,
      const char* value) const -> wl_proxy*;
  auto marshal(
      wl_proxy* proxy,
      U32 opcode,
      const wl_interface* interface,
      U32 flags,
      U32 value) const -> wl_proxy*;
  auto marshal(
      wl_proxy* proxy,
      U32 opcode,
      const wl_interface* interface,
      U32 flags,
      wl_proxy* value) const -> wl_proxy*;
  auto get_version(wl_proxy* proxy) const -> U32;
  auto dispatch_pending(wl_display* display) const -> S32;
  auto prepare_read(wl_display* display) const -> S32;
  auto cancel_read(wl_display* display) const -> void;
  auto read_events(wl_display* display) const -> S32;
  auto flush(wl_display* display) const -> S32;
  auto roundtrip(wl_display* display) const -> S32;
  auto get_fd(wl_display* display) const -> S32;

  const wl_interface* registry_interface = nullptr;
  const wl_interface* compositor_interface = nullptr;
  const wl_interface* surface_interface = nullptr;
  const wl_interface* seat_interface = nullptr;
  const wl_interface* keyboard_interface = nullptr;
  const wl_interface* pointer_interface = nullptr;
  wl_interface shell_interface = {};

 private:
  using DisplayConnect = auto(const char*) -> wl_display*;
  using DisplayDisconnect = auto(wl_display*) -> void;
  using DisplayOperation = auto(wl_display*) -> S32;
  using DisplayCancelRead = auto(wl_display*) -> void;
  using ProxyAddDispatcher =
      auto(wl_proxy*, WaylandDispatcher*, const void*, void*) -> S32;
  using ProxyDestroy = auto(wl_proxy*) -> void;
  using ProxyGetVersion = auto(wl_proxy*) -> U32;
  using ProxyMarshal = auto(wl_proxy*, U32, const wl_interface*, U32, U32, ...)
      -> wl_proxy*;

  // Only `create` constructs a client from the successfully opened Wayland
  // library. Explicit construction prevents another pointer from silently
  // becoming the runtime that defines these function and interface records.
  explicit WaylandClient(Process::Library* library) : library(library) {}
  auto load() -> Bool;
  auto symbol(View::Bytes name) const -> void*;
  auto release(wl_proxy* proxy, U32 version, U32 opcode) const -> void;

  Process::Library* library;
  DisplayConnect* display_connect = nullptr;
  DisplayDisconnect* display_disconnect = nullptr;
  DisplayOperation* display_dispatch_pending = nullptr;
  DisplayOperation* display_prepare_read = nullptr;
  DisplayCancelRead* display_cancel_read = nullptr;
  DisplayOperation* display_read_events = nullptr;
  DisplayOperation* display_flush = nullptr;
  DisplayOperation* display_roundtrip = nullptr;
  DisplayOperation* display_get_fd = nullptr;
  ProxyAddDispatcher* proxy_add_dispatcher = nullptr;
  ProxyDestroy* proxy_destroy = nullptr;
  ProxyGetVersion* proxy_get_version = nullptr;
  ProxyMarshal* proxy_marshal = nullptr;
  const void* shell_surface_types[2] = {};
  wl_message shell_requests[4] = {};
  wl_message shell_events[1] = {};
};

auto WaylandClient::create() -> WaylandClient* {
  Allocator::Arena errors;
  auto* library =
      Process::Library::open("libwayland-client.so.0"_view, errors)
          .visit(
              [](Process::Library& opened) {
                return create_native<Process::Library>(
                    [&opened] { return Data::take(opened); });
              },

              [](View::Bytes) -> Process::Library* { return nullptr; });

  if (!library) {
    return nullptr;
  }

  auto* client = create_native<WaylandClient>(
      [library] { return WaylandClient(library); });
  if (!client) {
    destroy_native(library);
    return nullptr;
  }

  if (!client->load()) {
    destroy_native(client);
    return nullptr;
  }

  return client;
}

WaylandClient::~WaylandClient() {
  destroy_native(library);
}

auto WaylandClient::symbol(View::Bytes name) const -> void* {
  Allocator::Arena errors;
  return library->symbol(name, errors)
      .visit(
          [](void* address) { return address; },
          [](View::Bytes) -> void* { return nullptr; });
}

auto WaylandClient::load() -> Bool {
  display_connect =
      Data::cast<DisplayConnect>(symbol("wl_display_connect"_view));
  display_disconnect =
      Data::cast<DisplayDisconnect>(symbol("wl_display_disconnect"_view));
  display_dispatch_pending =
      Data::cast<DisplayOperation>(symbol("wl_display_dispatch_pending"_view));
  display_prepare_read =
      Data::cast<DisplayOperation>(symbol("wl_display_prepare_read"_view));
  display_cancel_read =
      Data::cast<DisplayCancelRead>(symbol("wl_display_cancel_read"_view));
  display_read_events =
      Data::cast<DisplayOperation>(symbol("wl_display_read_events"_view));
  display_flush = Data::cast<DisplayOperation>(symbol("wl_display_flush"_view));
  display_roundtrip =
      Data::cast<DisplayOperation>(symbol("wl_display_roundtrip"_view));
  display_get_fd =
      Data::cast<DisplayOperation>(symbol("wl_display_get_fd"_view));
  proxy_add_dispatcher =
      Data::cast<ProxyAddDispatcher>(symbol("wl_proxy_add_dispatcher"_view));
  proxy_destroy = Data::cast<ProxyDestroy>(symbol("wl_proxy_destroy"_view));
  proxy_get_version =
      Data::cast<ProxyGetVersion>(symbol("wl_proxy_get_version"_view));
  proxy_marshal =
      Data::cast<ProxyMarshal>(symbol("wl_proxy_marshal_flags"_view));
  registry_interface =
      Data::cast<wl_interface>(symbol("wl_registry_interface"_view));
  compositor_interface =
      Data::cast<wl_interface>(symbol("wl_compositor_interface"_view));
  surface_interface =
      Data::cast<wl_interface>(symbol("wl_surface_interface"_view));
  seat_interface = Data::cast<wl_interface>(symbol("wl_seat_interface"_view));
  keyboard_interface =
      Data::cast<wl_interface>(symbol("wl_keyboard_interface"_view));
  pointer_interface =
      Data::cast<wl_interface>(symbol("wl_pointer_interface"_view));

  const Bool complete = Bool(
      display_connect && display_disconnect && display_dispatch_pending &&
      display_prepare_read && display_cancel_read && display_read_events &&
      display_flush && display_roundtrip && display_get_fd &&
      proxy_add_dispatcher && proxy_destroy && proxy_get_version &&
      proxy_marshal && registry_interface && compositor_interface &&
      surface_interface && seat_interface && keyboard_interface &&
      pointer_interface);
  if (!complete) {
    return False;
  }

  // Window binds version one of the stable desktop shell protocol. Each client
  // retains metadata through its proxies, including the core surface identity
  // supplied by the exact runtime that owns those objects.
  shell_surface_types[0] = &photophore_xdg_surface_interface;
  shell_surface_types[1] = surface_interface;
  shell_requests[0] = {"destroy", "", nullptr};
  shell_requests[1] = {"create_positioner", "n", nullptr};
  shell_requests[2] = {
    "get_xdg_surface",
    "no",
    shell_surface_types,
  };

  shell_requests[3] = {"pong", "u", nullptr};
  shell_events[0] = {"ping", "u", nullptr};
  shell_interface = {
    "xdg_wm_base", 1, 4, shell_requests, 1, shell_events,
  };

  return True;
}

auto WaylandClient::connect() const -> wl_display* {
  return display_connect(nullptr);
}

auto WaylandClient::disconnect(wl_display* display) const -> void {
  display_disconnect(display);
}

auto WaylandClient::get_registry(wl_display* display) const -> wl_proxy* {
  auto* proxy = Data::bit_cast<wl_proxy*>(display);
  return proxy_marshal(
      proxy, 1, registry_interface, proxy_get_version(proxy), 0, nullptr);
}

auto WaylandClient::bind(
    wl_proxy* registry,
    U32 name,
    const wl_interface* interface,
    U32 version) const -> wl_proxy* {
  return proxy_marshal(
      registry, 0, interface, version, 0, name, interface->name, version,
      nullptr);
}

auto WaylandClient::create_surface(wl_proxy* compositor) const -> wl_proxy* {
  return marshal(compositor, 0, surface_interface, 0);
}

auto WaylandClient::get_keyboard(wl_proxy* seat) const -> wl_proxy* {
  return marshal(seat, 1, keyboard_interface, 0);
}

auto WaylandClient::get_pointer(wl_proxy* seat) const -> wl_proxy* {
  return marshal(seat, 0, pointer_interface, 0);
}

auto WaylandClient::release_keyboard(wl_proxy* keyboard) const -> void {
  release(keyboard, 3, 0);
}

auto WaylandClient::release_pointer(wl_proxy* pointer) const -> void {
  release(pointer, 3, 1);
}

auto WaylandClient::release_seat(wl_proxy* seat) const -> void {
  release(seat, 5, 3);
}

auto WaylandClient::release(wl_proxy* proxy, U32 version, U32 opcode) const
    -> void {
  if (get_version(proxy) >= version) {
    destroy_request(proxy, opcode);
  } else {
    destroy_proxy(proxy);
  }
}

auto WaylandClient::destroy_proxy(wl_proxy* proxy) const -> void {
  proxy_destroy(proxy);
}

auto WaylandClient::destroy_request(wl_proxy* proxy, U32 opcode) const -> void {
  marshal(proxy, opcode, nullptr, 1);
}

auto WaylandClient::commit(wl_proxy* surface) const -> void {
  marshal(surface, 6, nullptr, 0);
}

auto WaylandClient::set_buffer_scale(wl_proxy* surface, S32 scale) const
    -> void {
  proxy_marshal(surface, 8, nullptr, proxy_get_version(surface), 0, scale);
}

auto WaylandClient::add_dispatcher(
    wl_proxy* proxy,
    WaylandDispatcher* dispatcher,
    const void* dispatcher_data,
    void* data) const -> S32 {
  return proxy_add_dispatcher(proxy, dispatcher, dispatcher_data, data);
}

auto WaylandClient::marshal(
    wl_proxy* proxy,
    U32 opcode,
    const wl_interface* interface,
    U32 flags) const -> wl_proxy* {
  return proxy_marshal(
      proxy, opcode, interface, proxy_get_version(proxy), flags, nullptr);
}

auto WaylandClient::marshal(
    wl_proxy* proxy,
    U32 opcode,
    const wl_interface* interface,
    U32 flags,
    const char* value) const -> wl_proxy* {
  return proxy_marshal(
      proxy, opcode, interface, proxy_get_version(proxy), flags, value);
}

auto WaylandClient::marshal(
    wl_proxy* proxy,
    U32 opcode,
    const wl_interface* interface,
    U32 flags,
    U32 value) const -> wl_proxy* {
  return proxy_marshal(
      proxy, opcode, interface, proxy_get_version(proxy), flags, value);
}

auto WaylandClient::marshal(
    wl_proxy* proxy,
    U32 opcode,
    const wl_interface* interface,
    U32 flags,
    wl_proxy* value) const -> wl_proxy* {
  return proxy_marshal(
      proxy, opcode, interface, proxy_get_version(proxy), flags, nullptr,
      value);
}

auto WaylandClient::get_version(wl_proxy* proxy) const -> U32 {
  return proxy_get_version(proxy);
}

auto WaylandClient::dispatch_pending(wl_display* display) const -> S32 {
  return display_dispatch_pending(display);
}

auto WaylandClient::prepare_read(wl_display* display) const -> S32 {
  return display_prepare_read(display);
}

auto WaylandClient::cancel_read(wl_display* display) const -> void {
  display_cancel_read(display);
}

auto WaylandClient::read_events(wl_display* display) const -> S32 {
  return display_read_events(display);
}

auto WaylandClient::flush(wl_display* display) const -> S32 {
  return display_flush(display);
}

auto WaylandClient::roundtrip(wl_display* display) const -> S32 {
  return display_roundtrip(display);
}

auto WaylandClient::get_fd(wl_display* display) const -> S32 {
  return display_get_fd(display);
}

class WaylandInput {
 public:
  WaylandInput(WaylandClient& client, Collector& values)
      : client(client), values(values), callback{this} {}
  ~WaylandInput();
  WaylandInput(WaylandInput&&) = delete;
  auto operator=(WaylandInput&&) -> WaylandInput& = delete;
  WaylandInput(const WaylandInput&) = delete;
  auto operator=(const WaylandInput&) -> WaylandInput& = delete;

  auto attach(wl_proxy* surface) -> Bool;
  auto register_global(
      wl_proxy* registry,
      U32 name,
      const char* interface,
      U32 version) -> void;
  auto remove_global(U32 name) -> void;
  auto destroy() -> void;

 private:
  WaylandClient& client;
  wl_proxy* seat = nullptr;
  wl_proxy* keyboard = nullptr;
  wl_proxy* pointer = nullptr;
  wl_proxy* surface = nullptr;
  U32 seat_name = 0;
  Collector& values;
  Bool keyboard_focused = False;
  CallbackContext callback;
};

WaylandInput::~WaylandInput() {
  destroy();
}

auto WaylandInput::attach(wl_proxy* selected_surface) -> Bool {
  if (!selected_surface || surface) {
    return False;
  }

  surface = selected_surface;
  return True;
}

auto WaylandInput::register_global(
    wl_proxy* registry,
    U32 name,
    const char* interface,
    U32 version) -> void {
  if (seat || strcmp(interface, client.seat_interface->name) != 0) {
    return;
  }

  // The first matching global supplies this window's input. Version nine is
  // the complete event set represented by the dispatch tables below, so the
  // negotiated proxy stays within that implemented contract.
  U32 selected_version = version < 9 ? version : 9;
  seat = client.bind(registry, name, client.seat_interface, selected_version);
  if (!seat) {
    return;
  }

  // Wayland decodes each event into the argument order declared by its
  // interface metadata. Each switch reads the corresponding signature. The
  // callback record points to this WaylandInput until the keyboard, pointer and
  // seat proxies are released.
  static const auto keyboard_dispatch =
      +[](const void* context, void*, U32 opcode, const wl_message*,
          wl_argument* arguments) -> S32 {
    enum class Event : U32 { Keymap, Enter, Leave, Key, Modifiers, RepeatInfo };
    auto& input = callback_value<WaylandInput>(context);
    switch (Event(opcode)) {
    case Event::Keymap: {
      S32 descriptor = arguments[1].h;
      // Physical controls use evdev codes directly. Photophore does not
      // interpret keyboard text, so it closes the delivered keymap descriptor
      // without mapping its contents.
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
      const auto* codes = Data::cast<U32>(keys->data);
      for (Count index = 0; index < keys->size / sizeof(U32); index++) {
        input.values.set_key(Collector::translate_keyboard(codes[index]), True);
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
            Collector::translate_keyboard(key), Bool(state != 0));
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
      +[](const void* context, void*, U32 opcode, const wl_message*,
          wl_argument* arguments) -> S32 {
    enum class Event : U32 {
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

    auto& input = callback_value<WaylandInput>(context);
    switch (Event(opcode)) {
    case Event::Enter: {
      void* surface = arguments[1].o;
      wl_fixed_t x = arguments[2].f;
      wl_fixed_t y = arguments[3].f;
      if (surface == input.surface) {
        input.values.set_pointer(R32(x) / 256.0f, R32(y) / 256.0f);
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
        input.values.set_pointer(R32(x) / 256.0f, R32(y) / 256.0f);
      }

      return 0;
    }

    case Event::Button: {
      U32 button = arguments[2].u;
      U32 state = arguments[3].u;
      if (input.values.is_pointer_active()) {
        input.values.set_key(
            Collector::translate_button(button), Bool(state == 1));
      }

      return 0;
    }

    case Event::Axis: {
      U32 axis = arguments[1].u;
      wl_fixed_t value = arguments[2].f;
      if (!input.values.is_pointer_active()) {
        return 0;
      }

      R32 amount = R32(value) / 256.0f;
      if (axis == 1) {
        input.values.scroll(amount, 0);
      } else if (axis == 0) {
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
      +[](const void* context, void* target, U32 opcode, const wl_message*,
          wl_argument* arguments) -> S32 {
    enum class Event : U32 { Capabilities, Name };
    auto& input = callback_value<WaylandInput>(context);
    switch (Event(opcode)) {
    case Event::Capabilities: {
      auto* seat = Data::cast<wl_proxy>(target);
      U32 capabilities = arguments[0].u;
      Bool has_keyboard = Bool((capabilities & 2) != 0);
      Bool has_pointer = Bool((capabilities & 1) != 0);
      // Capabilities may change while the surface remains alive. Releasing a
      // proxy also clears its held controls, making the next snapshot publish
      // the corresponding release transitions.
      if (has_keyboard && !input.keyboard) {
        input.keyboard = input.client.get_keyboard(seat);
        if (input.keyboard) {
          input.client.add_dispatcher(
              input.keyboard, keyboard_dispatch, &input.callback, &input);
        }
      } else if (!has_keyboard && input.keyboard) {
        input.client.release_keyboard(input.keyboard);
        input.keyboard = nullptr;
        input.keyboard_focused = False;
        input.values.clear_keyboard();
      }

      if (has_pointer && !input.pointer) {
        input.pointer = input.client.get_pointer(seat);
        if (input.pointer) {
          input.client.add_dispatcher(
              input.pointer, pointer_dispatch, &input.callback, &input);
        }
      } else if (!has_pointer && input.pointer) {
        input.client.release_pointer(input.pointer);
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
  client.add_dispatcher(seat, seat_dispatch, &callback, this);
}

auto WaylandInput::remove_global(U32 name) -> void {
  if (seat_name != name) {
    return;
  }

  if (keyboard) {
    client.release_keyboard(keyboard);
    keyboard = nullptr;
  }

  if (pointer) {
    client.release_pointer(pointer);
    pointer = nullptr;
  }

  if (seat) {
    client.release_seat(seat);
    seat = nullptr;
  }

  seat_name = 0;
  keyboard_focused = False;
  values.clear_keyboard();
  values.clear_pointer();
}

auto WaylandInput::destroy() -> void {
  remove_global(seat_name);
  surface = nullptr;
}

// WaylandWindow stores every proxy and callback record for one Window. Its
// process heap allocation keeps the private protocol records at a stable
// address until all proxies have been released.
struct WaylandWindow {
  WaylandWindow(WaylandClient* client, Collector& values, Window& window)
      : client(client), input(*client, values), callback{&window} {}
  WaylandClient* client;
  wl_display* display = nullptr;
  wl_proxy* registry = nullptr;
  wl_proxy* compositor = nullptr;
  wl_proxy* shell = nullptr;
  wl_proxy* shell_surface = nullptr;
  wl_proxy* toplevel = nullptr;
  wl_proxy* surface = nullptr;
  WaylandInput input;
  CallbackContext callback;
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

  auto* client = WaylandClient::create();
  if (!client) {
    failed = True;
    return;
  }

  auto* state = create_native<WaylandWindow>(
      [client, this] { return WaylandWindow(client, input_collector, *this); });
  if (!state) {
    destroy_native(client);
    failed = True;
    return;
  }

  native = state;
  state->display = client->connect();
  connection = state->display;
  if (!state->display) {
    failed = True;
    return;
  }

  state->registry = client->get_registry(state->display);
  if (!state->registry) {
    failed = True;
    return;
  }

  // The initial registry roundtrip discovers the compositor, shell and input
  // devices required before assigning a surface role. Callbacks retain this
  // immovable Window through native teardown.
  static const auto registry_dispatch =
      +[](const void* context, void* target, U32 opcode, const wl_message*,
          wl_argument* arguments) -> S32 {
    enum class Event : U32 { Global, GlobalRemove };
    auto* window = &callback_value<Window>(context);
    switch (Event(opcode)) {
    case Event::Global: {
      auto* registry = Data::cast<wl_proxy>(target);
      U32 name = arguments[0].u;
      const char* interface = arguments[1].s;
      U32 version = arguments[2].u;
      auto* state = Data::cast<WaylandWindow>(window->native);
      auto& client = *state->client;
      state->input.register_global(registry, name, interface, version);
      if (!strcmp(interface, client.compositor_interface->name)) {
        state->compositor = client.bind(
            registry, name, client.compositor_interface,
            version < 6 ? version : 6);
      } else if (
          !strcmp(interface, client.shell_interface.name) && !state->shell) {
        state->shell = client.bind(registry, name, &client.shell_interface, 1);
        if (!state->shell) {
          window->failed = True;
          return 0;
        }

        static const auto shell_dispatch =
            +[](const void* context, void* target, U32 opcode,
                const wl_message*, wl_argument* arguments) -> S32 {
          enum class Event : U32 { Ping };
          switch (Event(opcode)) {
          case Event::Ping: {
            auto* window = &callback_value<Window>(context);
            auto* state = Data::cast<WaylandWindow>(window->native);
            state->client->marshal(
                Data::cast<wl_proxy>(target), 3, nullptr, 0, arguments[0].u);
            return 0;
          }
          }

          return -1;
        };

        if (client.add_dispatcher(
                state->shell, shell_dispatch, &state->callback, window)) {
          client.destroy_request(state->shell, 0);
          state->shell = nullptr;
          window->failed = True;
        }
      }

      return 0;
    }

    case Event::GlobalRemove: {
      U32 name = arguments[0].u;
      Data::cast<WaylandWindow>(window->native)->input.remove_global(name);
      return 0;
    }
    }

    return -1;
  };

  if (client->add_dispatcher(
          state->registry, registry_dispatch, &state->callback, this) ||
      client->roundtrip(state->display) < 0 || failed || !state->compositor ||
      !state->shell) {
    failed = True;
    return;
  }

  state->surface = client->create_surface(state->compositor);
  surface = state->surface;
  if (!state->surface) {
    failed = True;
    return;
  }

  static const auto surface_dispatch =
      +[](const void* context, void* target, U32 opcode, const wl_message*,
          wl_argument* arguments) -> S32 {
    enum class Event : U32 {
      Enter,
      Leave,
      PreferredBufferScale,
      PreferredBufferTransform
    };

    auto* window = &callback_value<Window>(context);
    switch (Event(opcode)) {
    case Event::PreferredBufferScale: {
      S32 factor = arguments[0].i;
      if (factor > 0 && U32(factor) != window->scale) {
        window->scale = U32(factor);
        window->needs_resize = True;

        auto* state = Data::cast<WaylandWindow>(window->native);
        state->client->set_buffer_scale(Data::cast<wl_proxy>(target), factor);
      }

      return 0;
    }

    case Event::Enter:
    case Event::Leave:
    case Event::PreferredBufferTransform:
      return 0;
    }

    return -1;
  };

  if (client->add_dispatcher(
          state->surface, surface_dispatch, &state->callback, this)) {
    failed = True;
    return;
  }

  // A desktop window is one wl_surface carrying one xdg_toplevel role. The
  // protocol objects stay in the same native state as their callbacks and are
  // retired in reverse role order before the wl_surface and display.

  state->shell_surface = client->marshal(
      state->shell, 2, &photophore_xdg_surface_interface, 0, state->surface);
  if (!state->shell_surface) {
    failed = True;
    return;
  }

  static const auto shell_surface_dispatch =
      +[](const void* context, void* target, U32 opcode, const wl_message*,
          wl_argument* arguments) -> S32 {
    enum class Event : U32 { Configure };
    auto* window = &callback_value<Window>(context);
    switch (Event(opcode)) {
    case Event::Configure:
      auto* state = Data::cast<WaylandWindow>(window->native);
      state->client->marshal(
          Data::cast<wl_proxy>(target), 4, nullptr, 0, arguments[0].u);
      state->client->commit(state->surface);
      return 0;
    }

    return -1;
  };

  if (client->add_dispatcher(
          state->shell_surface, shell_surface_dispatch, &state->callback,
          this)) {
    failed = True;
    return;
  }

  state->toplevel = client->marshal(
      state->shell_surface, 1, &photophore_xdg_toplevel_interface, 0);
  if (!state->toplevel) {
    failed = True;
    return;
  }

  static const auto toplevel_dispatch =
      +[](const void* context, void*, U32 opcode, const wl_message*,
          wl_argument* arguments) -> S32 {
    enum class Event : U32 { Configure, Close };
    auto* window = &callback_value<Window>(context);
    switch (Event(opcode)) {
    case Event::Configure:
      configure(
          *window,
          arguments[0].i > 0 ? U32(arguments[0].i) : window->initial_width,
          arguments[1].i > 0 ? U32(arguments[1].i) : window->initial_height);
      return 0;
    case Event::Close:
      window->close_requested = True;
      return 0;
    }

    return -1;
  };

  if (client->add_dispatcher(
          state->toplevel, toplevel_dispatch, &state->callback, this)) {
    failed = True;
    return;
  }

  client->marshal(state->toplevel, 2, nullptr, 0, title);
  client->marshal(state->toplevel, 3, nullptr, 0, "photophore");
  if (!state->input.attach(state->surface)) {
    failed = True;
    return;
  }

  client->commit(state->surface);
  if (client->roundtrip(state->display) < 0) {
    failed = True;
  }
}

Window::~Window() {
  auto* state = Data::cast<WaylandWindow>(native);
  if (!state) {
    return;
  }

  // Input listeners and shell roles refer to the surface and connection.
  // Releasing them first preserves every callback target through proxy
  // teardown.

  auto* owned_client = state->client;
  auto& client = *owned_client;
  state->input.destroy();
  if (state->toplevel) {
    client.destroy_request(state->toplevel, 0);
  }

  if (state->shell_surface) {
    client.destroy_request(state->shell_surface, 0);
  }

  if (state->shell) {
    client.destroy_request(state->shell, 0);
  }

  if (state->surface) {
    client.destroy_request(state->surface, 0);
  }

  if (state->compositor) {
    client.destroy_proxy(state->compositor);
  }

  if (state->registry) {
    client.destroy_proxy(state->registry);
  }

  if (state->display) {
    client.disconnect(state->display);
  }

  destroy_native(state);
  destroy_native(owned_client);
}

auto Window::poll_events() -> Window::EventStatus {
  EventStatus status = get_event_status();
  if (status != EventStatus::Ready) {
    return status;
  }

  auto* state = Data::cast<WaylandWindow>(native);
  auto& client = *state->client;
  wl_display* display = state->display;
  if (client.dispatch_pending(display) < 0) {
    failed = True;
    return EventStatus::Failed;
  }

  if (close_requested) {
    return EventStatus::Closed;
  }

  // Wayland permits a socket read only after its pending queue is drained.
  // A successful prepare_read reserves that read, so every later path either
  // reads events or cancels the reservation before returning.
  while (client.prepare_read(display) != 0) {
    if (client.dispatch_pending(display) < 0) {
      failed = True;
      return EventStatus::Failed;
    }

    if (close_requested) {
      return EventStatus::Closed;
    }
  }

  if (client.flush(display) < 0 && errno != EAGAIN) {
    client.cancel_read(display);
    failed = True;
    return EventStatus::Failed;
  }

  // A zero timeout makes each call one event observation for the application
  // frame loop. The application remains responsible for its broader scheduling
  // and compositor wait policy.
  pollfd display_fd = {
    client.get_fd(display),
    POLLIN,
    0,
  };

  auto poll_result = 0;
  do {
    poll_result = poll(&display_fd, 1, 0);
  } while (poll_result < 0 && errno == EINTR);

  if (poll_result < 0) {
    client.cancel_read(display);
    failed = True;
    return EventStatus::Failed;
  }

  if (display_fd.revents & (POLLERR | POLLHUP | POLLNVAL)) {
    client.cancel_read(display);
    failed = True;
    return EventStatus::Failed;
  }

  if (poll_result > 0 && (display_fd.revents & POLLIN) != 0) {
    S32 events_read = client.read_events(display);
    if (events_read < 0) {
      failed = True;
      return EventStatus::Failed;
    }
  } else {
    client.cancel_read(display);
  }

  S32 events_dispatched = client.dispatch_pending(display);
  if (events_dispatched < 0) {
    failed = True;
    return EventStatus::Failed;
  }

  input_snapshot = input_collector.collect(input_mapping);
  return close_requested ? EventStatus::Closed : EventStatus::Ready;
}

auto Window::get_presentation() const -> Presentation {
  return Presentation::create(Presentation::Kind::Wayland, connection, surface);
}

#elifdef TOOLCHAIN_WINDOWS

// Each Window pairs its registration with its HWND. Destruction releases the
// native window first, then unregisters the class after User32 retires
// callbacks.
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
  if (width > S32(U32(-1) >> 1) || height > S32(U32(-1) >> 1)) {
    failed = True;
    return;
  }

  const S32 length = MultiByteToWideChar(
      CP_UTF8, MB_ERR_INVALID_CHARS, title ? title : "", -1, nullptr, 0);
  if (!length) {
    failed = True;
    return;
  }

  Thread::Vector<wchar_t> text;
  text.resize(length);
  if (!MultiByteToWideChar(
          CP_UTF8, MB_ERR_INVALID_CHARS, title ? title : "", -1,
          text.get_data(), length)) {
    failed = True;
    return;
  }

  auto* state = create_native<Win32Window>([] { return Win32Window(); });
  if (!state) {
    failed = True;
    return;
  }

  native = state;
  connection = GetModuleHandleW(nullptr);

  wchar_t class_name[64];
  swprintf(class_name, 64, L"Photophore.%p", Data::cast<void>(this));

  WNDCLASSEXW descriptor = {};
  descriptor.cbSize = sizeof(descriptor);
  descriptor.hInstance = Data::bit_cast<HINSTANCE>(connection);
  descriptor.lpszClassName = class_name;
  descriptor.hCursor = LoadCursorW(nullptr, IDC_ARROW);
  descriptor.lpfnWndProc = [](HWND handle, UINT message, WPARAM wparam,
                              LPARAM lparam) -> LRESULT {
    auto* window =
        Data::bit_cast<Window*>(GetWindowLongPtrW(handle, GWLP_USERDATA));
    // CreateWindowEx sends messages before it returns. Install the receiver
    // during WM_NCCREATE so those early callbacks already reach this Window.
    if (message == WM_NCCREATE) {
      auto* creation = Data::bit_cast<CREATESTRUCTW*>(lparam);
      window = Data::cast<Window>(creation->lpCreateParams);
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

  state->registration = RegisterClassExW(&descriptor);
  if (!state->registration) {
    failed = True;
    return;
  }

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
        0, MAKEINTATOM(state->registration), text.get_data(),
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, S32(outer_width),
        S32(outer_height), nullptr, nullptr,
        Data::bit_cast<HINSTANCE>(connection), this);
  }

  if (awareness) {
    SetThreadDpiAwarenessContext(awareness);
  }

  if (!surface) {
    failed = True;
    return;
  }

  ShowWindow(Data::bit_cast<HWND>(surface), SW_SHOW);

  RECT client;
  if (GetClientRect(Data::bit_cast<HWND>(surface), &client)) {
    configure(*this, U32(client.right), U32(client.bottom));
  }
}

Window::~Window() {
  if (surface) {
    DestroyWindow(Data::bit_cast<HWND>(surface));
  }

  auto* state = Data::cast<Win32Window>(native);
  if (state && state->registration) {
    UnregisterClassW(
        MAKEINTATOM(state->registration),
        Data::bit_cast<HINSTANCE>(connection));
  }

  destroy_native(state);
}

auto Window::poll_events() -> EventStatus {
  if (get_event_status() != EventStatus::Ready) {
    return get_event_status();
  }

  // Only this window's messages contribute to its observation. WM_QUIT is
  // thread state, so preserve it for the application pump after marking this
  // window closed.

  MSG message;
  while (
      PeekMessageW(&message, Data::bit_cast<HWND>(surface), 0, 0, PM_REMOVE)) {
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

auto Window::get_presentation() const -> Presentation {
  return Presentation::create(Presentation::Kind::Win32, connection, surface);
}

#else

// Browser builds retain the common Window interface as an unavailable native
// presenter. The ordinary failure observation lets applications select a web
// presentation path at their integration boundary.
Window::Window(U32 width, U32 height, const char*) {
  logical_width = initial_width = width;
  logical_height = initial_height = height;
  failed = True;
}

Window::~Window() = default;
auto Window::poll_events() -> EventStatus {
  return get_event_status();
}

auto Window::get_presentation() const -> Presentation {
  return Presentation::create(Presentation::Kind::None, nullptr, nullptr);
}

#endif

// Host branches update this compact shared state from native
// callbacks. Common accessors keep event status, extent and input observations
// consistent across Wayland, Win32 and browser builds.
auto Window::get_event_status() const -> Window::EventStatus {
  if (failed || !connection) {
    return EventStatus::Failed;
  }

  return close_requested ? EventStatus::Closed : EventStatus::Ready;
}

auto Window::get_logical_width() const -> U32 {
  return logical_width;
}

auto Window::get_logical_height() const -> U32 {
  return logical_height;
}

auto Window::get_scale() const -> U32 {
  return scale;
}

auto Window::get_needs_resize() const -> Bool {
  return needs_resize;
}

auto Window::clear_resize() -> void {
  needs_resize = False;
}

auto Window::get_input() const -> const Snapshot& {
  return input_snapshot;
}

auto Window::get_input_mapping() -> Mapping& {
  return input_mapping;
}

auto Window::get_input_mapping() const -> const Mapping& {
  return input_mapping;
}
