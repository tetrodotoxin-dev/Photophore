// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#define TOOLCHAIN_PRIVATE_HEADER 1

#include "toolchain/toolchain.hpp"

#include "perimortem/core/storage/vector.hpp"
#include "perimortem/core/view/vector.hpp"

#include "photophore/description/program.hpp"
#include "photophore/frame/batch.hpp"

namespace Photophore::Vulkan::Details {

// Program owns the private rendering profile shared by Pipelines construction
// and command recording. It determines whether an authored Description can be
// realized by the selected device and whether each Batch matches that complete
// contract, including shader inputs, descriptors, host data and unit geometry.
// These checks create no Vulkan objects and retain no state, allowing every
// rejected form to be tested without a device or command buffer.
class Program {
 public:
  enum class Status : U8 {
    Accepted,
    EmptyInventory,
    InvalidDescription,
    UnsupportedFloat64,
    InvalidDescriptor,
    RepeatedDescriptorSet,
    RepeatedIdentity,
  };

  static constexpr Count module_limit = 8;

  // Pipelines owns one interleaved unit quad. Program descriptions must publish
  // these two R32 vectors with the same offsets and stride before a Batch can
  // use that immutable geometry.
  struct UnitQuadVertex {
    Perimortem::Core::Storage::Vector<R32, 2> position;
    Perimortem::Core::Storage::Vector<R32, 2> texture_uv;
  };

  using Descriptions = Perimortem::Core::View::Vector<Description::Program>;
  using Batches = Perimortem::Core::View::Vector<Frame::Batch>;

 private:
  static auto valid_stage(Description::Stage stage) -> Bool {
    switch (stage) {
    case Description::Stage::Vertex:
    case Description::Stage::Pixel:
      return True;
    }

    return False;
  }

  static auto valid_module(const Description::Module& module) -> Bool {
    return Bool(!module.words.is_empty() && valid_stage(module.stage));
  }

  static auto valid_host_layout(const Description::Program& program) -> Bool {
    if (program.host_size == 0 || program.host_size > U32(-1) ||
        (program.host_size & 3) != 0 ||
        program.parameters_offset > program.host_size ||
        program.parameters_size >
            program.host_size - program.parameters_offset ||
        program.host_input_ranges.get_size() != 1) {
      return False;
    }

    const auto& range = program.host_input_ranges.get_data()[0];
    if (range.offset != 0 || range.size != program.host_size ||
        range.stages.is_empty()) {
      return False;
    }

    for (Count index = 0; index < range.stages.get_size(); index++) {
      if (!valid_stage(range.stages.get_data()[index])) {
        return False;
      }
    }

    for (Count index = 0; index < program.host_fields.get_size(); index++) {
      const auto& field = program.host_fields.get_data()[index];
      if (field.size == 0 || field.offset > program.host_size ||
          field.size > program.host_size - field.offset) {
        return False;
      }

      switch (field.role) {
      case Description::HostRole::Parameter:
        if (field.offset < program.parameters_offset ||
            field.offset >
                program.parameters_offset + program.parameters_size ||
            field.size > program.parameters_offset + program.parameters_size -
                             field.offset) {
          return False;
        }

        break;
      case Description::HostRole::TransformX:
      case Description::HostRole::TransformY:
        if (field.size != sizeof(R32) * 4) {
          return False;
        }

        break;
      default:
        return False;
      }
    }

    return True;
  }

 public:
  static auto inspect(Descriptions descriptions, Bool supports_float64)
      -> Status {
    if (descriptions.is_empty()) {
      return Status::EmptyInventory;
    }

    for (Count index = 0; index < descriptions.get_size(); index++) {
      const auto& description = descriptions.get_data()[index];
      if (!description.identity.is_set() || description.modules.is_empty() ||
          description.modules.get_size() > module_limit ||
          description.descriptors.is_empty() ||
          !valid_host_layout(description)) {
        return Status::InvalidDescription;
      }

      for (Count module = 0; module < description.modules.get_size();
           module++) {
        if (!valid_module(description.modules.get_data()[module])) {
          return Status::InvalidDescription;
        }
      }

      if (description.requires_float64 && !supports_float64) {
        return Status::UnsupportedFloat64;
      }

      for (Count descriptor_index = 0;
           descriptor_index < description.descriptors.get_size();
           descriptor_index++) {
        const auto& descriptor =
            description.descriptors.get_data()[descriptor_index];
        if (descriptor.set > U32(-1) || descriptor.slot != 0 ||
            descriptor.resource != Description::Resource::SampledTexture2D) {
          return Status::InvalidDescriptor;
        }

        for (Count prior = 0; prior < descriptor_index; prior++) {
          if (description.descriptors.get_data()[prior].set == descriptor.set) {
            return Status::RepeatedDescriptorSet;
          }
        }
      }

      for (Count previous = 0; previous < index; previous++) {
        if (descriptions.get_data()[previous].identity ==
            description.identity) {
          return Status::RepeatedIdentity;
        }
      }
    }

    return Status::Accepted;
  }

  static auto find(
      Descriptions descriptions,
      Perimortem::Core::Identity::Uuid id) -> const Description::Program* {
    for (Count index = 0; index < descriptions.get_size(); index++) {
      if (descriptions.get_data()[index].identity == id) {
        return &descriptions.get_data()[index];
      }
    }

    return nullptr;
  }

  static auto supports_unit_quad(const Description::Program& description)
      -> Bool {
    const auto inputs = description.vertex_inputs;
    return Bool(
        inputs.get_size() == 2 && inputs.get_data()[0].location == 0 &&
        inputs.get_data()[0].components == 2 &&
        inputs.get_data()[0].offset == 0 &&
        inputs.get_data()[0].stride == sizeof(UnitQuadVertex) &&
        inputs.get_data()[1].location == 1 &&
        inputs.get_data()[1].components == 2 &&
        inputs.get_data()[1].offset == sizeof(R32) * 2 &&
        inputs.get_data()[1].stride == sizeof(UnitQuadVertex));
  }

  static auto accepts(Descriptions descriptions, const Frame::Batch& batch)
      -> Bool {
    const auto* description =
        find(descriptions, batch.get_program().get_identity());
    const auto pipeline = batch.get_pipeline();
    if (description == nullptr || batch.get_vertex_count() == 0 ||
        batch.get_vertex_count() > U32(-1) ||
        pipeline.get_geometry() != Frame::Pipeline::Geometry::UnitQuad2D ||
        pipeline.get_topology() != Frame::Pipeline::Topology::TriangleList ||
        pipeline.get_blend() != Frame::Pipeline::Blend::Alpha ||
        !supports_unit_quad(*description) ||
        batch.get_resources().get_size() !=
            description->descriptors.get_size() ||
        batch.get_inputs().get_size() != description->parameters_size ||
        batch.get_size_pixels().is_empty()) {
      return False;
    }

    const auto resources = batch.get_resources();
    for (Count index = 0; index < resources.get_size(); index++) {
      if (!resources.get_data()[index].is_drawable()) {
        return False;
      }
    }

    return True;
  }

  static auto accepts(Descriptions descriptions, Batches batches) -> Bool {
    for (Count index = 0; index < batches.get_size(); index++) {
      if (!accepts(descriptions, batches.get_data()[index])) {
        return False;
      }
    }

    return True;
  }
};

}  // namespace Photophore::Vulkan::Details
