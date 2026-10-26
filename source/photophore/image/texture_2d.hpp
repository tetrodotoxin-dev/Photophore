// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/math/size.hpp"

#include "photophore/image/pixels_2d.hpp"
#include "photophore/image/sampler_2d.hpp"

namespace Photophore::Image {

// Texture2D pairs retained pixel content with the policy used to sample it.
// Pixels2D governs content identity and lifetime, while Sampler2D independently
// selects addressing and filtering. Copies preserve both values so a Batch can
// keep its complete sampled image contract through command recording.
//
// Every Texture2D contains a Pixels2D value. Optional image state belongs in an
// explicit Option at the application boundary rather than an empty Texture2D.
class Texture2D {
 public:
  static auto create(Pixels2D content, Sampler2D sampler = Sampler2D::create())
      -> Texture2D {
    return Texture2D(content, sampler);
  }

  constexpr auto get_content() const -> const Pixels2D& { return content; }
  constexpr auto get_sampler() const -> Sampler2D { return sampler; }
  auto get_size_pixels() const -> Perimortem::Core::Math::Size<U32, 2> {
    return content.get_size_pixels();
  }

  auto is_drawable() const -> Bool { return !content.is_empty(); }

  // Equality requires one content identity and the same complete sampling
  // policy, matching the native image and descriptor realization boundary.
  auto operator==(const Texture2D& other) const -> Bool {
    return content == other.content && sampler == other.sampler;
  }

 private:
  constexpr Texture2D(Pixels2D content, Sampler2D sampler)
      : content(content), sampler(sampler) {}

  Pixels2D content;
  Sampler2D sampler;
};

}  // namespace Photophore::Image
