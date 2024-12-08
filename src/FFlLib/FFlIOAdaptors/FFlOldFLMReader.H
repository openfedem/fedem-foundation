// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#ifndef FFL_OLD_FLM_READER_H
#define FFL_OLD_FLM_READER_H

#include <string>
#include <cstdio>

#include "FFlLib/FFlIOAdaptors/FFlReaderBase.H"

#ifdef FF_NAMESPACE
namespace FF_NAMESPACE {
#endif


class FFlOldFLMReader : public FFlReaderBase
{
public:
  FFlOldFLMReader (FFlLinkHandler* readIntoLink);
  virtual ~FFlOldFLMReader ();

  static void init ();
  static void identifierCB (const std::string& fileName, int& isFlmFile);
  static void readerCB     (const std::string& fileName, FFlLinkHandler* link);

protected:
  bool read(const std::string& fileName);

  char* findNextIdentifier(FILE* fp, char* first, ...);
  int getLine(char* buf, const int n, FILE* fp);

  bool readNodes(FILE* fp, int count);
  bool readElements(FILE* fp, int count);
  bool readBeamData(FILE* fp, int count);
  bool readSectionData(FILE* fp, int count);
  bool readElemProp(FILE* fp, int count);
  bool readElemMat(FILE* fp, int count);

  enum elementType {
    UNDEFINED_ELEMENT = -1,
    BEAM_ELEMENT = 11,
    FTS_ELEMENT  = 21,
    FQS_ELEMENT  = 22,
    ITET_ELEMENT = 41,
    IPRI_ELEMENT = 42,
    IHEX_ELEMENT = 43,
    HEXA_ELEMENT = 44,
    CSTET_ELEMENT = 45,
    WEDGE_ELEMENT = 46,
    MASS_ELEMENT = 51,
    RIGID_BEAM_ELEMENT = 61,
    RIGID_NFOLD_BEAM_ELEMENT = 62
  };
};

#ifdef FF_NAMESPACE
} // namespace
#endif
#endif
