// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/perimortem.hpp"

namespace Photophore::Image {

// Srgba8 stores one sRGB color and linear alpha in red, green, blue, alpha
// byte order. The color channels contain sRGB encoded samples; alpha contains
// coverage and receives no transfer function. Construction preserves the
// supplied samples without performing color conversion. A producer converting
// another image format therefore establishes that its color values use sRGB.
//
// Public channels expose the exact tightly packed representation uploaded by
// Photophore's current sampled image capability. Missing alpha is fully opaque,
// while default construction produces transparent black.
class Srgba8 {
 public:
  constexpr Srgba8() = default;

  static constexpr auto from_grey(U8 grey) -> Srgba8 {
    return from_rgba(grey, grey, grey, opaque);
  }

  static constexpr auto from_grey_alpha(U8 grey, U8 alpha) -> Srgba8 {
    return from_rgba(grey, grey, grey, alpha);
  }

  static constexpr auto from_rgb(U8 red, U8 green, U8 blue) -> Srgba8 {
    return from_rgba(red, green, blue, opaque);
  }

  static constexpr auto from_rgba(U8 red, U8 green, U8 blue, U8 alpha)
      -> Srgba8 {
    Srgba8 result;
    result.red = red;
    result.green = green;
    result.blue = blue;
    result.alpha = alpha;
    return result;
  }

  static constexpr auto get_byte_count() -> Count { return 4; }

  U8 red = 0;
  U8 green = 0;
  U8 blue = 0;
  U8 alpha = 0;

 private:
  static constexpr U8 opaque = 0xFF;
};

static_assert(sizeof(Srgba8) == 4);
static_assert(alignof(Srgba8) == alignof(U8));
static_assert(__is_standard_layout(Srgba8));
static_assert(__is_trivially_copyable(Srgba8));

}  // namespace Photophore::Image
