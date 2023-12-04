// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#ifndef FFR_ITEM_GROUP_H
#define FFR_ITEM_GROUP_H

#include "FFrLib/FFrFieldEntryBase.H"
#include "FFrLib/FFrEnums.H"
#include <cstdio>

class FFrCreatorData;


class FFrItemGroup : public FFrFieldEntryBase
{
public:
  FFrItemGroup(bool inlined = false);
  FFrItemGroup(const FFrItemGroup& obj);
  virtual ~FFrItemGroup();

  FFrItemGroup& operator=(const FFrItemGroup&) = delete;

  static FFrStatus create(FILE* varStream, FFrCreatorData& cd, bool dataBlocks);

  int fillObject(const std::vector<std::string>& tokens, FFrCreatorData& cd);

  virtual int traverse(FFrResultContainer* resultContainer,
		       FFrEntryBase* owner,
		       FFrEntryBase*& objToBeModified,
		       int binPos);

  virtual bool compare(const FFrEntryBase* obj) const;
  virtual bool equal(const FFrEntryBase* obj) const;
  virtual bool less(const FFrEntryBase* obj) const;

  virtual int  getUserID() const { return myId; }
  virtual bool hasUserID() const { return myId > 0; }

  virtual const std::string& getType() const;
  virtual const std::string& getDescription() const { return this->getType(); }
  virtual bool hasDescription() const { return true; }

  virtual bool isIG() const { return true; }
  virtual void setGlobal();

private:
  std::set<std::string>::const_iterator myNameIt;

  union {
    int myId; // -1 means a named item group and myNameIt points to its name
    FFrItemGroup* next; // used in memory management
  };
  bool isInlined;

#if FFR_DEBUG > 2
  static long int count;
#endif

public:
  struct Less
  {
    bool operator()(const FFrItemGroup* first, const FFrItemGroup* sec) const;
  };

#ifdef FFR_NEWALLOC
  // overloading the new & delete opers
  static void* operator new(size_t size);
  static void operator delete(void* deadObject, size_t size);

  static void releaseMemBlocks();

private:
  static size_t newed;
  static FFrItemGroup* headOfFreeList;
  static std::vector<FFrItemGroup*> memBlocks;
#endif
};

typedef std::set<FFrItemGroup*,FFrItemGroup::Less> ItemGroupSet;
typedef ItemGroupSet::const_iterator               ItemGroupSetCIt;
typedef ItemGroupSet::iterator                     ItemGroupSetIt;

#endif
