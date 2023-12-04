// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#ifndef FFR_OBJECT_GROUP_H
#define FFR_OBJECT_GROUP_H

#include "FFrLib/FFrFieldEntryBase.H"
#include "FFrLib/FFrEnums.H"
#include <cstdio>

class FFrCreatorData;


class FFrObjectGroup : public FFrFieldEntryBase
{
public:
  FFrObjectGroup();
  virtual ~FFrObjectGroup();

  static FFrStatus create(FILE* varStream, FFrCreatorData& cd, bool dataBlocks);

  int fillObject(const std::vector<std::string>& tokens, FFrCreatorData& cd);

  virtual int traverse(FFrResultContainer* resultContainer,
		       FFrEntryBase* owner,
		       FFrEntryBase*& objToBeModified,
		       int binPos);

  virtual const std::string& getDescription() const { return description; }
  virtual bool hasDescription() const { return !description.empty(); }

  virtual const std::string& getType() const { return *typeIt; }

  // Returns baseID for objects that have a baseID. Returns 0 for other types.
  virtual int getBaseID() const  { return baseId; }
  virtual bool hasBaseID() const { return baseId != 0; }

  // Returns userID for objects that have a userID (OG and some IGs).
  // Returns 0 for other types
  virtual int getUserID() const  { return id; }
  virtual bool hasUserID() const { return id != 0; }

  virtual bool isOG() const { return true; }

private:
  int baseId;
  int id;

  std::set<std::string>::const_iterator typeIt;
  std::string description;

#if FFR_DEBUG > 2
  static long int count;
#endif
};

#endif
