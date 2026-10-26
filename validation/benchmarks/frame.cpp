// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "toolchain/validation/benchmark.hpp"

#include "perimortem/core/algorithm/sort.hpp"
#include "perimortem/core/data.hpp"
#include "perimortem/core/math/domain.hpp"
#include "perimortem/core/math/size.hpp"
#include "perimortem/core/storage/vector.hpp"
#include "perimortem/memory/matrix.hpp"

#include "photophore/frame/batch.hpp"
#include "photophore/image/srgba8.hpp"
#include "photophore/image/texture_2d.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Photophore::Image;
using namespace Photophore::Frame;
using namespace Toolchain::Validation;

static auto make_size(U32 width, U32 height) -> Math::Size<U32, 2> {
  Storage::Vector<U32, 2> dimensions = {{width, height}};
  return Math::Size<U32, 2>(dimensions);
}

static auto one_pixel_image() -> Matrix<Srgba8> {
  Thread::Vector<Srgba8> pixels;
  return Matrix<Srgba8>::create(
      Math::Domain<2>(make_size(1, 1)), pixels.get_view());
}

static Thread::Vector<Batch> batches;
static Harness Frames = {
  .name = "Photophore::Frame",
  .setup =
      [] {
        Matrix<Srgba8> image = one_pixel_image();
        const Texture2D texture(Data::take(image));
        for (Count i = 0; i < 1024; ++i) {
          Thread::Vector<Texture2D> resources;
          resources.insert(texture);
          batches.insert(Batch(
              Program(), Data::take(resources), Thread::Bytes(), Transform(),
              make_size(1, 1), Pipeline(), 6, S64(1024 - i), i));
        }
      },

  .teardown = [] { batches.reset(); },
};

// These CPU costs precede Vulkan submission. Device presentation has its own
// integration check because compositor pacing would obscure container costs.
VALIDATION_BENCHMARK(Frames, sort_1024) {
  Algorithm::sort(batches.get_access());

  auto order = batches.get_data()[0].get_z_index();
  Benchmark::prevent_optimization(order);
}

VALIDATION_BENCHMARK(Frames, retain_1024) {
  auto copy = batches;
  auto count = copy.get_size();
  Benchmark::prevent_optimization(count);
}
