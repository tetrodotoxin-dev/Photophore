// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/graphics/point_2d.hpp"
#include "photophore/frame/transform.hpp"

namespace Photophore {

// Placement retains translation, scale and rotation as separate editable
// values. to_frame converts them to affine coefficients when collecting a draw,
// so subsequent edits cannot change a batch that has already been collected.
// Rotation is measured in radians, with scale applied before rotation.
struct Transform2D {
  auto to_frame() const -> Frame::Transform {
    return Frame::Transform::create(
        translation.x, translation.y, scale_x, scale_y, rotation);
  }

  Perimortem::Graphics::Point2D translation;
  R64 scale_x = 1.0;
  R64 scale_y = 1.0;
  R64 rotation = 0.0;
};

static_assert(
    sizeof(Transform2D) ==
    sizeof(Perimortem::Graphics::Point2D) + sizeof(R64) * 3);
static_assert(__is_standard_layout(Transform2D));

}  // namespace Photophore
