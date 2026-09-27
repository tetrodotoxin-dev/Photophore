// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "toolchain/validation/unit_test.hpp"

#include "perimortem/graphics/image.hpp"
#include "photophore/frame/batch.hpp"
#include "photophore/sprite.hpp"
#include "photophore/texture_2d.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Graphics;
using namespace Perimortem::Memory;
using namespace Toolchain::Validation;
using namespace Photophore;

static Harness GraphicsImage = {.name = "Photophore::ImageResources"};

VALIDATION_TEST(GraphicsImage, empty_values) {
  const Count requests = Bibliotheca::check_out_requests();
  Sprite sprite;
  Texture2D texture;
  EXPECT_EQ(Bibliotheca::check_out_requests(), requests);
  EXPECT_NOT(sprite.is_drawable());
  EXPECT(texture.is_empty());
  EXPECT_EQ(texture.get_reservations(), Count(0));
}

// Sharing is selected by the texture owner. A frame retains that same Image
// after the author's owner is released, with independent sampling values.
VALIDATION_TEST(GraphicsImage, frame_lifetime) {
  Dynamic::Vector<Texture2D> textures;
  const Image* identity;
  {
    Dynamic::Vector<Pixel> pixels;
    pixels.emplace(Pixel::from_rgb(17, 34, 51));
    auto created = Image::create(Size2D(1, 1), Data::take(pixels));
    ASSERT(created);
    Dynamic::Record<const Image> image(Data::take(*created));
    identity = &*image;
    Texture2D wrapped(
        image,
        Sampler2D(Sampler2D::Addressing::Wrap, Sampler2D::Filtering::Linear));
    Texture2D clamped(
        image,
        Sampler2D(Sampler2D::Addressing::Clamp, Sampler2D::Filtering::Nearest));
    EXPECT_NOT(wrapped.matches(clamped));
    textures.insert(wrapped);
    textures.insert(clamped);
    EXPECT_EQ(image.get_reservations(), Count(5));
  }

  Frame::Batch frame(
      Frame::Program(), Data::take(textures), Dynamic::Bytes(),
      Frame::Transform(), Size2D(1, 1), Frame::Pipeline(), 6, 0, 0);
  const auto* resources = frame.get_resources().get_data();
  EXPECT_EQ(&resources[0].get_image(), identity);
  EXPECT_EQ(&resources[1].get_image(), identity);
  EXPECT_EQ(resources[0].get_reservations(), Count(2));
  EXPECT_EQ(resources[1].get_image().get_pixel(0, 0).red, U8(17));
  EXPECT(
      resources[0].get_sampler().get_addressing() ==
      Sampler2D::Addressing::Wrap);
  EXPECT(
      resources[1].get_sampler().get_addressing() ==
      Sampler2D::Addressing::Clamp);

  const Count retained = Bibliotheca::allocated_memory();
  frame = Frame::Batch();
  EXPECT(Bibliotheca::allocated_memory() < retained);
}
