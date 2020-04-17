//===-- WLAVObjectWriter.cpp - WLAV object writer -------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file implements WLAV object file writer information.
//
//===----------------------------------------------------------------------===//

#include "llvm/BinaryFormat/WLAV.h"
#include "llvm/MC/MCAsmLayout.h"
#include "llvm/MC/MCAssembler.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCObjectWriter.h"
#include "llvm/MC/MCSection.h"
#include "llvm/MC/MCSymbol.h"
#include "llvm/MC/MCValue.h"
#include "llvm/MC/MCWLAVObjectWriter.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/SourceMgr.h"

using namespace llvm;

namespace {

class WLAVSectionMap {
  // Maps MCSection to WLAV section ID.
  llvm::DenseMap<const MCSection *, unsigned> SectionIDMap;

  unsigned NextID;

public:
  WLAVSectionMap() : NextID(1) {}

  void addSection(const MCSection &Section) {
    SectionIDMap[&Section] = NextID++;
  }

  unsigned getSectionID(const MCSection &Section) const {
    return SectionIDMap.lookup(&Section);
  }
};

class WLAVSymbolMap {
  struct SymbolInfo {
    bool Exported;
    bool Private;
  };
  DenseMap<const MCSymbol *, SymbolInfo> SymbolInfoMap;

public:
  void addSymbol(const MCSymbol &Symbol) {
    SymbolInfo SI;
    SI.Exported = Symbol.isInSection() && !Symbol.getName().empty();
    SI.Private = Symbol.isDefined() && !Symbol.isExternal();
    SymbolInfoMap[&Symbol] = SI;
  }

  bool hasSymbol(const MCSymbol &Symb) const {
    return SymbolInfoMap.find(&Symb) != SymbolInfoMap.end();
  }

  bool isSymbolPrivate(const MCSymbol &Symb) const {
    assert(hasSymbol(Symb));
    return Symb.isTemporary() || SymbolInfoMap.lookup(&Symb).Private;
    // &&
    //       !SymbolInfoMap.lookup(&Symb).External);
  }

  // bool isSymbolExternal(const MCSymbol &Symb) const {
  //   return SymbolInfoMap.lookup(&Symb).External;
  // }

  void getExportedSymbols(SmallVectorImpl<const MCSymbol *> &Out) const {
    for (auto I : SymbolInfoMap) {
      if (I.second.Exported) {
        Out.push_back(I.first);
      }
    }
  }
};

// For the WLAV object file type, we need to add underscores to names that
// are not external and not guaranteed to be unique across all object
// files. This function serves this purpose.
//
void writeSymbolName(raw_ostream &OS, MCAssembler &Asm, const MCSymbol &Symbol,
                     const WLAVSymbolMap &SymbolMap) {
  // Append the file name to private symbols. We can't use underscores
  // here, since they are section-private and does not resolve across
  // sections.
  if (SymbolMap.isSymbolPrivate(Symbol)) {
    ArrayRef<std::string> FileNames = Asm.getFileNames();
    if (FileNames.begin() != FileNames.end()) {
      std::string Name = *FileNames.begin();
      for (auto I = Name.begin(), E = Name.end(); I != E; ++I) {
        if (*I == '_')
          OS << '~';
        else
          OS << *I;
      }
      OS << '~';
    } else {
      // With no file name defined, prepend an underscore.
      OS << '_';
    }
  }
  OS << Symbol.getName();
}

class WLAVCalcStackEntry {
  unsigned Type;
  unsigned Sign;
  // Source: wladx/wlalink/defines.h
  union {
    double Value;
    const MCSymbol *Symbol;
  };
  WLAVCalcStackEntry(double Imm)
      : Type(WLAV::CALC_TYPE_VALUE), Sign(0), Value(Imm){};
  WLAVCalcStackEntry(unsigned Op)
      : Type(WLAV::CALC_TYPE_OPERATOR), Sign(0), Value((double)Op){};
  WLAVCalcStackEntry(const MCSymbol &Symbol, unsigned Sign)
      : Type(WLAV::CALC_TYPE_STRING), Sign(Sign), Symbol(&Symbol){};

public:
  static WLAVCalcStackEntry createImm(int Imm) {
    return WLAVCalcStackEntry((double)Imm);
  }
  static WLAVCalcStackEntry createOp(unsigned Op) {
    return WLAVCalcStackEntry(Op);
  }
  static WLAVCalcStackEntry createSymb(const MCSymbol &Symbol,
                                       bool Invert = false) {
    return WLAVCalcStackEntry(Symbol, Invert ? 1 : 0);
  }
  void write(support::endian::Writer &W, MCAssembler &Asm,
             const WLAVSymbolMap &SymbolMap) const {
    W.write<uint8_t>(Type);
    W.write<uint8_t>(Sign);
    if (Type == WLAV::CALC_TYPE_VALUE || Type == WLAV::CALC_TYPE_OPERATOR) {
      W.write<double>(Value);
    } else {
      writeSymbolName(W.OS, Asm, *Symbol, SymbolMap);
      W.write<uint8_t>(0);
    }
  }
};

class WLAVRelocationEntry {
protected:
  const MCSection *Section;
  unsigned Type;
  unsigned FileID;
  unsigned LineNumber;
  unsigned Offset;

public:
  WLAVRelocationEntry(const MCSection &Section, unsigned Type, unsigned FileID,
                      unsigned LineNumber, unsigned Offset)
      : Section(&Section), Type(Type), FileID(FileID), LineNumber(LineNumber),
        Offset(Offset){};

