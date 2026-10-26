// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "perimortem/core/static/vector.hpp"
#include "perimortem/core/algorithm/sort.hpp"
#include "perimortem/core/data.hpp"

#include "perimortem/memory/matrix.hpp"

#include "perimortem/math/domain.hpp"
#include "perimortem/math/size.hpp"
#include "photophore/frame/batch.hpp"
#include "photophore/image/srgba8.hpp"
#include "photophore/image/texture_2d.hpp"
#include "toolchain/validation/benchmark.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Math;
using namespace Perimortem::Memory;
using namespace Photophore::Image;

static auto make_size(U32 width, U32 height) -> Size<U32, 2> {
  Static::Vector<U32, 2> dimensions = {{width, height}};
  return Size<U32, 2>(dimensions);
}

static auto one_pixel_image() -> Matrix<Srgba8> {
  Dynamic::Vector<Srgba8> pixels;
  return Matrix<Srgba8>::create(
      Domain<2, Boundary::Clip, Boundary::Clip>(make_size(1, 1)),
      pixels.get_view());
}

static Dynamic::Vector<Photophore::Frame::Batch> batches;
static Toolchain::Validation::Harness Frames = {
  .name = "Photophore::Frame",
  .setup =
      [] {
        Matrix<Srgba8> image = one_pixel_image();
        const Texture2D texture(Data::take(image));
        for (Count i = 0; i < 1024; ++i) {
          Dynamic::Vector<Texture2D> resources;
          resources.insert(texture);
          batches.emplace(
              Photophore::Frame::Batch(
                  Photophore::Frame::Program(), Data::take(resources),
                  Dynamic::Bytes(), Photophore::Frame::Transform(),
                  make_size(1, 1), Photophore::Frame::Pipeline(), 6,
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
