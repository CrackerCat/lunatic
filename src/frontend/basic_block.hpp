/*
 * Copyright (C) 2021 fleroviux. All rights reserved.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#pragma once

#include <lunatic/integer.hpp>
#include <vector>

#include "common/pool_allocator.hpp"
#include "decode/definition/common.hpp"
#include "ir/emitter.hpp"
#include "state.hpp"

namespace lunatic {
namespace frontend {

struct BasicBlock : PoolObject {
  using CompiledFn = uintptr;

  union Key {
    Key() {}

    Key(State& state) {
      value  = state.GetGPR(Mode::User, GPR::PC) >> 1;
      value |= u64(state.GetCPSR().v & 0x3F) << 31; // mode and thumb bit
    }

    Key(u64 value) : value(value) {}

    Key(u32 address, Mode mode, bool thumb) {
      value  = address >> 1;
      value |= static_cast<u64>(mode) << 31;
      if (thumb) {
        value |= 1ULL << 36;
      }
    }

    auto Address() -> u32 { return (value & 0x7FFFFFFF) << 1; }
    auto Mode() -> Mode { return static_cast<lunatic::Mode>((value >> 31) & 0x1F); }
    bool Thumb() { return value & (1ULL << 36); }
    
    // bits  0 - 30: address[31:1]
    // bits 31 - 35: CPU mode
    // bit       36: thumb-flag
    u64 value = 0;
  } key;


  BasicBlock() {}
  BasicBlock(Key key) : key(key) {}

 ~BasicBlock() {
    // TODO: release the underlying JIT memory.
  }

  int length = 0;

  struct MicroBlock {
    Condition condition;
    IREmitter emitter;
    int length = 0;
  };

  std::vector<MicroBlock> micro_blocks;

  // Pointer to the compiled code.
  CompiledFn function = (CompiledFn)0;

  // TODO: clean this up
  struct BranchTarget {
    Key key{};
  } branch_target;


  u32 hash = 0;
  bool enable_fast_dispatch = true;
};

} // namespace lunatic::frontend
} // namespace lunatic
