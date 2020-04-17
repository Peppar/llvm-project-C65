//===- MCSectionWLAV.h - WLAV Machine Code Sections -------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file declares the MCSectionWLAV class.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_MC_MCSECTIONWLAV_H
#define LLVM_MC_MCSECTIONWLAV_H

#include "llvm/ADT/Twine.h"
#include "llvm/BinaryFormat/WLAV.h"
#include "llvm/MC/MCSection.h"
#include "llvm/MC/MCSymbolWLAV.h"

namespace llvm {

class MCSectionWLAV final : public MCSection {
  friend class MCContext;

  StringRef Name;

  MCSectionWLAV(StringRef Section, SectionKind K, MCSymbol *Begin)
      : MCSection(SV_WLAV, K, Begin), Name(Section) {
  }

public:
  ~MCSectionWLAV();

  static bool classof(const MCSection *S) {
    return S->getVariant() == SV_WLAV;
  }

  StringRef getSectionName() const { return Name; }

  void PrintSwitchToSection(const MCAsmInfo &MAI, const Triple &T,
                            raw_ostream &OS,
                            const MCExpr *Subsection) const override;
  bool UseCodeAlign() const override;
  bool isVirtualSection() const override;
};

} // end namespace llvm

#endif
