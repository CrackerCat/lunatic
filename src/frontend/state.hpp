/*
 * Copyright (C) 2021 fleroviux. All rights reserved.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#pragma once

#include <lunatic/cpu.hpp>
#include <string>

namespace lunatic {
namespace frontend {

/// Stores the state of the emulated ARM core.
struct State {
  State();

  /// Reset the ARM core.
  void Reset();

  /// \returns for a given processor mode a reference to a general-purpose register.
  auto GetGPR(Mode mode, GPR reg) -> u32& { return *GetPointerToGPR(mode, reg); }

  /// \returns reference to the current program status register (cpsr).
  auto GetCPSR() -> StatusRegister& { return common.cpsr; }

  /// \returns for a given processor mode the pointer to a general-purpose register.
  auto GetPointerToGPR(Mode mode, GPR reg) -> u32*;

  /// \returns for a given processor mode the pointer to the saved program status register (spsr).
  auto GetPointerToSPSR(Mode mode) -> StatusRegister*;

  /// \returns pointer to the current program status register (cpsr).
  auto GetPointerToCPSR() -> StatusRegister*;

  /// \returns for a given processor mode the offset to the saved program status register (spsr).
  auto GetOffsetToSPSR(Mode mode) -> uintptr;

  /// \returns the offset to the current program status register (cpsr).
  auto GetOffsetToCPSR() -> uintptr;

  /// \returns for a given processor mode the offset of a general-purpose register.
  auto GetOffsetToGPR(Mode mode, GPR reg) -> uintptr;

private:
  void InitializeLookupTable();

  /// Common registers r0 - r7, r15 and cpsr.
  /// These registers are visible in all ARM processor modes.
  struct {
    u32 reg[8] {0};
    u32 r15 = 0x00000008;
    StatusRegister cpsr = {};
  } common;

  /// Banked registers r8 - r14 and spsr for FIQ and User/System processor modes.
  /// User/System mode r8 - r12 are also used by all other modes except FIQ.
  struct {
    u32 reg[7] {0};
    StatusRegister spsr = {};
  } fiq, sys;

  /// Banked registers r13 - r14 and spsr for IRQ, supervisor,
  /// abort and undefined processor modes.
  struct {
    u32 reg[2] {0};
    StatusRegister spsr = {};
  } irq, svc, abt, und;

  /// Per ARM processor mode address/pointer lookup table for GPRs and SPSRs.
  struct LookupTable {
    u32* gpr[16] {nullptr};
    StatusRegister* spsr {nullptr};
  } table[0x20] = {};
};

} // namespace lunatic::frontend
} // namespace lunatic

namespace std {

// TODO: move this somewhere more appropriate?
inline auto to_string(lunatic::Mode value) -> std::string {
  using Mode = lunatic::Mode;

  switch (value) {
    case Mode::User: return "usr";
    case Mode::FIQ: return "fiq";
    case Mode::IRQ: return "irq";
    case Mode::Supervisor: return "svc";
    case Mode::Abort: return "abt";
    case Mode::Undefined: return "und";
    case Mode::System: return "sys";
  }

  return "???";
}

} // namespace std
