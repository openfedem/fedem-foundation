// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#ifndef FFL_READER_BASE_H
#define FFL_READER_BASE_H

#include <cstddef>

class FFaProfiler;

#ifdef FF_NAMESPACE
namespace FF_NAMESPACE {
#endif

class FFlLinkHandler;


class FFlReaderBase
{
public:
  FFlReaderBase(FFlLinkHandler* p) : myLink(p), myProfiler(NULL) {}
  virtual ~FFlReaderBase() {}

protected:
  static int searchKeyword(const char* fileName, const char* keyWord,
                           int maxLines = 100);
  static int searchKeyword(const char* fileName, const char** keyWord,
                           int maxLines = 100);

  FFlLinkHandler* myLink;
  FFaProfiler*    myProfiler;
};

#ifdef FF_NAMESPACE
} // namespace
#endif
#endif