  const MCSection &getSection() { return *Section; }
};

class WLAVComplexRelocationEntry : public WLAVRelocationEntry {
protected:
  std::vector<WLAVCalcStackEntry> Stack;

public:
  WLAVComplexRelocationEntry(const MCSection &Section, unsigned Type,
                             unsigned FileID, unsigned LineNumber,
                             unsigned Offset)
      : WLAVRelocationEntry(Section, Type, FileID, LineNumber, Offset){};

  void addImm(int Value) {
    Stack.push_back(WLAVCalcStackEntry::createImm(Value));
  }
  void addOp(unsigned Op) { Stack.push_back(WLAVCalcStackEntry::createOp(Op)); }
  void addSymb(const MCSymbol &Symbol) {
    Stack.push_back(WLAVCalcStackEntry::createSymb(Symbol));
  }

  void write(support::endian::Writer &W, MCAssembler &Asm,
             const WLAVSymbolMap &SymbolMap, const WLAVSectionMap &SectionMap,
             uint32_t ID) const {
    W.write<uint32_t>(ID);
    if (Type == WLAV::R_DIRECT_8BIT)
      W.write<uint8_t>(0x0);
    else if (Type == WLAV::R_DIRECT_16BIT)
      W.write<uint8_t>(0x1);
    else if (Type == WLAV::R_DIRECT_24BIT)
      W.write<uint8_t>(0x2);
    else if (Type == WLAV::R_RELATIVE_8BIT)
      W.write<uint8_t>(0x80);
    else /* Type == WLAV::R_RELATIVE_16BIT */
      W.write<uint8_t>(0x81);
    W.write<uint8_t>(0);
    W.write<uint32_t>(SectionMap.getSectionID(*Section));
    W.write<uint8_t>(FileID);
    W.write<uint8_t>(Stack.size());
    W.write<uint8_t>(0);
    W.write<uint32_t>(Offset);
    W.write<uint32_t>(LineNumber);
    for (const WLAVCalcStackEntry &E : Stack)
      E.write(W, Asm, SymbolMap);
  }
};

class WLAVSimpleRelocationEntry : public WLAVRelocationEntry {
protected:
  const MCSymbol *Symbol;

public:
  WLAVSimpleRelocationEntry(const MCSection &Section, unsigned Type,
                            unsigned FileID, unsigned LineNumber,
                            unsigned Offset, const MCSymbol &Symbol)
      : WLAVRelocationEntry(Section, Type, FileID, LineNumber, Offset),
        Symbol(&Symbol){};

