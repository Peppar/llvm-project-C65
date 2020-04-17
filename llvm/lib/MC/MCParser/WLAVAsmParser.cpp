//===- WLAVAsmParser.cpp - WLAV Assembly Parser ---------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "llvm/ADT/StringRef.h"
#include "llvm/ADT/StringSwitch.h"
#include "llvm/BinaryFormat/WLAV.h"
#include "llvm/MC/MCAsmInfo.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCDirectives.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCParser/MCAsmLexer.h"
#include "llvm/MC/MCParser/MCAsmParser.h"
#include "llvm/MC/MCParser/MCAsmParserExtension.h"
#include "llvm/MC/MCSection.h"
#include "llvm/MC/MCSectionWLAV.h"
#include "llvm/MC/MCStreamer.h"
#include "llvm/MC/MCSymbol.h"
#include "llvm/MC/MCSymbolWLAV.h"
#include "llvm/MC/SectionKind.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/MathExtras.h"
#include "llvm/Support/SMLoc.h"
#include <cassert>
#include <cstdint>
#include <utility>

using namespace llvm;

namespace {

class WLAVAsmParser : public MCAsmParserExtension {
  template<bool (WLAVAsmParser::*HandlerMethod)(StringRef, SMLoc)>
  void addDirectiveHandler(StringRef Directive) {
    MCAsmParser::ExtensionDirectiveHandler Handler = std::make_pair(
        this, HandleDirective<WLAVAsmParser, HandlerMethod>);

    getParser().addDirectiveHandler(Directive, Handler);
  }

  bool ParseSectionSwitch(StringRef Section, unsigned Type, unsigned Flags,
                          SectionKind Kind);

public:
  WLAVAsmParser() { BracketExpressionsSupported = true; }

  void Initialize(MCAsmParser &Parser) override {
    // Call the base implementation.
    this->MCAsmParserExtension::Initialize(Parser);

    addDirectiveHandler<&WLAVAsmParser::ParseSectionDirectiveData>(".data");
    addDirectiveHandler<&WLAVAsmParser::ParseSectionDirectiveText>(".text");
    addDirectiveHandler<&WLAVAsmParser::ParseSectionDirectiveBSS>(".bss");
  }

  bool ParseSectionDirectiveData(StringRef, SMLoc) {
    return ParseSectionSwitch(".data", SectionKind::getData());
  }

  bool ParseSectionDirectiveText(StringRef, SMLoc) {
    return ParseSectionSwitch(".text", SectionKind::getText());
  }

  bool ParseSectionDirectiveBSS(StringRef, SMLoc) {
    return ParseSectionSwitch(".bss", SectionKind::getBSS());
  }

  bool ParseSectionSwitch(StringRef Section, SectionKind Kind) {
    const MCExpr *Subsection = nullptr;
    if (getLexer().isNot(AsmToken::EndOfStatement)) {
      if (getParser().parseExpression(Subsection))
        return true;
    }
    Lex();

    getStreamer().SwitchSection(
        getContext().getWLAVSection(Section, Kind), Subsection);

    return false;
  }
};

} // end anonymous namespace

namespace llvm {

MCAsmParserExtension *createWLAVAsmParser() {
  return new WLAVAsmParser;
}

} // end namespace llvm
