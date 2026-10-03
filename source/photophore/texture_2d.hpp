// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/static/vector.hpp"
#include "perimortem/core/data.hpp"
#include "perimortem/core/option.hpp"

#include "perimortem/memory/dynamic/record.hpp"
#include "perimortem/memory/matrix.hpp"

#include "perimortem/serialization/rgba8.hpp"

#include "perimortem/math/size.hpp"
#include "photophore/sampler_2d.hpp"

namespace Photophore {

// A sampled image needs to survive every frame that uses it. Texture2D pairs
// an explicitly shared, immutable pixel matrix with its sampling policy. Copies
// retain that matrix without copying pixels, while other textures can use the
// same matrix with a different sampler. An unconfigured texture allocates
// nothing.
class Texture2D {
 public:
  Texture2D() = default;
  explicit Texture2D(
      Perimortem::Memory::Matrix<Perimortem::Serialization::Rgba8>&& image,
      Sampler2D sampler = Sampler2D())
      : image(
            Perimortem::Memory::Dynamic::Record<
                Perimortem::Memory::Matrix<Perimortem::Serialization::Rgba8>>(
                Perimortem::Core::Data::take(image))),
        sampler(sampler) {}

  Texture2D(const Texture2D& image, Sampler2D sampler)
      : image(image.image), sampler(sampler) {}

  // Requires a configured texture. The observation borrows its retained matrix.
  auto get_image() const
      -> const Perimortem::Memory::Matrix<Perimortem::Serialization::Rgba8>& {
    return **image;
  }
  constexpr auto get_sampler() const -> Sampler2D { return sampler; }
  auto get_size_pixels() const -> Perimortem::Math::Size<U32, 2> {
    if (!image) {
      return {};
    }

    Perimortem::Core::Static::Vector<U32, 2> dimensions = {{
      get_image().get_column_count(),
      get_image().get_row_count(),
    }};
    return Perimortem::Math::Size<U32, 2>(dimensions);
  }

  auto is_empty() const -> Bool { return !image; }
  auto is_drawable() const -> Bool { return image && !get_image().is_empty(); }

  auto matches(const Texture2D& other) const -> Bool {
    return image && other.image && &get_image() == &other.get_image() &&
           sampler == other.sampler;
  }

  auto get_reservations() const -> Count {
    return image ? image->get_reservations() : 0;
  }

 private:
  Perimortem::Core::Option<Perimortem::Memory::Dynamic::Record<
      Perimortem::Memory::Matrix<Perimortem::Serialization::Rgba8>>>
      image;
  Sampler2D sampler;
};

}  // namespace Photophore
