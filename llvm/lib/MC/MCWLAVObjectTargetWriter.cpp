//===-- MCWLAVObjectTargetWriter.cpp - WLAV Target Writer Subclass --------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "llvm/MC/MCWLAVObjectWriter.h"

using namespace llvm;

MCWLAVObjectTargetWriter::MCWLAVObjectTargetWriter() = default;

// Pin the vtable to this object file
MCWLAVObjectTargetWriter::~MCWLAVObjectTargetWriter() = default;