  void write(support::endian::Writer &W, MCAssembler &Asm,
             const WLAVSymbolMap &SymbolMap,
             const WLAVSectionMap &SectionMap) const {
    writeSymbolName(W.OS, Asm, *Symbol, SymbolMap);
    W.write<uint8_t>(0);
    if (Type == WLAV::R_DIRECT_8BIT)
      W.write<uint8_t>(0x2);
    else if (Type == WLAV::R_DIRECT_16BIT)
      W.write<uint8_t>(0x0);
    else if (Type == WLAV::R_DIRECT_24BIT)
      W.write<uint8_t>(0x3);
    else if (Type == WLAV::R_RELATIVE_8BIT)
      W.write<uint8_t>(0x1);
    else /* Type == WLAV::R_RELATIVE_16BIT */
      W.write<uint8_t>(0x4);
    W.write<uint8_t>(0); /* Not a special case */
    W.write<uint32_t>(SectionMap.getSectionID(*Section));
    W.write<uint8_t>(FileID);
    W.write<uint32_t>(LineNumber);
    W.write<uint32_t>(Offset);
  }
};

class WLAVObjectWriter : public MCObjectWriter {
  support::endian::Writer W;

  std::unique_ptr<MCWLAVObjectTargetWriter> TargetObjectWriter;

  std::vector<WLAVSimpleRelocationEntry> SimpleRelocations;
  std::vector<WLAVComplexRelocationEntry> ComplexRelocations;

  WLAVSymbolMap SymbolMap;
  WLAVSectionMap SectionMap;

  // If we have symbols without file or line number information, they
  // are assigned this file ID. Set to 0 if no such symbols are
  // encountered, in which case this "unknown" file is not emitted to
  // the source file list.
  unsigned UnknownFileID;

  unsigned NextSourceID;
  SmallDenseMap<unsigned, unsigned> BufferIDMap;
  SmallDenseMap<unsigned, const char *> SourceFilenameMap;

  std::pair<unsigned, unsigned> getFileAndLine(MCAssembler &Asm,
                                               const MCFixup &Fixup);

  void getSections(MCAssembler &Asm, std::vector<const MCSection *> &Sections);

  void enumerateSections(MCAssembler &Asm, const MCAsmLayout &Layout);

public:
  WLAVObjectWriter(std::unique_ptr<MCWLAVObjectTargetWriter> MOTW,
                   raw_pwrite_stream &OS)
      : W(OS, support::big), TargetObjectWriter(std::move(MOTW)),
        UnknownFileID(0), NextSourceID(0) {}

  virtual ~WLAVObjectWriter() = default;

  /// \brief Record a relocation entry.
  ///
  /// This routine is called by the assembler after layout and relaxation, and
  /// post layout binding. The implementation is responsible for storing
  /// information about the relocation so that it can be emitted during
  /// WriteObject().
  void recordRelocation(MCAssembler &Asm, const MCAsmLayout &Layout,
                        const MCFragment *Fragment, const MCFixup &Fixup,
                        MCValue Target, uint64_t &FixedValue) override;

  /// \brief Perform any late binding of symbols (for example, to assign symbol
  /// indices for use when generating relocations).
  ///
  /// This routine is called by the assembler after layout and relaxation is
  /// complete.
  virtual void executePostLayoutBinding(MCAssembler &Asm,
                                        const MCAsmLayout &Layout) override;
  void writeSection(MCAssembler &Asm, const MCAsmLayout &Layout,
                    const MCSection &Section);

  void writeSymbol(MCAssembler &Asm, const MCAsmLayout &Layout,
                   const MCSymbol &Symbol);

  void writeSymbolTable(MCAssembler &Asm, const MCAsmLayout &Layout);

  void writeSourceFiles(MCAssembler &Asm, const MCAsmLayout &Layout);

  virtual uint64_t writeObject(MCAssembler &Asm,
                               const MCAsmLayout &Layout) override;
};
} // end anonymous namespace

