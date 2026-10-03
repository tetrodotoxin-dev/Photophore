// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/vector.hpp"

#include "photophore/description/descriptor_binding.hpp"
#include "photophore/description/host_field.hpp"
#include "photophore/description/host_input_range.hpp"
#include "photophore/description/module.hpp"
#include "photophore/description/vertex_input.hpp"

namespace Photophore::Description {

// Program is the input to Vulkan pipeline construction. A compiler or another
// producer supplies executable SPIR V modules together with the resource,
// vertex and host input layouts those modules expect. Draws select this record
// by locator and supply their changing values through Frame::Batch.
//
// All arrays and names are borrowed. Pipelines retains pointers into this
// inventory, so the complete description must outlive that Pipelines owner.
// The locator is an opaque address token, compared without dereferencing it,
// and must be unique within the inventory. host_size covers the entire push
// input block, with parameters_offset and parameters_size locating the bytes
// copied from each batch. HostRole fields describe the placement values filled
// by the renderer around that parameter region.
struct Program {
  const U8* locator = nullptr;
  Perimortem::Core::View::Vector<Module> modules;
  Perimortem::Core::View::Vector<HostInputRange> host_input_ranges;
  Perimortem::Core::View::Vector<DescriptorBinding> descriptors;
  Perimortem::Core::View::Vector<HostField> host_fields;
  Perimortem::Core::View::Vector<VertexInput> vertex_inputs;
  Count host_size = 0;
  Count parameters_offset = 0;
  Count parameters_size = 0;
  Bool requires_float64 = False;
};

}  // namespace Photophore::Description
