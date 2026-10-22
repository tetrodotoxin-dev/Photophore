// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/math/point.hpp"
#include "photophore/frame/transform.hpp"

namespace Photophore::Scene {

// Transform2D keeps translation, scale and rotation as editable application
// values. to_frame captures them as affine coefficients for Frame::Batch,
// preserving the observed placement when the application later edits this
// value. Rotation uses radians and applies after scale.
struct Transform2D {
  auto to_frame() const -> Frame::Transform {
    return Frame::Transform::create(
        translation[0], translation[1], scale_x, scale_y, rotation);
  }

  Perimortem::Math::Point<R64, 2> translation;
  R64 scale_x = 1.0;
  R64 scale_y = 1.0;
  R64 rotation = 0.0;
};

static_assert(
    sizeof(Transform2D) ==
    sizeof(Perimortem::Math::Point<R64, 2>) + sizeof(R64) * 3);
static_assert(__is_standard_layout(Transform2D));

}  // namespace Photophore::Scene
