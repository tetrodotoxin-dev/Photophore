// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/identity/uuid.hpp"
#include "perimortem/core/view/vector.hpp"

#include "photophore/description/descriptor_binding.hpp"
#include "photophore/description/host_field.hpp"
#include "photophore/description/host_input_range.hpp"
#include "photophore/description/module.hpp"
#include "photophore/description/vertex_input.hpp"

namespace Photophore::Description {

// Program is the complete compiled input to one Vulkan pipeline family. It
// joins executable SPIR V modules with the descriptor, vertex and host layouts
// those modules consume. Frame::Batch selects this product through identity and
// supplies its changing resources, placement and parameter bytes.
//
// Every nested array, name and module remains accessible while a Pipelines
// instance uses this Program. The producer assigns a nonzero identity that is
// unique within that inventory. Frame::Program carries the same value without
// acquiring the description or its nested storage.
//
// `host_size` covers the complete push input block. `parameters_offset` and
// `parameters_size` locate the contiguous region copied from each batch, while
// HostField entries fill placement values around that region. HostInputRange
// entries publish the stage visibility expected by the compiled modules.
struct Program {
  Perimortem::Core::Identity::Uuid identity;
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
