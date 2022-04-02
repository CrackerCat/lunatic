/*
 * Copyright (C) 2021 fleroviux. All rights reserved.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "frontend/translator/translator.hpp"

namespace lunatic {
namespace frontend {

auto Translator::Handle(ARMSingleDataTransfer const& opcode) -> Status {
  if (!opcode.pre_increment && opcode.writeback) {
    // LDRT and STRT are not supported right now.
    return Status::Unimplemented;
  }

  auto offset = IRAnyRef{};
  bool might_be_haltcnt_write = false;

  if (opcode.immediate) {
    offset = IRConstant{opcode.offset_imm};
    might_be_haltcnt_write = !opcode.load && opcode.byte && opcode.offset_imm == 0x301;

    // Detect PC-relative load from a region known to be ROM.
    if (opcode.reg_base == GPR::PC && !opcode.writeback && opcode.pre_increment && opcode.load) {
      u32 address = (code_address & ~3) + opcode_size * 2 + opcode.offset_imm;

      if (address >= 0x08000000 && address <= 0x09FFFFFF) {
        auto& data = emitter->CreateVar(IRDataType::UInt32, "data");
        if (opcode.byte) {
          emitter->MOV(data, IRConstant{memory.FastRead<u8 , Memory::Bus::Data>(address)}, false); 
        } else {
          emitter->MOV(data, IRConstant{memory.FastRead<u32, Memory::Bus::Data>(address)}, false);
        }
        emitter->StoreGPR(IRGuestReg{opcode.reg_dst, mode}, data);
        EmitAdvancePC();
        micro_block->data_cycles++;
        return Status::Continue;
      }
    }
  } else {
    auto& offset_reg = emitter->CreateVar(IRDataType::UInt32, "base_offset_reg");
    auto& offset_var = emitter->CreateVar(IRDataType::UInt32, "base_offset_shifted");

    emitter->LoadGPR(IRGuestReg{opcode.offset_reg.reg, mode}, offset_reg);

    // TODO: write a helper method to make this pattern less verbose.
    // Or alternatively pass the shift type to the emitter directly?
    switch (opcode.offset_reg.shift) {
      case Shift::LSL: {
        emitter->LSL(offset_var, offset_reg, IRConstant{opcode.offset_reg.amount}, false);
        break;
      }
      case Shift::LSR: {
        emitter->LSR(offset_var, offset_reg, IRConstant{opcode.offset_reg.amount}, false);
        break;
      }
      case Shift::ASR: {
        emitter->ASR(offset_var, offset_reg, IRConstant{opcode.offset_reg.amount}, false);
        break;
      }
      case Shift::ROR: {
        emitter->ROR(offset_var, offset_reg, IRConstant{opcode.offset_reg.amount}, false);
        break;
      }
    }

    offset = offset_var;
  }

  auto& base_old = emitter->CreateVar(IRDataType::UInt32, "base_old");
  auto& base_new = emitter->CreateVar(IRDataType::UInt32, "base_new");

  if (opcode.reg_base == GPR::PC) {
    /* This handles an edge-case in PC-relative loads in Thumb-mode.
     * The value of PC will be word-aligned before forming the final address,
     * so that no rotated read will happen.
     */
    emitter->MOV(base_old, IRConstant{(code_address & ~3) + opcode_size * 2}, false);
  } else {
    emitter->LoadGPR(IRGuestReg{opcode.reg_base, mode}, base_old);
  }

  if (opcode.add) {
    emitter->ADD(base_new, base_old, offset, false);
  } else {
    emitter->SUB(base_new, base_old, offset, false);
  }

  auto& address = opcode.pre_increment ? base_new : base_old;

  EmitAdvancePC();

  auto writeback = [&]() {
    if (!opcode.pre_increment || opcode.writeback) {
      emitter->StoreGPR(IRGuestReg{opcode.reg_base, mode}, base_new);
    }
  };

  // TODO: maybe it's cleaner to decode opcode into an enum upfront.
  if (opcode.load) {
    auto& data = emitter->CreateVar(IRDataType::UInt32, "data");

    writeback();

    if (opcode.byte) {
      emitter->LDR(Byte, data, address);
    } else {
      emitter->LDR(Word | Rotate, data, address);
    }

    emitter->StoreGPR(IRGuestReg{opcode.reg_dst, mode}, data);
  } else {
    auto& data = emitter->CreateVar(IRDataType::UInt32, "data");

    emitter->LoadGPR(IRGuestReg{opcode.reg_dst, mode}, data);

    if (opcode.byte) {
      emitter->STR(Byte, data, address);
    } else {
      emitter->STR(Word, data, address);
    }

    writeback();
  }

  micro_block->data_cycles++;

  if (opcode.load && opcode.reg_dst == GPR::PC) {
    if (armv5te) {
      // Branch with exchange
      auto& address = emitter->CreateVar(IRDataType::UInt32, "address");
      emitter->LoadGPR(IRGuestReg{GPR::PC, mode}, address);
      EmitFlushExchange(address);
    } else {
      EmitFlushNoSwitch();
    }
    return Status::BreakBasicBlock;
  }

  if (might_be_haltcnt_write) {
    basic_block->enable_fast_dispatch = false;
    return Status::BreakBasicBlock;
  }

  return Status::Continue;
}

} // namespace lunatic::frontend
} // namespace lunatic
