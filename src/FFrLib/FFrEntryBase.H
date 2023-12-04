// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#ifndef FFR_ENTRY_BASE_H
#define FFR_ENTRY_BASE_H

#ifdef FFR_LISTVIEW
#include "FFaLib/FFaDefinitions/FFaListViewItem.H"
#endif
#include <iostream>
#include <string>
#include <vector>
#include <set>

class FFrResultContainer;
class FFaResultDescription;


#ifdef FFR_LISTVIEW
class FFrEntryBase : public FFaListViewItem
#else
class FFrEntryBase
#endif
{
public:
  //! \brief Default constructor.
  FFrEntryBase() : myOwner(NULL), myGlobal(false)
  {
#if FFR_DEBUG > 2
    myCount = 0;
#endif
  }
  //! \brief Copy constructor.
  FFrEntryBase(const FFrEntryBase& obj)
  {
    myOwner = obj.myOwner;
    myGlobal = obj.myGlobal;
#if FFR_DEBUG > 2
    myCount = 0;
#endif
  }
  //! \brief Disable default assignment operator.
  FFrEntryBase& operator=(const FFrEntryBase&) = delete;
  //! \brief Empty destructor.
  virtual ~FFrEntryBase() {}

  //! \brief Returns the full path of this result entry.
  FFaResultDescription getEntryDescription() const;

  //! Returns the description of the object, e.g., "Triad 12" for a triad.
  //! For variable references, it returns the variable name.
  virtual const std::string& getDescription() const = 0;
  virtual bool hasDescription() const { return false; }

  //! Returns the referred object type (e.g., "Triad" for an object group
  //! representing triad data. For variable references, it returns the
  //! data type of the variable, e.g., SCALAR.
  virtual const std::string& getType() const = 0;

#ifdef FFR_LISTVIEW
  // From FFaListViewItem
  virtual const char* getItemName() const { return this->getType().c_str(); }
  // The next two methods always return 0 to avoid that result items are sorted
  virtual std::string getItemDescr() const { return ""; }
  virtual int getItemID() const { return 0; }
  virtual int getItemBaseID() const { return this->getBaseID(); }
#endif

  //! Returns baseID for objects that have a baseID. Returns 0 for other types.
  virtual int  getBaseID() const { return 0; }
  virtual bool hasBaseID() const { return false; }

  //! Returns userID for objects that have a userID (OG and some IGs).
  virtual int  getUserID() const { return 0; }
  virtual bool hasUserID() const { return false; }

  //! Returns a pointer to the data fields if they are set, returns 0 if not.
  virtual const std::vector<FFrEntryBase*>* getDataFields() const { return 0; }
  virtual bool hasDataFields() const { return false; }

  //! Reads all variable reference data in the hierarhcy under this entry.
  template<class T>
  int readPositionedTimestepData(const T* values, int nvals) const
  { return this->recursiveReadPosData(values, nvals, 0); }

  //! Prints out the positioning data of this entry.
  virtual void printPosition(std::ostream& os) const = 0;
#if FFR_DEBUG > 2
  void printID(std::ostream& os, bool EOL = true) const;
#endif

  //! Returns the owner of this entry in the hierarchy.
  FFrEntryBase* getOwner() const { return myOwner; }
  //! Defines the owner of this entry in the hierarchy.
#if FFR_DEBUG > 2
  void setOwner(FFrEntryBase* owner);
#else
  void setOwner(FFrEntryBase* owner) { myOwner = owner; }
#endif
  //! Defines this as a global-scope entry (as opposed to container-scope).
  virtual void setGlobal() { myGlobal = true; }
  //! Check if this entry is in global scope.
  bool isGlobal() const { return myGlobal; }

  // Simple type checking
  virtual bool isOG() const     { return false; }
  virtual bool isSOG() const    { return false; }
  virtual bool isIG() const     { return false; }
  virtual bool isVarRef() const { return false; }

  //! Returns true if this entry and all below it are empty.
  virtual bool isEmpty() const = 0;

  //! Returns true if the first variable referred is a float.
  virtual bool isVariableFloat() const = 0;

  // "Private" public methods

  virtual void removeContainers(const std::set<FFrResultContainer*>& cont) = 0;

  //! Traverses to build owner hierarchy and calculate binary sizes
  virtual int traverse(FFrResultContainer*, FFrEntryBase*, FFrEntryBase*&,
		       int binPos) { return binPos; }

  virtual bool merge(FFrEntryBase* objToMergeFrom) = 0;
  virtual bool compare(const FFrEntryBase*) const { return false; }
  virtual bool equal(const FFrEntryBase* obj) const = 0;
  virtual bool less(const FFrEntryBase* obj) const = 0;

  virtual int recursiveReadPosData(const double* vals, int nvals, int arrayPos) const = 0;
  virtual int recursiveReadPosData(const float* vals, int nvals, int arrayPos) const = 0;
  virtual int recursiveReadPosData(const int* vals, int nvals, int arrayPos) const = 0;

private:
  FFrEntryBase* myOwner;  //!< Parent of this entry in the RDB hierchy
  bool          myGlobal; //!< Global (extractor-level) or local (container level)

#if FFR_DEBUG > 2
protected:
  long int myCount;
#endif
};

#endif
