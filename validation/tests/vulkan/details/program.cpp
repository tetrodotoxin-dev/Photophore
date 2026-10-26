// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "photophore/vulkan/details/program.hpp"

#include "toolchain/validation/test.hpp"

#include "perimortem/core/data.hpp"
#include "perimortem/core/math/domain.hpp"
#include "perimortem/core/math/size.hpp"
#include "perimortem/core/storage/vector.hpp"
#include "perimortem/memory/matrix.hpp"
#include "perimortem/memory/thread/bytes.hpp"
#include "perimortem/memory/thread/vector.hpp"

#include "examples/program.hpp"
#include "photophore/image/srgba8.hpp"
#include "photophore/image/texture_2d.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Photophore::Description;
using namespace Photophore::Examples;
using namespace Photophore::Frame;
using namespace Photophore::Image;
using namespace Photophore::Vulkan;
using namespace Toolchain::Validation;

using DescriptionProgram = decltype(SpriteProgram::get_description());
using FrameProgram = decltype(SpriteProgram::get_program());

static Harness VulkanDetailsProgram = {
  .name = "Photophore::Vulkan::Details::Program",
};

static auto inspect(
    const DescriptionProgram& description,
    Bool supports_float64 = True) -> Details::Program::Status {
  return Details::Program::inspect(
      View::Vector<DescriptionProgram>(description, 1), supports_float64);
}

static auto make_size(U32 width, U32 height) -> Math::Size<U32, 2> {
  Storage::Vector<U32, 2> dimensions = {{width, height}};
  return Math::Size<U32, 2>(dimensions);
}

static auto make_content(Bool drawable = True) -> Pixels2D {
  if (!drawable) {
    Matrix<Srgba8> empty;
    return Pixels2D::create(Data::take(empty));
  }

  Thread::Vector<Srgba8> pixels;
  pixels.insert(Srgba8::from_rgb(0xFF, 0x00, 0x00));

  Matrix<Srgba8> matrix = Matrix<Srgba8>::create(
      Math::Domain<2>(make_size(1, 1)), pixels.get_view());
  return Pixels2D::create(Data::take(matrix));
}

static auto make_batch(
    Identity::Uuid identity,
    Count resource_count = 2,
    Count input_size = sizeof(R32) * 4,
    Math::Size<U32, 2> size = make_size(8, 8),
    Pipeline pipeline = Pipeline(),
    Count vertex_count = 6,
    Bool drawable = True) -> Batch {
  Thread::Vector<Texture2D> resources;
  for (Count index = 0; index < resource_count; index++) {
    resources.insert(Texture2D::create(make_content(drawable)));
  }

  Thread::Bytes inputs;
  inputs.append(0, input_size);
  return Batch(
      FrameProgram(identity), Data::take(resources), Data::take(inputs),
      Transform(), size, pipeline, vertex_count, 0, 0);
}

VALIDATION_TEST(VulkanDetailsProgram, inventory) {
  const DescriptionProgram valid = SpriteProgram::get_description();
  EXPECT(inspect(valid) == Details::Program::Status::Accepted);
  EXPECT(
      Details::Program::inspect(Details::Program::Descriptions(), True) ==
      Details::Program::Status::EmptyInventory);

  DescriptionProgram missing_identity = valid;
  missing_identity.identity = {};
  EXPECT(
      inspect(missing_identity) ==
      Details::Program::Status::InvalidDescription);

  Storage::Vector<DescriptionProgram, 2> duplicate;
  duplicate[0] = valid;
  duplicate[1] = valid;
  EXPECT(
      Details::Program::inspect(duplicate.get_view(), True) ==
      Details::Program::Status::RepeatedIdentity);
}

VALIDATION_TEST(VulkanDetailsProgram, modules) {
  const DescriptionProgram valid = SpriteProgram::get_description();

  DescriptionProgram missing = valid;
  missing.modules = {};
  EXPECT(inspect(missing) == Details::Program::Status::InvalidDescription);

  Module empty = valid.modules[0];
  empty.words = {};

  DescriptionProgram empty_words = valid;
  empty_words.modules = View::Vector<Module>(empty, 1);
  EXPECT(inspect(empty_words) == Details::Program::Status::InvalidDescription);

  Module invalid = valid.modules[0];
  invalid.stage = Stage(U32(-1));

  DescriptionProgram invalid_stage = valid;
  invalid_stage.modules = View::Vector<Module>(invalid, 1);
  EXPECT(
      inspect(invalid_stage) == Details::Program::Status::InvalidDescription);

  Storage::Vector<Module, Details::Program::module_limit + 1> excessive;
  for (Count index = 0; index < excessive.get_size(); index++) {
    excessive[index] = valid.modules[0];
  }

  DescriptionProgram too_many = valid;
  too_many.modules = excessive.get_view();
  EXPECT(inspect(too_many) == Details::Program::Status::InvalidDescription);
}

