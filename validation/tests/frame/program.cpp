// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "photophore/frame/program.hpp"

#include "toolchain/validation/test.hpp"

#include "perimortem/core/identity/uuid.hpp"

using namespace Perimortem::Core;
using namespace Photophore::Frame;
using namespace Toolchain::Validation;

static Harness FrameProgram = {
  .name = "Photophore::Frame::Program",
};

// Program equality follows the authored UUID value. It has no relationship to
// the address of either Program or the description selected by that value.
VALIDATION_TEST(FrameProgram, explicit_identity) {
  constexpr Identity::Uuid first_identity(
      0x51dbd870cf5841a4ULL, 0x82250967a5210d8fULL);
  constexpr Identity::Uuid second_identity(
      0xa11af3db39634a79ULL, 0x8521bef0ed08e3d2ULL);

  constexpr Program unavailable;
  constexpr Program first(first_identity);
  constexpr Program same(first_identity);
  constexpr Program second(second_identity);

  EXPECT_NOT(unavailable.is_valid());
  EXPECT(first.is_valid());
  EXPECT(first == same);
  EXPECT_NOT(first == second);
  EXPECT(first.get_identity() == first_identity);
}
