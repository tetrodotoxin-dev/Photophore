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
#include "photophore/image/sampler_2d.hpp"

namespace Photophore::Image {

// Texture2D is the retained CPU value consumed by Frame::Batch. Construction
// moves an Rgba8 Matrix into shared storage and pairs it with one Sampler2D.
// Copies keep that exact matrix identity alive, which lets Pipelines upload it
// once and cache separate descriptor sets for different sampling policies.
// Matrix row zero maps to texture coordinate Y zero and appears along the top
// edge of unit quad geometry.
//
// The default value represents an unconfigured texture through an empty image
// reservation. get_image exposes a const borrow from a configured value.
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

  // The returned matrix remains valid through this Texture2D reservation.
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

}  // namespace Photophore::Image
