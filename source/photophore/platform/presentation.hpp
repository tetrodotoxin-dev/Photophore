// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/perimortem.hpp"

namespace Photophore::Platform {

// A Vulkan surface needs native handles, but renderer callers do not need to
// include every window system's headers. Presentation carries the host tag and
// two borrowed handles to the selected surface implementation. Wayland uses a
// wl_display and wl_surface, while Win32 uses an HINSTANCE and HWND.
//
// Copying this record does not retain either handle. Its Window must outlive
// the Vulkan surface made from it. Kind::None represents an unavailable
// presenter, and each host adapter accepts only the kind it knows how to
// interpret.
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
