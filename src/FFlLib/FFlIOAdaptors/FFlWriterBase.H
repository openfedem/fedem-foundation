// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#ifndef FFL_WRITER_BASE_H
#define FFL_WRITER_BASE_H

#include <cstddef>

class FFaProfiler;

#ifdef FF_NAMESPACE
namespace FF_NAMESPACE {
#endif

class FFlLinkHandler;


class FFlWriterBase
{
public:
  FFlWriterBase(const FFlLinkHandler* p) : myLink(p), myProfiler(NULL) {}
  virtual ~FFlWriterBase() {}

protected:
  const FFlLinkHandler* myLink;
  FFaProfiler*          myProfiler;
};

#ifdef FF_NAMESPACE
} // namespace
#endif
#endif
