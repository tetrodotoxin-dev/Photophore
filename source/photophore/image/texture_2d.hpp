// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/static/vector.hpp"
#include "perimortem/core/data.hpp"
#include "perimortem/core/option.hpp"

#include "perimortem/memory/dynamic/record.hpp"
#include "perimortem/memory/matrix.hpp"

#include "perimortem/math/size.hpp"
#include "photophore/image/sampler_2d.hpp"
#include "photophore/image/srgba8.hpp"

namespace Photophore::Image {

// Texture2D retains one CPU image and its sampling policy for Frame::Batch.
// Construction moves an Srgba8 Matrix into a shared Record. Copies retain that
// exact Matrix, which lets Pipelines upload it once and realize separate
// descriptor sets for different Sampler2D values. Matrix row zero maps to
// texture coordinate Y zero and appears along the top edge of unit quad
// geometry.
//
// The default value has no image. `get_image` requires a configured Texture2D
// and returns a reference valid while this value retains the shared Record.
// Record allocation and release stay on the constructing thread.
class Texture2D {
 public:
  Texture2D() = default;
  explicit Texture2D(
      Perimortem::Memory::Matrix<Srgba8>&& image,
      Sampler2D sampler = Sampler2D())
      : image(
            Perimortem::Memory::Dynamic::Record<
                Perimortem::Memory::Matrix<Srgba8>>(
                Perimortem::Core::Data::take(image))),
        sampler(sampler) {}

  Texture2D(const Texture2D& image, Sampler2D sampler)
      : image(image.image), sampler(sampler) {}

  // The returned Matrix remains valid while this Texture2D remains accessible.
  auto get_image() const -> const Perimortem::Memory::Matrix<Srgba8>& {
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
  Perimortem::Core::Option<
      Perimortem::Memory::Dynamic::Record<Perimortem::Memory::Matrix<Srgba8>>>
      image;
  Sampler2D sampler;
};

}  // namespace Photophore::Image
