// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "photophore/frame/batch.hpp"

#include "toolchain/validation/test.hpp"

#include "perimortem/core/data.hpp"
#include "perimortem/core/math/size.hpp"
#include "perimortem/core/storage/vector.hpp"
#include "perimortem/memory/thread/bytes.hpp"
#include "perimortem/memory/thread/vector.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Photophore::Frame;
using namespace Photophore::Image;
using namespace Toolchain::Validation;

static Harness FrameBatch = {
  .name = "Photophore::Frame::Batch",
};

static auto make_size(U32 width, U32 height) -> Math::Size<U32, 2> {
  Storage::Vector<U32, 2> dimensions = {{width, height}};
  return Math::Size<U32, 2>(dimensions);
}

static auto make_batch(S64 z_index, Count order) -> Batch {
  Thread::Bytes inputs;
  inputs.append(0xA5, 4);

  Thread::Vector<Texture2D> resources;
  return Batch(
      Program(), Data::take(resources), Data::take(inputs), Transform(),
      make_size(7, 9), Pipeline(), 6, z_index, order);
}

VALIDATION_TEST(FrameBatch, captured_values) {
  const Batch batch = make_batch(12, 34);
  EXPECT_EQ(batch.get_inputs().get_size(), Count(4));
  EXPECT_EQ(batch.get_inputs()[0], U8(0xA5));
  EXPECT(batch.get_resources().is_empty());
  EXPECT_EQ(batch.get_size_pixels()[0], U32(7));
  EXPECT_EQ(batch.get_size_pixels()[1], U32(9));
  EXPECT_EQ(batch.get_vertex_count(), Count(6));
  EXPECT_EQ(batch.get_z_index(), S64(12));
  EXPECT_EQ(batch.get_authored_order(), Count(34));
}

VALIDATION_TEST(FrameBatch, ordering) {
  const Batch lower_layer = make_batch(2, 100);
  const Batch earlier = make_batch(3, 7);
  const Batch later = make_batch(3, 8);
  EXPECT(earlier > lower_layer);
  EXPECT(later > earlier);
  EXPECT_NOT(earlier > later);
}
