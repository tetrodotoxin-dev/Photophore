// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "perimortem/core/static/vector.hpp"

#include "perimortem/memory/dynamic/vector.hpp"
#include "perimortem/memory/matrix.hpp"

#include "perimortem/serialization/rgba8.hpp"

#include "perimortem/math/domain.hpp"
#include "perimortem/math/point.hpp"
#include "perimortem/math/size.hpp"
#include "photophore/sprite.hpp"
#include "photophore/texture_2d.hpp"
#include "toolchain/validation/unit_test.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Math;
using namespace Perimortem::Memory;
using namespace Perimortem::Serialization;
using namespace Toolchain::Validation;
using namespace Photophore;

static Harness GraphicsObjects = {
  .name = "Photophore::Objects",
};

static auto make_size(U32 width, U32 height) -> Size<U32, 2> {
  Static::Vector<U32, 2> dimensions = {{width, height}};
  return Size<U32, 2>(dimensions);
}

static auto one_pixel_image() -> Matrix<Rgba8> {
  Dynamic::Vector<Rgba8> pixels;
  pixels.emplace(Rgba8::from_rgba(0x12, 0x34, 0x56, 0x78));
  return Matrix<Rgba8>::create(
      Domain<2, Boundary::Clip, Boundary::Clip>(make_size(1, 1)),
      pixels.get_view());
}

VALIDATION_TEST(GraphicsObjects, sprite_values) {
  Sprite sprite;
  EXPECT_NOT(sprite.is_drawable());
  EXPECT(sprite.is_visible());
  EXPECT_EQ(sprite.get_z_index(), S64(0));

  Texture2D texture(one_pixel_image());
  sprite.set_texture(texture);
  sprite.set_size(make_size(64, 32));
  Transform2D transform;
  Static::Vector<R64, 2> translation = {{12, 34}};
  transform.translation = Point<R64, 2>{{translation}};
  transform.scale_x = 2.0;
  sprite.set_transform(transform);
  sprite.set_z_index(7);
  ASSERT(sprite.is_drawable());

  Sprite alias = sprite;
  alias.set_visible(False);
  EXPECT_NOT(alias.is_visible());
  EXPECT_NOT(alias.is_drawable());
  EXPECT(sprite.is_visible());
  EXPECT(sprite.is_drawable());
  EXPECT(sprite.get_transform().translation[0] == 12);
  EXPECT_EQ(sprite.get_size()[0], U32(64));
  EXPECT_EQ(
      &alias.get_texture().get_image(), &sprite.get_texture().get_image());
}