std::pair<unsigned, unsigned>
WLAVObjectWriter::getFileAndLine(MCAssembler &Asm, const MCFixup &Fixup) {
  MCContext &Ctx = Asm.getContext();
  const SourceMgr *SrcMgr = Ctx.getSourceManager();
  if (!SrcMgr) {
    if (!UnknownFileID)
      UnknownFileID = NextSourceID++;
    return std::make_pair(UnknownFileID, 0);
  }

  SMLoc Loc = Fixup.getLoc();
  if (!Loc.isValid()) {
    if (!UnknownFileID)
      UnknownFileID = NextSourceID++;
    return std::make_pair(UnknownFileID, 0);
  }

  unsigned BufferID = SrcMgr->FindBufferContainingLoc(Loc);
  unsigned SourceID;

  // See if we have already assigned an ID for this buffer.
  auto I = BufferIDMap.find(BufferID);
  if (I != BufferIDMap.end()) {
    SourceID = I->second;
  } else {
    SourceID = NextSourceID++;
    BufferIDMap[BufferID] = SourceID;
  }
  unsigned LineNumber = SrcMgr->FindLineNumber(Loc, BufferID);
  return std::make_pair(SourceID, LineNumber);
}

void WLAVObjectWriter::recordRelocation(MCAssembler &Asm,
                                        const MCAsmLayout &Layout,
                                        const MCFragment *Fragment,
                                        const MCFixup &Fixup, MCValue Target,
                                        uint64_t &FixedValue) {
  const MCSection *Section = Fragment->getParent();
  std::pair<unsigned, unsigned> FileAndLine = getFileAndLine(Asm, Fixup);
  unsigned FileID = FileAndLine.first;
  unsigned LineNumber = FileAndLine.second;

  unsigned C = Target.getConstant();
  unsigned Offset = Layout.getFragmentOffset(Fragment) + Fixup.getOffset();
  unsigned Type = TargetObjectWriter->getRelocType(Fixup);
  unsigned ShiftAmt = TargetObjectWriter->getFixupShiftAmt(Fixup);

  const MCSymbolRefExpr *RefA = Target.getSymA();
  const MCSymbolRefExpr *RefB = Target.getSymB();
  const MCSymbol &SymA = RefA->getSymbol();

  if (ShiftAmt || RefB || C) {
    // WLAV supports arbitrary calculations for relocations using a
    // stack-based language.
    WLAVComplexRelocationEntry Rel(*Section, Type, FileID, LineNumber, Offset);
    Rel.addSymb(SymA);
    if (RefB) {
      assert(RefB->getKind() == MCSymbolRefExpr::VK_None &&
             "Should not have constructed this");

      const MCSymbol &SymB = RefB->getSymbol();
      assert(!SymB.isAbsolute() && "Should have been folded");

      Rel.addSymb(RefB->getSymbol());
      Rel.addOp(WLAV::CALC_OP_SUB);
    }
    if (C) {
      Rel.addImm(C);
      Rel.addOp(WLAV::CALC_OP_ADD);
    }
    if (ShiftAmt) {
      Rel.addImm(ShiftAmt);
      Rel.addOp(WLAV::CALC_OP_SHIFT_RIGHT);
    }
    ComplexRelocations.push_back(Rel);
  } else {
    WLAVSimpleRelocationEntry Rel(*Section, Type, FileID, LineNumber, Offset,
                                  SymA);
    SimpleRelocations.push_back(Rel);
  }
}

void WLAVObjectWriter::executePostLayoutBinding(MCAssembler &Asm,
                                                const MCAsmLayout &Layout) {
  for (const MCSymbol &SD : Asm.symbols()) {
    SymbolMap.addSymbol(SD);
  }
}

void WLAVObjectWriter::writeSection(MCAssembler &Asm, const MCAsmLayout &Layout,
                                    const MCSection &Section) {
  unsigned Size = Layout.getSectionFileSize(&Section);
  StringRef SectionName;
  SectionKind Kind = Section.getKind();

  if (Kind.isText())
    SectionName = StringRef("TEXT");
  else if (Kind.isData())
    SectionName = StringRef("DATA_REL");
  // else if (Kind.isDataRelLocal())
  //   SectionName = StringRef("DATA_REL_LOCAL");
  // else if (Kind.isDataNoRel())
  //   SectionName = StringRef("DATA_NOREL");
  else
    SectionName = StringRef("UNKNOWN");
  W.OS << SectionName;
  W.write<uint8_t>(WLAV::SECTION_FREE);
  W.OS << '\0'; /* Namespace */
  W.write<uint32_t>(SectionMap.getSectionID(Section)); /* Section ID */
  W.write<uint8_t>(1); /* FileID */
  W.write<uint32_t>(Size);
  W.write<uint32_t>(1); /* Alignment */
  W.write<uint32_t>(0); /* Priority */
  Asm.writeSectionData(W.OS, &Section, Layout);
  W.write<uint8_t>(0); // List file information, 0 - not present
}

