// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "examples/program.hpp"

#include "perimortem/core/data.hpp"
#include "perimortem/core/view/null_terminated.hpp"

using namespace Perimortem::Core;
using namespace Photophore::Description;
using namespace Photophore::Examples;

// These modules are optimized Vulkan 1.3 SPIR V compiled from the neighboring
// GLSL sources. Embedding the validated words keeps the executable independent
// of a host shader compiler while the authored programs remain reviewable.
alignas(U32) static constexpr U32 vertex_module[] = {
  119734787u, 67072u,      851979u, 77u,         0u,          131089u,
  1u,         393227u,     1u,      1280527431u, 1685353262u, 808793134u,
  0u,         196622u,     0u,      1u,          655375u,     0u,
  4u,         1852399981u, 0u,      13u,         21u,         65u,
  74u,        75u,         196679u, 11u,         2u,          327752u,
  11u,        0u,          35u,     0u,          327752u,     11u,
  1u,         35u,         16u,     327752u,     11u,         2u,
  35u,        32u,         262215u, 21u,         30u,         0u,
  196679u,    63u,         2u,      327752u,     63u,         0u,
  11u,        0u,          327752u, 63u,         1u,          11u,
  1u,         327752u,     63u,     2u,          11u,         3u,
  327752u,    63u,         3u,      11u,         4u,          262215u,
  74u,        30u,         0u,      262215u,     75u,         30u,
  1u,         131091u,     2u,      196641u,     3u,          2u,
  196630u,    6u,          32u,     262167u,     7u,          6u,
  2u,         262167u,     10u,     6u,          4u,          327710u,
  11u,        10u,         10u,     10u,         262176u,     12u,
  9u,         11u,         262203u, 12u,         13u,         9u,
  262165u,    14u,         32u,     1u,          262187u,     14u,
  15u,        0u,          262176u, 16u,         9u,          10u,
  262176u,    20u,         1u,      7u,          262203u,     20u,
  21u,        1u,          262165u, 24u,         32u,         0u,
  262187u,    24u,         25u,     2u,          262176u,     26u,
  9u,         6u,          262187u, 14u,         30u,         1u,
  262187u,    24u,         45u,     3u,          262187u,     6u,
  49u,        1073741824u, 262187u, 6u,          51u,         1065353216u,
  262187u,    24u,         53u,     1u,          262172u,     62u,
  6u,         53u,         393246u, 63u,         10u,         6u,
  62u,        62u,         262176u, 64u,         3u,          63u,
  262203u,    64u,         65u,     3u,          262187u,     6u,
  67u,        0u,          262176u, 71u,         3u,          10u,
  262176u,    73u,         3u,      7u,          262203u,     73u,
  74u,        3u,          262203u, 20u,         75u,         1u,
  327734u,    2u,          4u,      0u,          3u,          131320u,
  5u,         327745u,     16u,     17u,         13u,         15u,
  262205u,    10u,         18u,     17u,         458831u,     7u,
  19u,        18u,         18u,     0u,          1u,          262205u,
  7u,         22u,         21u,     327828u,     6u,          23u,
  19u,        22u,         393281u, 26u,         27u,         13u,
  15u,        25u,         262205u, 6u,          28u,         27u,
  327809u,    6u,          29u,     23u,         28u,         327745u,
  16u,        31u,         13u,     30u,         262205u,     10u,
  32u,        31u,         458831u, 7u,          33u,         32u,
  32u,        0u,          1u,      327828u,     6u,          35u,
  33u,        22u,         393281u, 26u,         36u,         13u,
  30u,        25u,         262205u, 6u,          37u,         36u,
  327809u,    6u,          38u,     35u,         37u,         393281u,
  26u,        46u,         13u,     15u,         45u,         262205u,
  6u,         47u,         46u,     327816u,     6u,          48u,
  29u,        47u,         327813u, 6u,          50u,         48u,
  49u,        327811u,     6u,      52u,         50u,         51u,
  393281u,    26u,         56u,     13u,         30u,         45u,
  262205u,    6u,          57u,     56u,         327816u,     6u,
  58u,        38u,         57u,     327813u,     6u,          59u,
  58u,        49u,         327811u, 6u,          60u,         59u,
  51u,        458832u,     10u,     70u,         52u,         60u,
  67u,        51u,         327745u, 71u,         72u,         65u,
  15u,        196670u,     72u,     70u,         262205u,     7u,
  76u,        75u,         196670u, 74u,         76u,         65789u,
  65592u,
};

