// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "toolchain/validation/test.hpp"

#include "perimortem/core/data.hpp"
#include "perimortem/core/math/domain.hpp"
#include "perimortem/core/math/size.hpp"
#include "perimortem/core/storage/vector.hpp"
#include "perimortem/memory/allocator/bibliotheca.hpp"
#include "perimortem/memory/matrix.hpp"

#include "photophore/frame/batch.hpp"
#include "photophore/image/srgba8.hpp"
#include "photophore/image/texture_2d.hpp"
#include "photophore/scene/sprite.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Toolchain::Validation;
using namespace Photophore::Image;
using namespace Photophore::Scene;
using namespace Photophore::Frame;

static Harness GraphicsImage = {.name = "Photophore::ImageResources"};

static auto make_size(U32 width, U32 height) -> Math::Size<U32, 2> {
  Storage::Vector<U32, 2> dimensions = {{width, height}};
  return Math::Size<U32, 2>(dimensions);
}

VALIDATION_TEST(GraphicsImage, srgba8_contract) {
  const Srgba8 empty;
  EXPECT_EQ(empty.red, U8(0));
  EXPECT_EQ(empty.green, U8(0));
  EXPECT_EQ(empty.blue, U8(0));
  EXPECT_EQ(empty.alpha, U8(0));

  const Srgba8 opaque = Srgba8::from_rgb(17, 34, 51);
  EXPECT_EQ(opaque.red, U8(17));
  EXPECT_EQ(opaque.green, U8(34));
  EXPECT_EQ(opaque.blue, U8(51));
  EXPECT_EQ(opaque.alpha, U8(0xFF));
}

VALIDATION_TEST(GraphicsImage, empty_values) {
  const Count requests = Allocator::Bibliotheca::check_out_requests();
  Sprite sprite;
  Texture2D texture;
  EXPECT_EQ(Allocator::Bibliotheca::check_out_requests(), requests);
  EXPECT_NOT(sprite.is_drawable());
  EXPECT(texture.is_empty());
  EXPECT_EQ(texture.get_reservations(), Count(0));
}

// Texture2D copies retain one pixel Matrix while preserving independent
// sampling values. This case destroys the original Texture2D values before the
// frame reads the shared pixels.
VALIDATION_TEST(GraphicsImage, frame_lifetime) {
  Thread::Vector<Texture2D> textures;
  const Matrix<Srgba8>* identity;
  {
    Thread::Vector<Srgba8> pixels;
    pixels.insert(Srgba8::from_rgb(17, 34, 51));
    Matrix<Srgba8> created = Matrix<Srgba8>::create(
        Math::Domain<2>(make_size(1, 1)), pixels.get_view());
    Texture2D wrapped(
        Data::take(created),
        Sampler2D::create(
            Sampler2D::Addressing::Wrap, Sampler2D::Filtering::Linear));
    identity = &wrapped.get_image();
    Texture2D clamped(
        wrapped,
        Sampler2D::create(
            Sampler2D::Addressing::Clamp, Sampler2D::Filtering::Nearest));
    EXPECT_NOT(wrapped.matches(clamped));
    textures.insert(wrapped);
    textures.insert(clamped);
    EXPECT_EQ(wrapped.get_reservations(), Count(4));
  }

  Batch frame(
      Program(), Data::take(textures), Thread::Bytes(), Transform(),
      make_size(1, 1), Pipeline(), 6, 0, 0);
  const auto* resources = frame.get_resources().get_data();
  EXPECT_EQ(&resources[0].get_image(), identity);
  EXPECT_EQ(&resources[1].get_image(), identity);
  EXPECT_EQ(resources[0].get_reservations(), Count(2));
  EXPECT_EQ(resources[1].get_image().get_view().get_values()[0].red, U8(17));
  EXPECT(
      resources[0].get_sampler().get_addressing() ==
      Sampler2D::Addressing::Wrap);
  EXPECT(
      resources[1].get_sampler().get_addressing() ==
      Sampler2D::Addressing::Clamp);

  const Count retained = Allocator::Bibliotheca::allocated_memory();
  frame = Batch();
  EXPECT(Allocator::Bibliotheca::allocated_memory() < retained);
}
