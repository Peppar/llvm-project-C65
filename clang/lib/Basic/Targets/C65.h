//===--- C65.h - Declare C65 target feature support -------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file declares C65 TargetInfo objects.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_LIB_BASIC_TARGETS_C65_H
#define LLVM_CLANG_LIB_BASIC_TARGETS_C65_H

#include "clang/Basic/TargetInfo.h"
#include "clang/Basic/TargetOptions.h"
#include "llvm/ADT/Triple.h"
#include "llvm/Support/Compiler.h"

namespace clang {
namespace targets {

class LLVM_LIBRARY_VISIBILITY C65TargetInfo : public TargetInfo {

  enum CPUKind {
    CK_NONE,
    CK_6502,
    CK_65C02,
    CK_65802,
    CK_65816
  } CPU;

public:
  C65TargetInfo(const llvm::Triple &Triple);
  uint64_t getPointerWidthV(unsigned AddrSpace) const override;
  void getTargetDefines(const LangOptions &Opts,
                        MacroBuilder &Builder) const override;
  bool hasFeature(StringRef Feature) const override;
  ArrayRef<Builtin::Info> getTargetBuiltins() const override;
  const char *getClobbers() const override;
  ArrayRef<TargetInfo::GCCRegAlias> getGCCRegAliases() const override;
  ArrayRef<const char *> getGCCRegNames() const override;
  bool validateAsmConstraint(const char *&Name,
                             TargetInfo::ConstraintInfo &info) const override;
  BuiltinVaListKind getBuiltinVaListKind() const override;
  bool setCPU(const std::string &Name) override;
};

} // namespace targets
} // namespace clang

#endif

