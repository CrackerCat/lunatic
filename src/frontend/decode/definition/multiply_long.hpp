/*
 * Copyright (C) 2021 fleroviux. All rights reserved.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#pragma once

#include "common.hpp"

namespace lunatic {
namespace frontend {

struct ARMMultiplyLong {
  Condition condition;
  bool sign_extend;
  bool accumulate;
  bool set_flags;
  GPR reg_op1;
  GPR reg_op2;
  GPR reg_dst_lo;
  GPR reg_dst_hi;
};

} // namespace lunatic::frontend
} // namespace lunatic
