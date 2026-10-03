// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/perimortem.hpp"

namespace Photophore {

// Filtering and addressing can vary while the source pixels stay the same.
// Sampler2D carries those choices as a copied value, allowing Pipelines to
// reuse the uploaded image while selecting a distinct Vulkan sampler.
// Zero samples the transparent border outside the image, Clamp extends its edge
// texels, and Wrap repeats it. Linear interpolation is the default filter.
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

  constexpr Sampler2D() = default;
  constexpr Sampler2D(Addressing addressing, Filtering filtering)
      : addressing(addressing), filtering(filtering) {}

  constexpr auto get_addressing() const -> Addressing { return addressing; }
  constexpr auto get_filtering() const -> Filtering { return filtering; }

  constexpr auto operator==(Sampler2D other) const -> Bool {
    return addressing == other.addressing && filtering == other.filtering;
  }

 private:
  Addressing addressing = Addressing::Zero;
  Filtering filtering = Filtering::Linear;
};

static_assert(sizeof(Sampler2D) == sizeof(U8) * 2);
static_assert(alignof(Sampler2D) == alignof(U8));
static_assert(__is_trivially_copyable(Sampler2D));
static_assert(__is_standard_layout(Sampler2D));

}  // namespace Photophore
