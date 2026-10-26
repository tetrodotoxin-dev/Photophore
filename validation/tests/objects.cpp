// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "toolchain/validation/test.hpp"

#include "perimortem/core/math/domain.hpp"
#include "perimortem/core/math/point.hpp"
#include "perimortem/core/math/size.hpp"
#include "perimortem/core/storage/vector.hpp"
#include "perimortem/memory/matrix.hpp"
#include "perimortem/memory/thread/vector.hpp"

#include "photophore/image/srgba8.hpp"
#include "photophore/image/texture_2d.hpp"
#include "photophore/scene/sprite.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Toolchain::Validation;
using namespace Photophore::Image;
using namespace Photophore::Scene;

static Harness GraphicsObjects = {
  .name = "Photophore::Objects",
};

static auto make_size(U32 width, U32 height) -> Math::Size<U32, 2> {
  Storage::Vector<U32, 2> dimensions = {{width, height}};
  return Math::Size<U32, 2>(dimensions);
}

static auto one_pixel_image() -> Matrix<Srgba8> {
  Thread::Vector<Srgba8> pixels;
  pixels.insert(Srgba8::from_rgba(0x12, 0x34, 0x56, 0x78));
  return Matrix<Srgba8>::create(
      Math::Domain<2>(make_size(1, 1)), pixels.get_view());
}

VALIDATION_TEST(GraphicsObjects, sprite_values) {
  Sprite sprite;
  EXPECT_NOT(sprite.is_drawable());
  EXPECT(sprite.is_visible());
  EXPECT_EQ(sprite.get_z_index(), S64(0));

  Texture2D texture = Texture2D::create(Pixels2D::create(one_pixel_image()));
  sprite.set_texture(texture);
  sprite.set_size(make_size(64, 32));

  Transform2D transform;
  Storage::Vector<R64, 2> translation = {{12, 34}};
  transform.translation = Math::Point<R64, 2>{{translation}};
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
  EXPECT(alias.try_texture().visit(
      [&sprite](const Texture2D& alias_texture) {
        return sprite.try_texture().visit(
            [&alias_texture](const Texture2D& sprite_texture) {
              return alias_texture.get_content() ==
                     sprite_texture.get_content();
            },

            [] { return False; });
      },

      [] { return False; }));
}
