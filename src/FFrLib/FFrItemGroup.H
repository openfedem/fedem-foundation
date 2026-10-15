// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

/*!
  \file FFrItemGroup.H
  \brief Class representing an item group in a result file hierarchy.
*/

#ifndef FFR_ITEM_GROUP_H
#define FFR_ITEM_GROUP_H

#include "FFrLib/FFrFieldEntryBase.H"
#include "FFrLib/FFrEnums.H"
#include <cstdio>

class FFrCreatorData;


/*!
  \brief Class representng an item group in a result file hierarchy.
*/

class FFrItemGroup : public FFrFieldEntryBase
{
public:
  //! \brief Default constructor.
  explicit FFrItemGroup(bool inlined = false);
  //! \brief Copy constructor.
  FFrItemGroup(const FFrItemGroup& obj);
  //! \brief Empty destructor.
  virtual ~FFrItemGroup();

  //! \brief Deleted Assignment operator.
  FFrItemGroup& operator=(const FFrItemGroup&) = delete;

  //! \brief Static method for parsing an item group definition from file.
  static FFrStatus create(FILE* varStream, FFrCreatorData& cd, bool dataBlocks);

  //! \brief Parses the item group definition from a list of strings.
  int fillObject(const std::vector<std::string>& tokens, FFrCreatorData& cd);

  virtual int traverse(FFrResultContainer* resultContainer,
		       FFrEntryBase* owner,
		       FFrEntryBase*& objToBeModified,
		       int binPos);

  virtual void setGlobal();

  virtual bool equal(const FFrEntryBase* obj) const;
  virtual bool comp(const FFrEntryBase* obj) const;
  virtual bool less(const FFrEntryBase* obj) const;

  virtual int  getUserID() const { return myId; }
  virtual bool hasUserID() const { return myId > 0; }

  //! \brief Returns either the given name or the ID of the item group.
  virtual const std::string& getType() const;
  //! \brief Returns the type name of the item group.
  virtual const std::string& getDescription() const { return this->getType(); }
  //! \brief Returns \e true since all item groups have a type name.
  virtual bool hasDescription() const { return true; }

  //! \brief Returns \e true since this entry is an Item Group.
  virtual bool isIG() const { return true; }

private:
  //! \brief Iterator pointing to the item group name.
  //! \details This is only used when \ref myId equals -1.
  std::set<std::string>::const_iterator myNameIt;

  //! \brief Shared data memory segment for \ref myId and \ref next.
  union {
    int           myId; //!< The ID of this item group
    FFrItemGroup* next; //!< Used in memory management
  };
  bool isInlined; //!< If \e true, the item group has no name and no Id

#if FFR_DEBUG > 2
  static long int count; //!< Internal counter, for debug usage
#endif

public:
  //! \brief Functor for comparison of two item group definitions.
  struct Less
  {
    //! \brief Returns \e true if \a *first is considered less than \a *sec.
    bool operator()(const FFrItemGroup* first, const FFrItemGroup* sec) const;
  };

#ifdef FFR_NEWALLOC
  //! \cond MEMORY_MANAGEMENT
  // overloading the new & delete opers
  static void* operator new(size_t size);
  static void operator delete(void* deadObject, size_t size);

  static void releaseMemBlocks();

private:
  static size_t newed;
  static FFrItemGroup* headOfFreeList;
  static std::vector<FFrItemGroup*> memBlocks;
  //! \endcond
#endif
};


//! Data type alias for a sorted set of item groups
using ItemGroupSet    = std::set<FFrItemGroup*,FFrItemGroup::Less>;
//! Iterator over const item group container
using ItemGroupSetCIt = ItemGroupSet::const_iterator;
//! Iterator over item group container
using ItemGroupSetIt  = ItemGroupSet::iterator;

#endif
