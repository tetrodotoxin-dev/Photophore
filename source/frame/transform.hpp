// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/perimortem.hpp"

namespace Photophore::Frame {

// Frame collection reduces placement to a two dimensional affine transform.
// For input coordinates (u, v), the rows produce xx*u + xy*v + x and
// yx*u + yy*v + y. Rotation is expressed in radians and
// create applies scale before rotation and translation.
//
// compose applies the local transform first and the parent second. Keeping the
// result as copied coefficients makes a completed batch independent of later
// changes to the source hierarchy.
class Transform {
 public:
  constexpr Transform() = default;

  static auto create(
      R64 translation_x,
      R64 translation_y,
      R64 scale_x,
      R64 scale_y,
      R64 rotation) -> Transform;

  static auto compose(const Transform& parent, const Transform& local)
      -> Transform;

  constexpr auto get_xx() const -> R64 { return xx; }
  constexpr auto get_xy() const -> R64 { return xy; }
  constexpr auto get_yx() const -> R64 { return yx; }
  constexpr auto get_yy() const -> R64 { return yy; }
  constexpr auto get_x() const -> R64 { return x; }
  constexpr auto get_y() const -> R64 { return y; }

  constexpr auto operator==(const Transform& rhs) const -> Bool {
    return xx == rhs.xx && xy == rhs.xy && yx == rhs.yx && yy == rhs.yy &&
           x == rhs.x && y == rhs.y;
  }

 private:
  constexpr Transform(R64 xx, R64 xy, R64 yx, R64 yy, R64 x, R64 y)
      : xx(xx), xy(xy), yx(yx), yy(yy), x(x), y(y) {}

  R64 xx = 1.0;
  R64 xy = 0.0;
  R64 yx = 0.0;
  R64 yy = 1.0;
  R64 x = 0.0;
  R64 y = 0.0;
};

}  // namespace Photophore::Frame