void WLAVObjectWriter::writeSymbol(MCAssembler &Asm, const MCAsmLayout &Layout,
                                   const MCSymbol &Symbol) {
  writeSymbolName(W.OS, Asm, Symbol, SymbolMap);
  // String terminator decides type
  W.write<uint8_t>(0); // 0 - label, 1 - symbol, 2 - breakpoint
  W.write<uint32_t>(SectionMap.getSectionID(Symbol.getSection())); // Section ID
  W.write<uint8_t>(1);                                             // File ID
  W.write<uint32_t>(0); // Line number

  // Write symbol offset
  uint64_t Val;
  if (!Layout.getSymbolOffset(Symbol, Val))
    report_fatal_error("expected absolute expression");
  W.write<uint32_t>(Val);
}

void WLAVObjectWriter::writeSymbolTable(MCAssembler &Asm,
                                        const MCAsmLayout &Layout) {
  llvm::SmallVector<const MCSymbol *, 32> ExportedSymbols;

  SymbolMap.getExportedSymbols(ExportedSymbols);

  W.write<uint32_t>(ExportedSymbols.size());

  for (const MCSymbol *Symbol : ExportedSymbols) {
    writeSymbol(Asm, Layout, *Symbol);
  }

  // for (const MCSymbolData &SD : Asm.symbols()) {
  //   const MCSymbol &Symbol = SD.getSymbol();
  //   SymbolInfo SI;
  //   if (Symbol.isDefined() && Symbol.isInSection()) {
  //     SI.Exists = true;
  //     SI.External =
  //       SD.isExternal() ||
  //       (!SD.getFragment() && !SD.getSymbol().isVariable());
  //     SymbolInfoMap[&Symbol] = SI;
  //     NumSymbols++;
  //   }
  // }
  // for (const MCSymbolData &SD : Asm.symbols()) {
  //   const MCSymbol &Symbol = SD.getSymbol();
  //   if (Symbol.isDefined()) {
  //     SymbolInfo SI;
  //     SI.Exists = true;
  //     SI.Exported = !Symbol.isTemporary() ||
  //       (Symbol.isInSection() && !Symbol.isVariable());
  //     SI.Local = !SD.isExternal() && Symbol.isInSection();
  //     SI.External = SD.isExternal() ||
  //       (!SD.getFragment() && !Symbol.isVariable());
  //       // SD.isExternal() ||
  //       // (!Symbol.isInSection() && !SD.getFragment() &&
  //       //  !SD.getSymbol().isVariable());
  //     SymbolInfoMap[&Symbol] = SI;
  //   }
  // }
}

void WLAVObjectWriter::enumerateSections(MCAssembler &Asm,
                                         const MCAsmLayout &Layout) {
  for (const MCSection &Section : Asm)
    SectionMap.addSection(Section);
}

