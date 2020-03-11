//===- llvm/BinaryFormat/WLAV.h - WLAV constants ----------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This header contains constants for the WLAV object file format.
//
//===----------------------------------------------------------------------===//

namespace llvm {
namespace WLAV {

enum {
  R_DIRECT_8BIT,
  R_DIRECT_16BIT,
  R_DIRECT_24BIT,
  R_RELATIVE_8BIT,
  R_RELATIVE_16BIT,
};

enum {
  SECTION_FREE = 0,
  SECTION_FORCED = 1,
  SECTION_OVERWRITE = 2,
  SECTION_HEADER = 3,
  SECTION_SEMIFREE = 4,
  SECTION_ABSOLUTE = 5,
  SECTION_RAM = 6,
  SECTION_SUPERFREE = 7,
};

enum {
  CALC_TYPE_VALUE = 0,
  CALC_TYPE_OPERATOR = 1,
  CALC_TYPE_STRING = 2,
  CALC_TYPE_STACK = 4,
};

enum {
  CALC_OP_ADD = 0,
  CALC_OP_SUB = 1,
  CALC_OP_MUL = 2,
  CALC_OP_OR = 5,
  CALC_OP_AND = 6,
  CALC_OP_DIVIDE = 7,
  CALC_OP_POWER = 8,
  CALC_OP_SHIFT_LEFT = 9,
  CALC_OP_SHIFT_RIGHT = 10,
  CALC_OP_MODULO = 11,
  CALC_OP_XOR = 12,
  CALC_OP_LOW_BYTE = 13,
  CALC_OP_HIGH_BYTE = 14,
};

} // end namespace WLAV
} // end namespace llvm