VALIDATION_TEST(VulkanDetailsProgram, host_layout) {
  const DescriptionProgram valid = SpriteProgram::get_description();

  DescriptionProgram empty = valid;
  empty.host_size = 0;
  EXPECT(inspect(empty) == Details::Program::Status::InvalidDescription);

  DescriptionProgram unaligned = valid;
  unaligned.host_size -= 1;
  EXPECT(inspect(unaligned) == Details::Program::Status::InvalidDescription);

  DescriptionProgram parameters = valid;
  parameters.parameters_offset = parameters.host_size + 1;
  EXPECT(inspect(parameters) == Details::Program::Status::InvalidDescription);

  parameters = valid;
  parameters.parameters_size = parameters.host_size;
  EXPECT(inspect(parameters) == Details::Program::Status::InvalidDescription);

  DescriptionProgram missing_range = valid;
  missing_range.host_input_ranges = {};
  EXPECT(
      inspect(missing_range) == Details::Program::Status::InvalidDescription);

  HostInputRange range = valid.host_input_ranges[0];
  range.offset = sizeof(R32);

  DescriptionProgram offset_range = valid;
  offset_range.host_input_ranges = View::Vector<HostInputRange>(range, 1);
  EXPECT(inspect(offset_range) == Details::Program::Status::InvalidDescription);

  range = valid.host_input_ranges[0];
  range.size -= sizeof(R32);

  DescriptionProgram short_range = valid;
  short_range.host_input_ranges = View::Vector<HostInputRange>(range, 1);
  EXPECT(inspect(short_range) == Details::Program::Status::InvalidDescription);

  range = valid.host_input_ranges[0];
  range.stages = {};

  DescriptionProgram unstaged_range = valid;
  unstaged_range.host_input_ranges = View::Vector<HostInputRange>(range, 1);
  EXPECT(
      inspect(unstaged_range) == Details::Program::Status::InvalidDescription);

  Stage invalid_stage = Stage(U32(-1));
  range = valid.host_input_ranges[0];
  range.stages = View::Vector<Stage>(invalid_stage, 1);

  DescriptionProgram invalid_stage_range = valid;
  invalid_stage_range.host_input_ranges =
      View::Vector<HostInputRange>(range, 1);
  EXPECT(
      inspect(invalid_stage_range) ==
      Details::Program::Status::InvalidDescription);

  HostField field = valid.host_fields[0];
  field.offset = valid.host_size;

  DescriptionProgram overflowing_field = valid;
  overflowing_field.host_fields = View::Vector<HostField>(field, 1);
  EXPECT(
      inspect(overflowing_field) ==
      Details::Program::Status::InvalidDescription);

  field = valid.host_fields[2];
  field.offset = 0;

  DescriptionProgram misplaced_parameter = valid;
  misplaced_parameter.host_fields = View::Vector<HostField>(field, 1);
  EXPECT(
      inspect(misplaced_parameter) ==
      Details::Program::Status::InvalidDescription);

  field = valid.host_fields[0];
  field.role = HostRole(U8(-1));

  DescriptionProgram invalid_role = valid;
  invalid_role.host_fields = View::Vector<HostField>(field, 1);
  EXPECT(inspect(invalid_role) == Details::Program::Status::InvalidDescription);

  field = valid.host_fields[0];
  field.size -= sizeof(R32);

  DescriptionProgram short_transform = valid;
  short_transform.host_fields = View::Vector<HostField>(field, 1);
  EXPECT(
      inspect(short_transform) == Details::Program::Status::InvalidDescription);
}

VALIDATION_TEST(VulkanDetailsProgram, descriptors) {
  const DescriptionProgram valid = SpriteProgram::get_description();

  DescriptionProgram missing = valid;
  missing.descriptors = {};
  EXPECT(inspect(missing) == Details::Program::Status::InvalidDescription);

  DescriptorBinding descriptor = valid.descriptors[0];
  descriptor.slot = 1;

  DescriptionProgram invalid_slot = valid;
  invalid_slot.descriptors = View::Vector<DescriptorBinding>(descriptor, 1);
  EXPECT(inspect(invalid_slot) == Details::Program::Status::InvalidDescriptor);

  descriptor = valid.descriptors[0];
  descriptor.resource = Resource(U8(-1));

  DescriptionProgram invalid_resource = valid;
  invalid_resource.descriptors = View::Vector<DescriptorBinding>(descriptor, 1);
  EXPECT(
      inspect(invalid_resource) == Details::Program::Status::InvalidDescriptor);

  descriptor = valid.descriptors[0];
  descriptor.set = Count(U32(-1)) + 1;

  DescriptionProgram narrow_set = valid;
  narrow_set.descriptors = View::Vector<DescriptorBinding>(descriptor, 1);
  EXPECT(inspect(narrow_set) == Details::Program::Status::InvalidDescriptor);

  Storage::Vector<DescriptorBinding, 2> repeated;
  repeated[0] = valid.descriptors[0];
  repeated[1] = valid.descriptors[1];
  repeated[1].set = repeated[0].set;

  DescriptionProgram duplicate_set = valid;
  duplicate_set.descriptors = repeated.get_view();
  EXPECT(
      inspect(duplicate_set) ==
      Details::Program::Status::RepeatedDescriptorSet);

  repeated[1].set = 3;

  DescriptionProgram sparse_sets = valid;
  sparse_sets.descriptors = repeated.get_view();
  EXPECT(inspect(sparse_sets) == Details::Program::Status::Accepted);
}