alignas(U32) static constexpr U32 fragment_module[] = {
  119734787u,  67072u,      851979u,     52u,         0u,          131089u,
  1u,          393227u,     1u,          1280527431u, 1685353262u, 808793134u,
  0u,          196622u,     0u,          1u,          655375u,     4u,
  4u,          1852399981u, 0u,          13u,         17u,         21u,
  36u,         44u,         196624u,     4u,          7u,          262215u,
  13u,         33u,         0u,          262215u,     13u,         34u,
  0u,          262215u,     17u,         30u,         0u,          262215u,
  21u,         33u,         0u,          262215u,     21u,         34u,
  1u,          262215u,     36u,         30u,         0u,          196679u,
  42u,         2u,          327752u,     42u,         0u,          35u,
  0u,          327752u,     42u,         1u,          35u,         16u,
  327752u,     42u,         2u,          35u,         32u,         131091u,
  2u,          196641u,     3u,          2u,          196630u,     6u,
  32u,         262167u,     7u,          6u,          4u,          589849u,
  10u,         6u,          1u,          0u,          0u,          0u,
  1u,          0u,          196635u,     11u,         10u,         262176u,
  12u,         0u,          11u,         262203u,     12u,         13u,
  0u,          262167u,     15u,         6u,          2u,          262176u,
  16u,         1u,          15u,         262203u,     16u,         17u,
  1u,          262203u,     12u,         21u,         0u,          262187u,
  6u,          23u,         1065353216u, 262165u,     24u,         32u,
  0u,          262187u,     24u,         25u,         0u,          262176u,
  26u,         1u,          6u,          262187u,     24u,         30u,
  1u,          262176u,     35u,         3u,          7u,          262203u,
  35u,         36u,         3u,          262187u,     6u,          39u,
  1051931443u, 327710u,     42u,         7u,          7u,          7u,
  262176u,     43u,         9u,          42u,         262203u,     43u,
  44u,         9u,          262165u,     45u,         32u,         1u,
  262187u,     45u,         46u,         2u,          262176u,     47u,
  9u,          7u,          458796u,     7u,          51u,         39u,
  39u,         39u,         39u,         327734u,     2u,          4u,
  0u,          3u,          131320u,     5u,          262205u,     11u,
  14u,         13u,         262205u,     15u,         18u,         17u,
  327767u,     7u,          19u,         14u,         18u,         262205u,
  11u,         22u,         21u,         327745u,     26u,         27u,
  17u,         25u,         262205u,     6u,          28u,         27u,
  327811u,     6u,          29u,         23u,         28u,         327745u,
  26u,         31u,         17u,         30u,         262205u,     6u,
  32u,         31u,         327760u,     15u,         33u,         29u,
  32u,         327767u,     7u,          34u,         22u,         33u,
  524300u,     7u,          41u,         1u,          46u,         19u,
  34u,         51u,         327745u,     47u,         48u,         44u,
  46u,         262205u,     7u,          49u,         48u,         327813u,
  7u,          50u,         41u,         49u,         196670u,     36u,
  50u,         65789u,      65592u,
};

static constexpr Identity::Uuid program_identity(
    0xf8f633ad0ed74346ULL,
    0x9fcf6be1e3be4a90ULL);

// Both shader stages read the same twelve word host block. The vertex stage
// consumes transform rows while the pixel stage consumes the tone suffix.
static constexpr Stage input_stages[] = {
  Stage::Vertex,
  Stage::Pixel,
};

static const Module modules[] = {
  {
    Stage::Vertex,
    View::Vector<U32>(vertex_module),
    {},
  },

  {
    Stage::Pixel,
    View::Vector<U32>(fragment_module),
    {},
  },
};

static const HostInputRange host_input_ranges[] = {
  {
    0,
    sizeof(R32) * 12,
    View::Vector<Stage>(input_stages),
  },
};

static const DescriptorBinding descriptors[] = {
  // Distinct descriptor sets demonstrate multiple independently cached images
  // in one fragment pipeline while retaining binding zero in each set.
  {"image"_view, 0, 0, Resource::SampledTexture2D},
  {"overlay"_view, 1, 0, Resource::SampledTexture2D},
};

static const HostField host_fields[] = {
  // Pipelines derives transform rows from each Batch and copies tone from its
  // variable parameter bytes into the same generated host block.
  {"transform_x"_view, 0, sizeof(R32) * 4, HostRole::TransformX},

  {
    "transform_y"_view,
    sizeof(R32) * 4,
    sizeof(R32) * 4,
    HostRole::TransformY,
  },

  {
    "tone"_view,
    sizeof(R32) * 8,
    sizeof(R32) * 4,
    HostRole::Parameter,
  },
};

static constexpr VertexInput vertex_inputs[] = {
  {0, 2, 0, sizeof(R32) * 4},
  {1, 2, sizeof(R32) * 2, sizeof(R32) * 4},
};

auto SpriteProgram::get_program() -> decltype(SpriteProgram::get_program()) {
  return decltype(SpriteProgram::get_program())(program_identity);
}

auto SpriteProgram::get_description()
    -> decltype(SpriteProgram::get_description()) {
  return {
    .identity = program_identity,
    .modules = View::Vector<Module>(modules),
    .host_input_ranges = View::Vector<HostInputRange>(host_input_ranges),
    .descriptors = View::Vector<DescriptorBinding>(descriptors),
    .host_fields = View::Vector<HostField>(host_fields),
    .vertex_inputs = View::Vector<VertexInput>(vertex_inputs),
    .host_size = sizeof(R32) * 12,
    .parameters_offset = sizeof(R32) * 8,
    .parameters_size = sizeof(R32) * 4,
    .requires_float64 = False,
  };
}
