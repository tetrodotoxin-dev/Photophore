// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "perimortem/core/static/vector.hpp"
#include "perimortem/core/data.hpp"

#include "perimortem/memory/matrix.hpp"

#include "perimortem/serialization/rgba8.hpp"

#include "perimortem/math/domain.hpp"
#include "perimortem/math/size.hpp"
#include "photophore/frame/batch.hpp"
#include "photophore/sprite.hpp"
#include "photophore/texture_2d.hpp"
#include "toolchain/validation/unit_test.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Math;
using namespace Perimortem::Memory;
using namespace Perimortem::Serialization;
using namespace Toolchain::Validation;
using namespace Photophore;

static Harness GraphicsImage = {.name = "Photophore::ImageResources"};

static auto make_size(U32 width, U32 height) -> Size<U32, 2> {
  Static::Vector<U32, 2> dimensions = {{width, height}};
  return Size<U32, 2>(dimensions);
}

VALIDATION_TEST(GraphicsImage, empty_values) {
  const Count requests = Bibliotheca::check_out_requests();
  Sprite sprite;
  Texture2D texture;
  EXPECT_EQ(Bibliotheca::check_out_requests(), requests);
  EXPECT_NOT(sprite.is_drawable());
  EXPECT(texture.is_empty());
  EXPECT_EQ(texture.get_reservations(), Count(0));
}

// Sharing is selected by the texture owner. A frame retains that same pixel
// matrix after the author's owner is released, with independent sampling
// values.
VALIDATION_TEST(GraphicsImage, frame_lifetime) {
  Dynamic::Vector<Texture2D> textures;
  const Matrix<Rgba8>* identity;
  {
    Dynamic::Vector<Rgba8> pixels;
    pixels.emplace(Rgba8::from_rgb(17, 34, 51));
    Matrix<Rgba8> created = Matrix<Rgba8>::create(
        Domain<2, Boundary::Clip, Boundary::Clip>(make_size(1, 1)),
        pixels.get_view());
    Texture2D wrapped(
        Data::take(created),
        Sampler2D(Sampler2D::Addressing::Wrap, Sampler2D::Filtering::Linear));
    identity = &wrapped.get_image();
    Texture2D clamped(
        wrapped,
        Sampler2D(Sampler2D::Addressing::Clamp, Sampler2D::Filtering::Nearest));
    EXPECT_NOT(wrapped.matches(clamped));
    textures.insert(wrapped);
    textures.insert(clamped);
    EXPECT_EQ(wrapped.get_reservations(), Count(4));
  }

  Frame::Batch frame(
      Frame::Program(), Data::take(textures), Dynamic::Bytes(),
      Frame::Transform(), make_size(1, 1), Frame::Pipeline(), 6, 0, 0);
  const auto* resources = frame.get_resources().get_data();
  EXPECT_EQ(&resources[0].get_image(), identity);
  EXPECT_EQ(&resources[1].get_image(), identity);
  EXPECT_EQ(resources[0].get_reservations(), Count(2));
  EXPECT_EQ(resources[1].get_image().get_values()[0].red, U8(17));
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
