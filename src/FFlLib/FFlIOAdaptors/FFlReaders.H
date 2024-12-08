// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#ifndef FFL_READERS_H
#define FFL_READERS_H

#include <string>
#include <vector>
#include <set>

#include "FFaLib/FFaPatterns/FFaSingelton.H"
#include "FFaLib/FFaDynCalls/FFaDynCB.H"

#ifdef FF_NAMESPACE
namespace FF_NAMESPACE {
#endif

class FFlLinkHandler;


struct FFlReaderInfo
{
  std::string name;
  std::set<std::string> extensions;
  std::string info1;
  std::string info2;
};

struct FFlReaderData
{
  FFlReaderInfo info;
  FFaDynCB2<const std::string&,FFlLinkHandler*> readerCB;
  FFaDynCB2<const std::string&,int&>            identifierCB;
};


class FFlReaders : public FFaSingelton<FFlReaders>
{
public:
  bool registerReader(const std::string& name, const std::string& ext,
                      const FFaDynCB2<const std::string&,FFlLinkHandler*>& readerCB,
                      const FFaDynCB2<const std::string&,int&>& identifierCB,
                      const std::string& info1, const std::string& info2);

  void getRegisteredReaders(std::vector<FFlReaderInfo>& infoData,
                            bool includeDefaultReader = true) const;

  const FFlReaderInfo& getDefaultReader() const;
  void setDefaultReader(const std::string& name);
  void addExtension(const std::string& name, const std::string& ext);

  int read(const std::string& fileName, FFlLinkHandler* link);

  static char convertToLinear;

protected:
  FFlReaders() : defaultReader(0) {}
  virtual ~FFlReaders() {}

private:
  std::vector<FFlReaderData> myReaders;
  unsigned int defaultReader;

  friend class FFaSingelton<FFlReaders>;
};

#ifdef FF_NAMESPACE
} // namespace
#endif
#endif
