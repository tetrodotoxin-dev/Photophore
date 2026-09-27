// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "perimortem/core/null_terminated.hpp"

#include "toolchain/validation/benchmark.hpp"

#include "perimortem/core/algorithm/sort.hpp"
#include "perimortem/core/diagnostics/log.hpp"

#include "photophore/frame/batch.hpp"

using namespace Perimortem;
using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::Graphics;
using namespace Photophore;

static Dynamic::Vector<Frame::Batch> batches;
static Toolchain::Validation::Harness Frames = {
  .name = "Photophore::Frame",
  .setup =
      [] {
        auto image = Image::create(Size2D(1, 1), Dynamic::Vector<Pixel>());
        if (!image) {
          Diagnostics::Log::fatal("Benchmark image creation failed."_view);
        }
        const Texture2D texture(
            Dynamic::Record<const Image>(Data::take(*image)));
        for (Count i = 0; i < 1024; ++i) {
          Dynamic::Vector<Texture2D> resources;
          resources.insert(texture);
          batches.emplace(
              Frame::Batch(
                  Frame::Program(), Data::take(resources), Dynamic::Bytes(),
                  Frame::Transform(), Size2D(1, 1), Frame::Pipeline(), 6,
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
