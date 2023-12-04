// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#ifndef FFL_SESAM_READER_H
#define FFL_SESAM_READER_H

#include "FFlReaderBase.H"
#include <string>
#include <vector>
#include <map>


class FFlSesamReader : public FFlReaderBase
{
  struct Record
  {
    Record(const std::vector<double>& f) : fields(f) {}
    std::vector<double>      fields;
    std::vector<std::string> text;
  };

  typedef std::vector<Record>           Records;
  typedef std::map<std::string,Records> RecordMap;
  typedef std::map<long int,short int>  LSintMap;
  typedef std::map<int,int>             IntMap;

public:
  FFlSesamReader(FFlLinkHandler* readIntoLink);
  virtual ~FFlSesamReader();

  static void init();
  static void identifierCB(const std::string& fileName, int& isSesamFile);
  static void readerCB    (const std::string& fileName, FFlLinkHandler* link);

  static const std::string& findName(const Records& names, int ID);

protected:
  bool read(const std::string& fileName);
  bool parse(const std::string& fileName);

  bool readNodes(const Records& recs);
  bool readBCs(const Records& recs);
  bool readMasses(const Records& recs);
  bool readLinearDependencies(const Records& recs);
  bool readElements(const Records& recs);
  bool readElementRefs(const Records& recs,
                       const Records& eccs, const Records& univ);
  bool readHinges(const Records& recs);
  bool readBeamSections(const Records& recs, const Records& names);
  bool readThicknesses(const Records& recs, const Records& names);
  bool readMaterials(const Records& recs, const Records& names);
  bool readGroundSprings(const Records& recs);
  bool readUnitVectors(const Records& recs);
  bool readGroups(const Records& recs, const Records& names);

private:
  RecordMap myRecs;
  IntMap    myHinges;
  LSintMap  myLinearDepDofs;
};

#endif
