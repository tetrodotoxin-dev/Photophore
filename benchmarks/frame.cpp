// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "perimortem/core/static/vector.hpp"
#include "perimortem/core/algorithm/sort.hpp"
#include "perimortem/core/data.hpp"

#include "perimortem/memory/matrix.hpp"

#include "perimortem/serialization/rgba8.hpp"

#include "perimortem/math/domain.hpp"
#include "perimortem/math/size.hpp"
#include "photophore/frame/batch.hpp"
#include "toolchain/validation/benchmark.hpp"

using namespace Perimortem;
using namespace Perimortem::Core;
using namespace Perimortem::Math;
using namespace Perimortem::Memory;
using namespace Perimortem::Serialization;
using namespace Photophore;

static auto make_size(U32 width, U32 height) -> Size<U32, 2> {
  Static::Vector<U32, 2> dimensions = {{width, height}};
  return Size<U32, 2>(dimensions);
}

static auto one_pixel_image() -> Matrix<Rgba8> {
  Dynamic::Vector<Rgba8> pixels;
  return Matrix<Rgba8>::create(
      Domain<2, Boundary::Clip, Boundary::Clip>(make_size(1, 1)),
      pixels.get_view());
}

static Dynamic::Vector<Frame::Batch> batches;
static Toolchain::Validation::Harness Frames = {
  .name = "Photophore::Frame",
  .setup =
      [] {
        Matrix<Rgba8> image = one_pixel_image();
        const Texture2D texture(Data::take(image));
        for (Count i = 0; i < 1024; ++i) {
          Dynamic::Vector<Texture2D> resources;
          resources.insert(texture);
          batches.emplace(
              Frame::Batch(
                  Frame::Program(), Data::take(resources), Dynamic::Bytes(),
                  Frame::Transform(), make_size(1, 1), Frame::Pipeline(), 6,
                  S64(1024 - i), i));
        }
      },
  .teardown = [] { batches.reset(); },
};

// These CPU costs precede Vulkan submission. Device presentation has its own
// integration check because compositor pacing would obscure container costs.
VALIDATION_BENCHMARK(Frames, sort_1024) {
  Algorithm::sort(batches.get_access());
  auto order = batches[0].get_z_index();
  Toolchain::Validation::Benchmark::prevent_optimization(order);
}

VALIDATION_BENCHMARK(Frames, retain_1024) {
  auto copy = batches;
  auto count = copy.get_size();
  Toolchain::Validation::Benchmark::prevent_optimization(count);
}
