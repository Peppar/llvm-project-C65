//===- lib/MC/MCSectionWLAV.cpp - WLAV Code Section Representation --------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "llvm/MC/MCSectionWLAV.h"
#include "llvm/MC/MCAsmInfo.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

MCSectionWLAV::~MCSectionWLAV() = default;


void MCSectionWLAV::PrintSwitchToSection(const MCAsmInfo &MAI, const Triple &T,
                                         raw_ostream &OS,
                                         const MCExpr *Subsection) const {
  OS << '\t' << getSectionName();
  if (Subsection) {
    OS << '\t';
    Subsection->print(OS, &MAI);
  }
  OS << '\n';
  return;
}

bool MCSectionWLAV::UseCodeAlign() const { return getKind().isText(); }

bool MCSectionWLAV::isVirtualSection() const { return false; }
