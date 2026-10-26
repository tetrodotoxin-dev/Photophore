// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#define TOOLCHAIN_PRIVATE_HEADER 1

#include "toolchain/toolchain.h"

#ifdef TOOLCHAIN_LINUX

#include "photophore/platform/wayland/details/abi.hpp"

namespace Photophore::Platform::Wayland::Details::Client {

// Client expresses the core Wayland operations used by Window in terms of the
// local ABI above. Event enums preserve their wire opcodes, and creation always
// uses the interface record exported by the loaded client runtime.
enum class RegistryEvent : U32 { Global, GlobalRemove };
enum class SurfaceEvent : U32 {
  Enter,
  Leave,
  PreferredBufferScale,
  PreferredBufferTransform
};

enum class SeatEvent : U32 { Capabilities, Name };
enum class KeyboardEvent : U32 {
  Keymap,
  Enter,
  Leave,
  Key,
  Modifiers,
  RepeatInfo
};

enum class PointerEvent : U32 {
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

constexpr U32 pointer_capability = 1;
constexpr U32 keyboard_capability = 2;
constexpr U32 pressed = 1;
constexpr U32 horizontal_scroll = 1;

auto connect() -> ABI::Display*;
auto disconnect(ABI::Display* display) -> void;
auto get_registry(ABI::Display* display) -> ABI::Proxy*;
auto bind(
    ABI::Proxy* registry,
    U32 name,
    const ABI::Interface& interface,
    U32 version) -> ABI::Proxy*;
auto select_version(
    U32 advertised,
    const ABI::Interface& interface,
    U32 maximum) -> U32;
auto create_surface(ABI::Proxy* compositor) -> ABI::Proxy*;
auto get_keyboard(ABI::Proxy* seat) -> ABI::Proxy*;
auto get_pointer(ABI::Proxy* seat) -> ABI::Proxy*;
auto listen(
    ABI::Proxy* proxy,
    ABI::Dispatcher* callback,
    const void* callback_context,
    void* context) -> S32;
auto commit(ABI::Proxy* surface) -> void;
auto set_buffer_scale(ABI::Proxy* surface, S32 scale) -> void;
auto release_compositor(ABI::Proxy* compositor) -> void;
auto release_seat(ABI::Proxy* seat) -> void;
auto release_keyboard(ABI::Proxy* keyboard) -> void;
auto release_pointer(ABI::Proxy* pointer) -> void;
auto destroy_surface(ABI::Proxy* surface) -> void;
auto destroy_proxy(ABI::Proxy* proxy) -> void;

auto dispatch_pending(ABI::Display* display) -> S32;
auto prepare_read(ABI::Display* display) -> S32;
auto cancel_read(ABI::Display* display) -> void;
auto read_events(ABI::Display* display) -> S32;
auto flush(ABI::Display* display) -> S32;
auto roundtrip(ABI::Display* display) -> S32;
auto get_descriptor(ABI::Display* display) -> S32;

}  // namespace Photophore::Platform::Wayland::Details::Client

#endif
