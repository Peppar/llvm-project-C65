//===- MC/MCAsmInfoWLAV.cpp - WLAV asm properties -------------- *- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "llvm/MC/MCAsmInfoWLAV.h"

using namespace llvm;

void MCAsmInfoWLAV::anchor() {}

MCAsmInfoWLAV::MCAsmInfoWLAV() {
  IsLittleEndian = false;
  HasDotTypeDotSizeDirective = false;
  COMMDirectiveAlignmentIsInBytes = false;
  LCOMMDirectiveAlignmentType = LCOMM::Log2Alignment;
  UseDotAlignForAlignment = true;
  AsciiDirective = nullptr; // not supported
  AscizDirective = nullptr; // not supported
  HasDotLGloblDirective = true;
  Data64bitsDirective = "\t.dq\t";
  SupportsQuotedNames = false;
}
