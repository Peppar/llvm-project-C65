//===- lib/MC/MCWLAVStreamer.cpp - WLAV Object Output ---------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file assembles .s files and emits WLAV .o object files.
//
//===----------------------------------------------------------------------===//

#include "llvm/BinaryFormat/WLAV.h"
#include "llvm/MC/MCAsmBackend.h"
#include "llvm/MC/MCCodeEmitter.h"
#include "llvm/MC/MCObjectWriter.h"
#include "llvm/MC/MCSymbolWLAV.h"
#include "llvm/MC/MCWLAVStreamer.h"
#include "llvm/Support/TargetRegistry.h"

using namespace llvm;

MCWLAVStreamer::MCWLAVStreamer(MCContext &Context,
                               std::unique_ptr<MCAsmBackend> MAB,
                               std::unique_ptr<MCObjectWriter> OW,
                               std::unique_ptr<MCCodeEmitter> Emitter)
    : MCObjectStreamer(Context, std::move(MAB), std::move(OW),
                       std::move(Emitter)) {}

bool MCWLAVStreamer::emitSymbolAttribute(MCSymbol *Sym,
                                         MCSymbolAttr Attribute) {
  //auto *Symbol = cast<MCSymbolWLAV>(Sym);
  getAssembler().registerSymbol(*Sym);

  switch (Attribute) {
  case MCSA_Global:
    //Symbol->setStorageClass(WLAV::C_EXT);
    Sym->setExternal(true);
    break;
  default:
    report_fatal_error("Not implemented yet.");
  }
  return true;
}

void MCWLAVStreamer::emitCommonSymbol(MCSymbol *Symbol, uint64_t Size,
                                      unsigned ByteAlignment) {
  getAssembler().registerSymbol(*Symbol);
  //Symbol->setExternal(cast<MCSymbolWLAV>(Symbol)->getStorageClass() !=
  //                    WLAV::C_HIDEXT);
  Symbol->setCommon(Size, ByteAlignment);

  // Emit the alignment and storage for the variable to the section.
  emitValueToAlignment(ByteAlignment);
  emitZeros(Size);
}

void MCWLAVStreamer::emitZerofill(MCSection *Section, MCSymbol *Symbol,
                                  uint64_t Size, unsigned ByteAlignment,
                                  SMLoc Loc) {
  report_fatal_error("Zero fill not implemented for WLAV.");
}

void MCWLAVStreamer::EmitInstToData(const MCInst &Inst,
                                     const MCSubtargetInfo &STI) {
  MCAssembler &Assembler = getAssembler();
  SmallVector<MCFixup, 4> Fixups;
  SmallString<256> Code;
  raw_svector_ostream VecOS(Code);
  Assembler.getEmitter().encodeInstruction(Inst, VecOS, Fixups, STI);

  // Add the fixups and data.
  MCDataFragment *DF = getOrCreateDataFragment(&STI);
  const size_t ContentsSize = DF->getContents().size();
  auto &DataFragmentFixups = DF->getFixups();
  for (auto &Fixup : Fixups) {
    Fixup.setOffset(Fixup.getOffset() + ContentsSize);
    DataFragmentFixups.push_back(Fixup);
  }

  DF->setHasInstructions(STI);
  DF->getContents().append(Code.begin(), Code.end());
}

MCStreamer *llvm::createWLAVStreamer(MCContext &Context,
                                     std::unique_ptr<MCAsmBackend> &&MAB,
                                     std::unique_ptr<MCObjectWriter> &&OW,
                                     std::unique_ptr<MCCodeEmitter> &&CE,
                                     bool RelaxAll) {
  MCWLAVStreamer *S = new MCWLAVStreamer(Context, std::move(MAB),
                                         std::move(OW), std::move(CE));
  if (RelaxAll)
    S->getAssembler().setRelaxAll(true);
  return S;
}

//void MCWLAVStreamer::emitWLAVLocalCommonSymbol(MCSymbol *LabelSym,
//                                                 uint64_t Size,
//                                                 MCSymbol *CsectSym,
//                                                 unsigned ByteAlignment) {
//  emitCommonSymbol(CsectSym, Size, ByteAlignment);
//}
