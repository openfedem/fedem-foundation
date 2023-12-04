// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#ifndef FFL_FEDEM_READER_H
#define FFL_FEDEM_READER_H

#include "FFlReaderBase.H"
#include "FFaLib/FFaDynCalls/FFaDynCB.H"
#include <string>
#include <vector>
#include <map>


class FFlFedemReader : public FFlReaderBase
{
public:
  FFlFedemReader(FFlLinkHandler* readIntoLink);
  virtual ~FFlFedemReader();

  static void init ();
  static void identifierCB (const std::string& fileName, int& isFtlFile);
  static void readerCB     (const std::string& fileName, FFlLinkHandler* link);

protected:
  struct ftlRefField
  {
    std::vector<int>         id;
    std::vector<std::string> options;
  };

  typedef std::multimap<std::string,ftlRefField> RefFieldMap;

  struct ftlField
  {
    std::string              label;
    std::vector<std::string> entries;
    RefFieldMap              refs;
  };

  typedef FFaDynCB1<const ftlField&>          FieldResolver;
  typedef std::map<std::string,FieldResolver> FieldResolverMap;

  bool read(const std::string& fileName);
  bool read(std::istream& is);

  bool getNextField(std::istream& is, ftlField& fl) const;
  void resolveVersion(const ftlField& field);
  void resolveNodeField(const ftlField& field);
  void resolveElementField(const ftlField& field);
  void resolveAttributeField(const ftlField& field);
  void resolveVisualField(const ftlField& field);
  void resolveGroupField(const ftlField& field);
  void resolveLoadField(const ftlField& field);

public:
  static bool ignoreCheckSum;

private:
  FieldResolverMap myFieldResolvers;

  int nErr;
  bool okAdd;
  int version;
  mutable int counter;
  mutable unsigned int linkChecksum;
};

#endif
