// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "toolchain/toolchain.hpp"

namespace Photophore::Platform {

// Presentation identifies the native window surface used to create a rendering
// target. Kind selects the matching platform representation. Wayland publishes
// its display and surface handles, while Win32 publishes its module and window
// handles. The record keeps native declarations outside the public Window and
// renderer headers.
//
// Window supplies the lifetime of both handles and remains live through the
// Vulkan surface created from them. Copies preserve the same borrowed handles.
// Kind::None represents an unavailable presentation path.
class Presentation {
 public:
  enum class Kind : U64 {
    None,
    Wayland,
    Win32,
  };

  static constexpr auto create(Kind kind, void* host, void* surface)
      -> Presentation {
    return Presentation(kind, host, surface);
  }

  constexpr auto get_kind() const -> Kind { return kind; }
  constexpr auto get_host() const -> void* { return host; }
  constexpr auto get_surface() const -> void* { return surface; }

  constexpr auto is_valid() const -> Bool {
    return Bool(kind != Kind::None && host && surface);
  }

 private:
  constexpr Presentation(Kind kind, void* host, void* surface)
      : kind(kind), host(host), surface(surface) {}

  Kind kind = Kind::None;
  void* host = nullptr;
  void* surface = nullptr;
};

static_assert(sizeof(Presentation) == sizeof(U64) + sizeof(void*) * 2);
static_assert(__is_trivially_copyable(Presentation));
static_assert(__is_standard_layout(Presentation));

}  // namespace Photophore::Platform
