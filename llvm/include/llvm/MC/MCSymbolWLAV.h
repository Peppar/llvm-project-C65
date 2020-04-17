//===- MCSymbolWLAV.h -  ----------------------------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#ifndef LLVM_MC_MCSYMBOLWLAV_H
#define LLVM_MC_MCSYMBOLWLAV_H
#include "llvm/ADT/Optional.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/BinaryFormat/WLAV.h"
#include "llvm/MC/MCSymbol.h"

namespace llvm {

class MCSectionWLAV;

class MCSymbolWLAV : public MCSymbol {
public:
  MCSymbolWLAV(const StringMapEntry<bool> *Name, bool isTemporary)
      : MCSymbol(SymbolKindWLAV, Name, isTemporary) {}

  static bool classof(const MCSymbol *S) { return S->isWLAV(); }

  bool isPrivate() const {
    return isTemporary() || (isDefined() && !isExternal());
  }

  bool isExported() const {
    return isInSection() && !getName().empty();
  }
};

} // end namespace llvm

#endif // LLVM_MC_MCSYMBOLWLAV_H