void WLAVObjectWriter::writeSourceFiles(MCAssembler &Asm,
                                        const MCAsmLayout &Layout) {
  // If there is no buffer information, try to use the file names
  // supplied in Asm.
  if (BufferIDMap.empty()) {
    // In case there are no symbols, UnknownFileID will be empty even
    // here.
    if (!UnknownFileID)
      UnknownFileID = NextSourceID++;

    //    unsigned SourceFileCount = 0;
    ArrayRef<std::string> FileNames = Asm.getFileNames();
    if (FileNames.begin() != FileNames.end() &&
        FileNames.begin() + 1 == FileNames.end()) {
      // There is only one file name, use it as the "unknown file".
      W.write<uint32_t>(1);
      W.OS << *FileNames.begin() << '\0';
      W.write<uint8_t>(UnknownFileID);
      W.write<uint32_t>(0); // File checksum
      return;
    }
  }

  // unsigned SourceFileCount = 0;
  // for (auto I = Asm.file_names_begin(), E = Asm.file_names_end();
  //      I != E; ++I)
  //   ++SourceFileCount;
  // W.write<uint32_t>(SourceFileCount);
  // assert(SourceFileCount == (Asm.file_names_end() - Asm.file_names_begin()));
  // assert(SourceFileCount < 255);

  //  BufferIDMap[&SD.getSection()] = NextID++;

  MCContext &Ctx = Asm.getContext();
  const SourceMgr *SrcMgr = Ctx.getSourceManager();
  if (UnknownFileID)
    W.write<uint32_t>(BufferIDMap.size() + 1);
  else
    W.write<uint32_t>(BufferIDMap.size());

  for (auto I = BufferIDMap.begin(), E = BufferIDMap.end(); I != E; ++I) {
    unsigned BufferID = I->first;
    StringRef Identifier;
    if (SrcMgr) {
      const MemoryBuffer *MemBuff = SrcMgr->getMemoryBuffer(BufferID);
      if (MemBuff)
        Identifier = MemBuff->getBufferIdentifier();
    }
    if (!Identifier.empty())
      W.OS << Identifier << '\0';
    else
      W.OS << "anonymous file " << BufferID << '\0';
    W.write<uint8_t>(I->second);
    W.write<uint32_t>(0); // File checksum
  }
  if (UnknownFileID) {
    W.OS << "unknown file" << '\0';
    W.write<uint8_t>(UnknownFileID);
    W.write<uint32_t>(0); // File checksum
  }

  //  SourceMgr::SrcBuffer Buff = SourceMgr->getBufferInfo(BufferID);
  //  const char *BuffIdent = MemBuff->getBufferIdentifier();

  // unsigned FileId = 1;
  // for (auto I = Asm.file_names_begin(), E = Asm.file_names_end();
  //      I != E; ++I) {
  // }
}

uint64_t WLAVObjectWriter::writeObject(MCAssembler &Asm,
                                       const MCAsmLayout &Layout) {
  uint64_t StartOffset = W.OS.tell();

  enumerateSections(Asm, Layout);


  // Header: 'WLAY', object file version 24 (2020-03-24)
  W.write<uint8_t>('W');
  W.write<uint8_t>('L');
  W.write<uint8_t>('A');
  W.write<uint8_t>('7');

  // Misc bits: little endian, 65816 present
  W.write<uint8_t>(2);

  // Write the name of the source files, if available.
  writeSourceFiles(Asm, Layout);

  // Write exported definitions
  W.write<uint32_t>(0);

  // Write labels, symbols and breakpoints. Also creates a table
  // outlining which symbols are marked as private.
  writeSymbolTable(Asm, Layout);

  // Write "Outside references"
  W.write<uint32_t>(SimpleRelocations.size());
  for (const WLAVSimpleRelocationEntry &Rel : SimpleRelocations) {
    Rel.write(W, Asm, SymbolMap, SectionMap);
  }

  // Write "Pending calculations"
  W.write<uint32_t>(ComplexRelocations.size());
  unsigned CalcID = 1;
  for (const WLAVComplexRelocationEntry &Rel : ComplexRelocations) {
    Rel.write(W, Asm, SymbolMap, SectionMap, CalcID++);
  }

  // Write label sizeofs
  W.write<uint32_t>(0);

  // Write section appends
  W.write<uint32_t>(0);

  // Write data sections
  std::vector<const MCSection *> Sections;
  getSections(Asm, Sections);
  for (const MCSection *Section : Sections) {
    writeSection(Asm, Layout, *Section);
  }

  return W.OS.tell() - StartOffset;
}

void WLAVObjectWriter::getSections(MCAssembler &Asm,
                                   std::vector<const MCSection *> &Sections) {
  for (MCAssembler::iterator IT = Asm.begin(), IE = Asm.end(); IT != IE; ++IT) {
    Sections.push_back(&(*IT));
  }
}

std::unique_ptr<MCObjectWriter>
llvm::createWLAVObjectWriter(std::unique_ptr<MCWLAVObjectTargetWriter> MOTW,
                             raw_pwrite_stream &OS) {
  return std::make_unique<WLAVObjectWriter>(std::move(MOTW), OS);
}
