// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/option.hpp"

#include "perimortem/memory/dynamic/record.hpp"

#include "perimortem/graphics/image.hpp"
#include "photophore/sampler_2d.hpp"

namespace Photophore {

// A sampled image needs to survive every frame that uses it. Texture2D pairs
// an explicitly shared, immutable Image with its sampling policy. Copies retain
// that Image without copying pixels, while other textures can use the same
// Image with a different sampler. An unconfigured texture allocates nothing.
class Texture2D {
 public:
  Texture2D() = default;
  explicit Texture2D(
      const Perimortem::Memory::Dynamic::Record<
          const Perimortem::Graphics::Image>& image,
      Sampler2D sampler = Sampler2D())
      : image(image), sampler(sampler) {}

  // Requires a configured texture. The observation borrows its retained Image.
  auto get_image() const -> const Perimortem::Graphics::Image& {
    return **image;
  }
  constexpr auto get_sampler() const -> Sampler2D { return sampler; }
  auto get_size_pixels() const -> Perimortem::Graphics::Size2D {
    return image ? get_image().get_size_pixels()
                 : Perimortem::Graphics::Size2D();
  }

  auto is_empty() const -> Bool { return !image; }
  auto is_drawable() const -> Bool {
    return image && get_image().is_drawable();
  }

  auto matches(const Texture2D& other) const -> Bool {
    return image && other.image && &get_image() == &other.get_image() &&
           sampler == other.sampler;
  }

  auto get_reservations() const -> Count {
    return image ? image->get_reservations() : 0;
  }

 private:
  Perimortem::Core::Option<
      Perimortem::Memory::Dynamic::Record<const Perimortem::Graphics::Image>>
      image;
  Sampler2D sampler;
};

}  // namespace Photophore
