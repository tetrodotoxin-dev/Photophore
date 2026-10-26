// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "photophore/frame/transform.hpp"

#include "toolchain/validation/test.hpp"

using namespace Photophore::Frame;
using namespace Toolchain::Validation;

static Harness FrameTransform = {
  .name = "Photophore::Frame::Transform",
};

VALIDATION_TEST(FrameTransform, create) {
  const Transform transform = Transform::create(11.0, 13.0, 2.0, 3.0, 0.0);
  EXPECT(transform.get_xx() == 2.0);
  EXPECT(transform.get_xy() == 0.0);
  EXPECT(transform.get_yx() == 0.0);
  EXPECT(transform.get_yy() == 3.0);
  EXPECT(transform.get_x() == 11.0);
  EXPECT(transform.get_y() == 13.0);
}

// Composition applies local coordinates first. Parent scale consequently also
// scales the local translation before adding the parent's translation.
VALIDATION_TEST(FrameTransform, compose) {
  const Transform parent = Transform::create(10.0, 20.0, 2.0, 3.0, 0.0);
  const Transform local = Transform::create(4.0, 5.0, 7.0, 11.0, 0.0);
  const Transform composed = Transform::compose(parent, local);
  EXPECT(composed.get_xx() == 14.0);
  EXPECT(composed.get_xy() == 0.0);
  EXPECT(composed.get_yx() == 0.0);
  EXPECT(composed.get_yy() == 33.0);
  EXPECT(composed.get_x() == 18.0);
  EXPECT(composed.get_y() == 35.0);
}
