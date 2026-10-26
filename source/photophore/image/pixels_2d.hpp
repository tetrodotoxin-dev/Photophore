// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/data.hpp"
#include "perimortem/core/math/size.hpp"
#include "perimortem/core/storage/vector.hpp"
#include "perimortem/memory/dynamic/record.hpp"
#include "perimortem/memory/matrix.hpp"

#include "photophore/image/srgba8.hpp"

namespace Photophore::Image {

// Pixels2D retains one row major Srgba8 Matrix independently of how a renderer
// samples it. Copies identify and keep the same Matrix accessible, allowing a
// backend to upload the content once for several Texture2D sampling policies.
// Matrix row zero maps to texture coordinate Y zero and appears along the top
// edge of Photophore's unit quad geometry.
//
// `create` moves the Matrix into a shared Record allocated by the current
// thread's Bibliotheca. Copies and their final release stay on that thread. An
// empty Matrix remains valid content with no drawable pixels.
class Pixels2D {
 public:
  static auto create(Perimortem::Memory::Matrix<Srgba8>&& pixels) -> Pixels2D {
    return Pixels2D(Perimortem::Core::Data::take(pixels));
  }

  auto get_pixels() const -> const Perimortem::Memory::Matrix<Srgba8>& {
    return *pixels;
  }

  auto get_size_pixels() const -> Perimortem::Core::Math::Size<U32, 2> {
    Perimortem::Core::Storage::Vector<U32, 2> dimensions = {{
      pixels->get_column_count(),
      pixels->get_row_count(),
    }};

    return Perimortem::Core::Math::Size<U32, 2>(dimensions);
  }

  auto is_empty() const -> Bool { return pixels->is_empty(); }

  // Equality identifies the same retained Matrix. Independently created
  // matrices remain distinct even when every dimension and sample matches.
  auto operator==(const Pixels2D& other) const -> Bool {
    return &*pixels == &*other.pixels;
  }

 private:
  Pixels2D(Perimortem::Memory::Matrix<Srgba8>&& source)
      : pixels(Perimortem::Core::Data::take(source)) {}

  Perimortem::Memory::Dynamic::Record<Perimortem::Memory::Matrix<Srgba8>>
      pixels;
};

}  // namespace Photophore::Image
