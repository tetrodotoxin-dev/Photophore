// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "photophore/frame/transform.hpp"

// create stores a two dimensional affine matrix as two rows. Multiplying scale
// into the rotation columns preserves the documented scale, rotate, translate
// application order for unit quad coordinates.
auto Photophore::Frame::Transform::create(
    R64 translation_x,
    R64 translation_y,
    R64 scale_x,
    R64 scale_y,
    R64 rotation) -> Transform {
  R64 cosine = __builtin_cos(rotation);
  R64 sine = __builtin_sin(rotation);
  return Transform(
      cosine * scale_x, -sine * scale_y, sine * scale_x, cosine * scale_y,
      translation_x, translation_y);
}

auto Photophore::Frame::Transform::compose(
    const Transform& parent,
    const Transform& local) -> Transform {
  // Matrix multiplication places local coordinates first and then carries the
  // result through the parent, including translation in the same operation.
  return Transform(
      parent.xx * local.xx + parent.xy * local.yx,
      parent.xx * local.xy + parent.xy * local.yy,
      parent.yx * local.xx + parent.yy * local.yx,
      parent.yx * local.xy + parent.yy * local.yy,
      parent.xx * local.x + parent.xy * local.y + parent.x,
      parent.yx * local.x + parent.yy * local.y + parent.y);
}
