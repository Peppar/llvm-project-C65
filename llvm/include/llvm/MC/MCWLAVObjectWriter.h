//===-- llvm/MC/MCWLAVObjectWriter.h - WLAV Object Writer -------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_MC_MCWLAVOBJECTWRITER_H
#define LLVM_MC_MCWLAVOBJECTWRITER_H

#include "llvm/MC/MCObjectWriter.h"
#include <memory>

namespace llvm {

class MCFixup;
class MCValue;
class raw_pwrite_stream;

class MCWLAVObjectTargetWriter : public MCObjectTargetWriter {
protected:
  MCWLAVObjectTargetWriter();

public:
  virtual ~MCWLAVObjectTargetWriter();

  virtual Triple::ObjectFormatType getFormat() const { return Triple::WLAV; }
  static bool classof(const MCObjectTargetWriter *W) {
    return W->getFormat() == Triple::WLAV;
  }

  virtual unsigned getRelocType(const MCFixup &Fixup) const = 0;

  virtual unsigned getFixupShiftAmt(const MCFixup &Fixup) const = 0;
};

/// Construct a new WLAV writer instance.
///
/// \param MOTW - The target specific WLAV writer subclass.
/// \param OS - The stream to write to.
/// \returns The constructed object writer.
std::unique_ptr<MCObjectWriter>
createWLAVObjectWriter(std::unique_ptr<MCWLAVObjectTargetWriter> MOTW,
                       raw_pwrite_stream &OS);

} // namespace llvm

#endif
