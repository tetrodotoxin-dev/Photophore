// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "toolchain/toolchain.hpp"

namespace Photophore::Image {

// Sampler2D stores the addressing and filtering rules for unit Texture2D
// coordinates. Addressing::Zero returns a transparent border,
// Addressing::Clamp extends edge texels and Addressing::Wrap repeats the image.
// Filtering selects linear interpolation or the nearest texel.
//
// `create` preserves any declared Addressing and Filtering pair. Its default
// arguments select a transparent border with linear interpolation. Equality
// compares the two U8 fields, so one Pixels2D value can participate in distinct
// Texture2D sampling contracts without another pixel allocation.
class Sampler2D {
 public:
  enum class Addressing : U8 {
    Zero,
    Clamp,
    Wrap,
  };

  enum class Filtering : U8 {
    Linear,
    Nearest,
  };

  static constexpr auto create(
      Addressing addressing = Addressing::Zero,
      Filtering filtering = Filtering::Linear) -> Sampler2D {
    return Sampler2D(addressing, filtering);
  }

  constexpr auto get_addressing() const -> Addressing { return addressing; }
  constexpr auto get_filtering() const -> Filtering { return filtering; }

  constexpr auto operator==(Sampler2D other) const -> Bool {
    return addressing == other.addressing && filtering == other.filtering;
  }

 private:
  constexpr Sampler2D(Addressing addressing, Filtering filtering)
      : addressing(addressing), filtering(filtering) {}

  Addressing addressing = Addressing::Zero;
  Filtering filtering = Filtering::Linear;
};

static_assert(sizeof(Sampler2D) == sizeof(U8) * 2);
static_assert(alignof(Sampler2D) == alignof(U8));
static_assert(__is_trivially_copyable(Sampler2D));
static_assert(__is_standard_layout(Sampler2D));

}  // namespace Photophore::Image