VALIDATION_TEST(VulkanDetailsProgram, float64) {
  DescriptionProgram required = SpriteProgram::get_description();
  required.requires_float64 = True;
  EXPECT(
      inspect(required, False) == Details::Program::Status::UnsupportedFloat64);
  EXPECT(inspect(required, True) == Details::Program::Status::Accepted);
}

VALIDATION_TEST(VulkanDetailsProgram, batch_contract) {
  DescriptionProgram description = SpriteProgram::get_description();
  const Details::Program::Descriptions descriptions(description, 1);
  const Identity::Uuid identity = description.identity;

  EXPECT(Details::Program::accepts(descriptions, make_batch(identity)));
  EXPECT(Details::Program::accepts(descriptions, Details::Program::Batches()));
  EXPECT_NOT(
      Details::Program::accepts(
          descriptions, make_batch(Identity::Uuid(0xBAD, 0xF00D))));
  EXPECT_NOT(Details::Program::accepts(descriptions, make_batch(identity, 1)));
  EXPECT_NOT(
      Details::Program::accepts(
          descriptions, make_batch(identity, 2, sizeof(R32) * 3)));
  EXPECT_NOT(
      Details::Program::accepts(
          descriptions, make_batch(identity, 2, sizeof(R32) * 4, {})));
  EXPECT_NOT(
      Details::Program::accepts(
          descriptions,
          make_batch(
              identity, 2, sizeof(R32) * 4, make_size(8, 8), Pipeline(), 0)));
  EXPECT_NOT(
      Details::Program::accepts(
          descriptions, make_batch(
                            identity, 2, sizeof(R32) * 4, make_size(8, 8),
                            Pipeline(), Count(U32(-1)) + 1)));
  EXPECT_NOT(
      Details::Program::accepts(
          descriptions, make_batch(
                            identity, 2, sizeof(R32) * 4, make_size(8, 8),
                            Pipeline(), 6, False)));
}

VALIDATION_TEST(VulkanDetailsProgram, fixed_pipeline) {
  DescriptionProgram description = SpriteProgram::get_description();
  const Details::Program::Descriptions descriptions(description, 1);
  const Identity::Uuid identity = description.identity;

  Pipeline invalid_topology(
      Pipeline::Topology(U8(-1)), Pipeline::Blend::Alpha,
      Pipeline::Geometry::UnitQuad2D);
  EXPECT_NOT(
      Details::Program::accepts(
          descriptions, make_batch(
                            identity, 2, sizeof(R32) * 4, make_size(8, 8),
                            invalid_topology)));

  Pipeline invalid_blend(
      Pipeline::Topology::TriangleList, Pipeline::Blend(U8(-1)),
      Pipeline::Geometry::UnitQuad2D);
  EXPECT_NOT(
      Details::Program::accepts(
          descriptions,
          make_batch(
              identity, 2, sizeof(R32) * 4, make_size(8, 8), invalid_blend)));

  Pipeline invalid_geometry(
      Pipeline::Topology::TriangleList, Pipeline::Blend::Alpha,
      Pipeline::Geometry(U8(-1)));
  EXPECT_NOT(
      Details::Program::accepts(
          descriptions, make_batch(
                            identity, 2, sizeof(R32) * 4, make_size(8, 8),
                            invalid_geometry)));

  Storage::Vector<VertexInput, 2> inputs;
  inputs[0] = description.vertex_inputs[0];
  inputs[1] = description.vertex_inputs[1];

  const VertexInput first = inputs[0];
  description.vertex_inputs = inputs.get_view();
  EXPECT(Details::Program::supports_unit_quad(description));

  inputs[0].location = 1;
  EXPECT_NOT(Details::Program::supports_unit_quad(description));
  inputs[0] = first;

  inputs[0].components = 3;
  EXPECT_NOT(Details::Program::supports_unit_quad(description));
  inputs[0] = first;

  inputs[0].stride -= sizeof(R32);
  EXPECT_NOT(Details::Program::supports_unit_quad(description));
  inputs[0] = first;

  inputs[1].offset = 0;
  EXPECT_NOT(
      Details::Program::accepts(
          Details::Program::Descriptions(description, 1),
          make_batch(identity)));
}
