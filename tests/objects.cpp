// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "toolchain/validation/unit_test.hpp"

#include "perimortem/core/data.hpp"
#include "perimortem/core/null_terminated.hpp"

#include "perimortem/memory/dynamic/vector.hpp"

#include "perimortem/graphics/image.hpp"
#include "perimortem/graphics/pixel.hpp"
#include "photophore/projection.hpp"
#include "photophore/sprite.hpp"
#include "photophore/texture_2d.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Graphics;
using namespace Perimortem::Memory;
using namespace Toolchain::Validation;
using namespace Photophore;

static Harness GraphicsObjects = {
  .name = "Photophore::Objects",
};

static auto one_pixel_image() -> Perimortem::Graphics::Image {
  Dynamic::Vector<Perimortem::Graphics::Pixel> pixels;
  pixels.emplace(
      Perimortem::Graphics::Pixel::from_rgba(0x12, 0x34, 0x56, 0x78));
  return Perimortem::Graphics::Image::create(
             Perimortem::Graphics::Size2D(1, 1), Data::take(pixels))
      .visit(
          [] { return Perimortem::Graphics::Image(); },
          [](Perimortem::Graphics::Image& image) { return Data::take(image); });
}

static auto finalize_shader(U8*) -> void {}

static const Object<>::Descriptor
    shader_descriptor(sizeof(R32) * 4, alignof(R32), finalize_shader);

alignas(U32) static constexpr U32 graphics_program[] = {0x07230203};

static const Projection graphics_projection = {
  Data::cast<const U8>(graphics_program),
  0,
  sizeof(R32) * 4,
};

static auto create_shader() -> Implementation {
  Object<> object = Object<>::create(shader_descriptor);
  auto shader = Implementation::retain(object, &graphics_projection);
  object.release();
  return shader ? static_cast<Implementation&&>(*shader) : Implementation();
}

VALIDATION_TEST(GraphicsObjects, sprite_values) {
  Sprite sprite;
  EXPECT_NOT(sprite.is_drawable());
  EXPECT(sprite.is_visible());
  EXPECT_EQ(sprite.get_z_index(), S64(0));
  EXPECT(sprite.get_material().is_empty());

  Dynamic::Record<const Perimortem::Graphics::Image> image(one_pixel_image());
  Texture2D texture(image);
  sprite.set_texture(texture);
  sprite.set_material(create_shader());
  sprite.set_size({64, 32});
  Transform2D transform;
  transform.translation = Perimortem::Graphics::Point2D(12, 34);
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
  EXPECT(sprite.get_transform().translation.x == 12);
  EXPECT_EQ(sprite.get_size().width, U32(64));
  EXPECT_EQ(sprite.get_material().get_projection(), &graphics_projection);
  EXPECT_EQ(
      &alias.get_texture().get_image(), &sprite.get_texture().get_image());
}
