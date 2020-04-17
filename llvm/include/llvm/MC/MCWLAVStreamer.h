//===- lib/MC/MCWLAVStreamer.cpp - WLAV Object Output ---------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_MC_MCWLAVSTREAMER_H
#define LLVM_MC_MCWLAVSTREAMER_H

#include "llvm/MC/MCObjectStreamer.h"

namespace llvm {

class MCWLAVStreamer : public MCObjectStreamer {
public:
  MCWLAVStreamer(MCContext &Context, std::unique_ptr<MCAsmBackend> MAB,
                 std::unique_ptr<MCObjectWriter> OW,
                 std::unique_ptr<MCCodeEmitter> Emitter);

  bool emitSymbolAttribute(MCSymbol *Symbol, MCSymbolAttr Attribute) override;
  void emitCommonSymbol(MCSymbol *Symbol, uint64_t Size,
                        unsigned ByteAlignment) override;
  void emitZerofill(MCSection *Section, MCSymbol *Symbol = nullptr,
                    uint64_t Size = 0, unsigned ByteAlignment = 0,
                    SMLoc Loc = SMLoc()) override;
  void EmitInstToData(const MCInst &Inst, const MCSubtargetInfo &) override;
  //void emitXCOFFLocalCommonSymbol(MCSymbol *LabelSym, uint64_t Size,
  //                                MCSymbol *CsectSym,
  //                                unsigned ByteAlign) override;
};

} // end namespace llvm

#endif // LLVM_MC_MCWLAVSTREAMER_H
