// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#define TOOLCHAIN_PRIVATE_HEADER 1

#include "toolchain/toolchain.h"

#ifdef TOOLCHAIN_LINUX

#include "photophore/platform/wayland/details/abi.hpp"

namespace Photophore::Platform::Wayland::Details {

// Client provides the typed core Wayland operations used by the private Window
// provider. Its event enums preserve wire opcodes, while object creation uses
// the interface records exported by the loaded client runtime. Each operation
// acts on libwayland-owned proxies and introduces no second proxy lifetime.
class Client {
 public:
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

  static constexpr U32 pointer_capability = 1;
  static constexpr U32 keyboard_capability = 2;
  static constexpr U32 pressed = 1;
  static constexpr U32 horizontal_scroll = 1;

  static auto connect() -> ABI::Display*;
  static auto disconnect(ABI::Display* display) -> void;
  static auto get_registry(ABI::Display* display) -> ABI::Proxy*;
  static auto bind(
      ABI::Proxy* registry,
      U32 name,
      const ABI::Interface& interface,
      U32 version) -> ABI::Proxy*;
  static auto select_version(
      U32 advertised,
      const ABI::Interface& interface,
      U32 maximum) -> U32;
  static auto create_surface(ABI::Proxy* compositor) -> ABI::Proxy*;
  static auto get_keyboard(ABI::Proxy* seat) -> ABI::Proxy*;
  static auto get_pointer(ABI::Proxy* seat) -> ABI::Proxy*;
  static auto listen(
      ABI::Proxy* proxy,
      ABI::Dispatcher* callback,
      const void* callback_context,
      void* context) -> S32;
  static auto commit(ABI::Proxy* surface) -> void;
  static auto set_buffer_scale(ABI::Proxy* surface, S32 scale) -> void;
  static auto release_compositor(ABI::Proxy* compositor) -> void;
  static auto release_seat(ABI::Proxy* seat) -> void;
  static auto release_keyboard(ABI::Proxy* keyboard) -> void;
  static auto release_pointer(ABI::Proxy* pointer) -> void;
  static auto destroy_surface(ABI::Proxy* surface) -> void;
  static auto destroy_proxy(ABI::Proxy* proxy) -> void;

  static auto dispatch_pending(ABI::Display* display) -> S32;
  static auto prepare_read(ABI::Display* display) -> S32;
  static auto cancel_read(ABI::Display* display) -> void;
  static auto read_events(ABI::Display* display) -> S32;
  static auto flush(ABI::Display* display) -> S32;
  static auto roundtrip(ABI::Display* display) -> S32;
  static auto get_descriptor(ABI::Display* display) -> S32;
};

}  // namespace Photophore::Platform::Wayland::Details

#endif
