//===--- C65.cpp - Implement C65 target feature support -------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file implements C65 TargetInfo objects.
//
//===----------------------------------------------------------------------===//

#include "C65.h"
#include "clang/Basic/Builtins.h"
#include "clang/Basic/LangOptions.h"
#include "clang/Basic/MacroBuilder.h"
#include "clang/Basic/TargetBuiltins.h"
#include "llvm/ADT/StringSwitch.h"

using namespace clang;
using namespace clang::targets;

namespace {
// If you edit the description strings, make sure you update
// getPointerWidthV().
const char *DataLayoutString6502 =
  "e-m:e-p:16:8-i16:8-i32:8-i64:8-n8:16:32:64-S8";
const char *DataLayoutString65816 =
  "e-m:e-p:16:8-p1:32:8-i16:8-i32:8-i64:8-n8:16:32:64-S8";

// const unsigned C65AddrSpaceMap[] = {
//   1, // Far addresses (opencl_global)
//   0, // opencl_local
//   0, // opencl_constant
//   0, // cuda_device
//   0, // cuda_constant
//   0  // cuda_shared
// };

} // namespace

C65TargetInfo::C65TargetInfo(const llvm::Triple &Triple)
  : TargetInfo(Triple) {
  assert(getTriple().getOS() == llvm::Triple::UnknownOS &&
         "C65 target must use unknown OS");
  assert(getTriple().getEnvironment() == llvm::Triple::UnknownEnvironment &&
         "C65 target must use unknown environment type");
  BigEndian = false;
  TLSSupported = false;

  BoolWidth = 8;
  IntWidth = 16;
  LongWidth = 32;
  LongLongWidth = 64;
  BoolAlign = IntAlign = LongAlign = LongLongAlign = 8;

  PointerWidth = 32;
  PointerAlign = 8;

  // Define available target features
  // These must be defined in sorted order!
  NoAsmVariants = true;
  // Use 16 bits here, since memory spans across bank (64K)
  // boundaries are meaningless.
  SizeType      = TargetInfo::UnsignedInt;
  PtrDiffType   = TargetInfo::SignedInt;

  // Use 32 bits here, otherwise far pointers are truncated to 16 bits.
  IntPtrType    = TargetInfo::SignedLong;
  // 65816 by default.
  //    DescriptionString = DescriptionString65816;

  // C65 uses address space maps to distinguish near and far
  // pointers.
  // AddrSpaceMap = &C65AddrSpaceMap;
  // UseAddrSpaceMapMangling = true;
  resetDataLayout(DataLayoutString65816);
}

uint64_t C65TargetInfo::getPointerWidthV(unsigned AddrSpace) const {
  if (AddrSpace == 1)
    return 32; // 24
  else
    return 16;
}

void C65TargetInfo::getTargetDefines(const LangOptions &Opts,
                                     MacroBuilder &Builder) const {
  Builder.defineMacro("__c65__");
  switch(CPU) {
  case CK_6502:
    Builder.defineMacro("__6502__");
    break;
  case CK_65C02:
    Builder.defineMacro("__65C02__");
    break;
  case CK_65802:
    Builder.defineMacro("__65802__");
    break;
  default:
  case CK_65816:
    Builder.defineMacro("__65816__");
    break;
  }
}

bool C65TargetInfo::hasFeature(StringRef Feature) const {
  return Feature == "c65";
}

ArrayRef<Builtin::Info> C65TargetInfo::getTargetBuiltins() const {
  return None;
}

const char *C65TargetInfo::getClobbers() const {
  return "";
}

ArrayRef<TargetInfo::GCCRegAlias> C65TargetInfo::getGCCRegAliases() const {
  return None;
}

ArrayRef<const char *> C65TargetInfo::getGCCRegNames() const {
  return None;
}

bool C65TargetInfo::validateAsmConstraint(const char *&Name,
                                          TargetInfo::ConstraintInfo &info) const {
  return true;
}

TargetInfo::BuiltinVaListKind C65TargetInfo::getBuiltinVaListKind() const {
  return TargetInfo::VoidPtrBuiltinVaList;
}

bool C65TargetInfo::setCPU(const std::string &Name) {
  resetDataLayout(DataLayoutString65816);
  CPUKind MatchedCPU = llvm::StringSwitch<CPUKind>(Name)
    .Case("6502" , CK_6502)
    .Case("65C02", CK_65C02)
    .Case("65802", CK_65802)
    .Case("65816", CK_65816)
    .Default(CK_NONE);
  if (MatchedCPU == CK_NONE)
    return false;
  CPU = MatchedCPU;
  if (CPU == CK_65816)
    resetDataLayout(DataLayoutString65816);
  else
    resetDataLayout(DataLayoutString6502);
  return true;
}

