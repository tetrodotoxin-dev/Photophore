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

// Program is the complete compiled input to one Vulkan pipeline family. It
// joins executable SPIR V modules with the descriptor, vertex and host layouts
// those modules consume. Frame::Batch selects this product through locator and
// supplies its changing resources, placement and parameter bytes.
//
// The caller lends every nested array, name and module for the lifetime of the
// Pipelines instance that receives this Program. locator is a stable address
// identity shared with Frame::Program and is unique within that inventory.
// Pipelines compares the address as a token while the referenced byte remains
// ordinary caller storage.
//
// host_size covers the complete push input block. parameters_offset and
// parameters_size locate the contiguous region copied from each batch, while
// HostField entries fill placement values around that region. HostInputRange
// entries publish the stage visibility expected by the compiled modules.
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
