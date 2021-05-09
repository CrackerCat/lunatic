/*
 * Copyright (C) 2021 fleroviux. All rights reserved.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "register_allocator.hpp"

using namespace lunatic::frontend;
using namespace Xbyak::util;

namespace lunatic {
namespace backend {

X64RegisterAllocator::X64RegisterAllocator(IREmitter const& emitter) : emitter(emitter) {
  // rax and rcx are statically allocated
  free_list = {
    edx,
    ebx,
    esi,
    edi,
    ebp,
    r8d,
    r9d,
    r10d,
    r11d,
    r12d,
    r13d,
    r14d,
    r15d
  };

  CreateVariableExpirationPoints();
}

auto X64RegisterAllocator::GetReg32(IRVariable const& var, int location) -> Xbyak::Reg32 {
  auto match = allocation.find(var.id);

  // Check if the variable is already allocated to a register at the moment.
  if (match != allocation.end()) {
    return match->second;
  }

  // TODO: defer this to the point where we'd need to spill otherwise.
  ExpireVariables(location);

  if (free_list.size() != 0) {
    auto reg = free_list.back();
    allocation[var.id] = reg;
    free_list.pop_back();
    return reg;
  }

  throw std::runtime_error("X64RegisterAllocator: failed to allocate register");
}

void X64RegisterAllocator::CreateVariableExpirationPoints() {
  for (auto const& var : emitter.Vars()) {
    int expiration_point = -1;
    int location = 0;

    for (auto const& op : emitter.Code()) {
      if (op->Writes(*var) || op->Reads(*var)) {
        expiration_point = location;
      }

      location++;
    }

    if (expiration_point != -1) {
      expiration_points[var->id] = expiration_point;
    }
  }
}

void X64RegisterAllocator::ExpireVariables(int location) {
  for (auto const& var : emitter.Vars()) {
    auto expiration_point = expiration_points[var->id];

    if (location > expiration_point) {
      auto match = allocation.find(var->id);
      if (match != allocation.end()) {
        auto reg = match->second;
        free_list.push_back(reg);
        allocation.erase(match);
      }
    }
  }
}

} // namespace lunatic::backend
} // namespace lunatic
