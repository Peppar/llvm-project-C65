//===- MCAsmInfoWLAV.h - WLAV asm properties ------------------- *- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_MC_MCASMINFOWLAV_H
#define LLVM_MC_MCASMINFOWLAV_H

#include "llvm/MC/MCAsmInfo.h"

namespace llvm {

class MCAsmInfoWLAV : public MCAsmInfo {
  virtual void anchor();

protected:
  MCAsmInfoWLAV();
};

} // end namespace llvm

#endif // LLVM_MC_MCASMINFOWLAV_H
