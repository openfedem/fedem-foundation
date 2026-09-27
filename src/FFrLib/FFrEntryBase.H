// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

/*!
  \file FFrEntryBase.H
  \brief Base class for result file entries.
*/

#ifndef FFR_ENTRY_BASE_H
#define FFR_ENTRY_BASE_H

#ifdef FFR_LISTVIEW
#include "FFaLib/FFaDefinitions/FFaListViewItem.H"
#endif
#include <iostream>
#include <string>
#include <vector>
#include <set>

class FFrEntryBase;
class FFrResultContainer;
class FFaResultDescription;

using FFrEntries = std::vector<FFrEntryBase*>; //!< Convenience type alias


/*!
  \brief Base class for result file entries.
  \details This is an abstract interface for various result file entry types.
  It contains many pure virtual methods to be implemented by sub-classes.
*/

#ifdef FFR_LISTVIEW
class FFrEntryBase : public FFaListViewItem
#else
class FFrEntryBase
#endif
{
protected:
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

public:
  //! \brief Empty destructor.
  virtual ~FFrEntryBase() {}

  //! \brief Returns the full path of this result entry.
  FFaResultDescription getEntryDescription() const;

  //! \brief Returns the description of this entry.
  virtual const std::string& getDescription() const = 0;
  //! \brief Returns whether the entry has a description or not.
  virtual bool hasDescription() const { return false; }

  //! \brief Returns the referred object type.
  //! \details E.g., "Triad" for an object group representing triad data.
  //! For variable references, it returns the data type of that variable,
  //! e.g., SCALAR.
  virtual const std::string& getType() const = 0;

#ifdef FFR_LISTVIEW
  //! \brief Returns the type name of this entry for use in list views.
  virtual const char* getItemName() const { return this->getType().c_str(); }
  //! \brief Returns an empty string to avoid sorting in list views.
  virtual std::string getItemDescr() const { return ""; }
  //! \brief Returns zero to avoid sorting in list views.
  virtual int getItemID() const { return 0; }
  //! \brief Returns the base ID of this entry, or zero if none.
  virtual int getItemBaseID() const { return this->getBaseID(); }
#endif

  //! \brief Returns the base ID of the object if this entry is an Object Group.
  //! \details The method returns 0 for other entry types.
  virtual int  getBaseID() const { return 0; }
  //! \brief Returns whether this entry has a base ID or not.
  virtual bool hasBaseID() const { return false; }

  //! \brief Returns the user ID for this entry.
  //! \details Object group and item group entries have a user ID.
  //! For other types (e.g., variable references) zero is returned.
  virtual int  getUserID() const { return 0; }
  //! \brief Returns whether this entry has a user ID or not.
  virtual bool hasUserID() const { return false; }

  //! \brief Returns a pointer to the data fields of this entry.
  //! \details A null pointer is returned if the data fields are not set.
  virtual const FFrEntries* getDataFields() const { return NULL; }
  //! \brief Returns whether this entry has data fields or not.
  virtual bool hasDataFields() const { return false; }

  //! \brief Reads variable reference data from the hierarhcy under this entry.
  template<class T>
  int readPositionedTimestepData(const T* values, int nvals) const
  { return this->recursiveReadPosData(values, nvals, 0); }

  //! \brief Prints out the positioning data of this entry.
  virtual void printPosition(std::ostream& os) const = 0;
#if FFR_DEBUG > 2
  //! \brief Prints out the identification of this entry.
  void printID(std::ostream& os, bool EOL = true) const;
#endif

  //! \brief Returns the owner of this entry in the hierarchy.
  FFrEntryBase* getOwner() const { return myOwner; }
  //! \brief Defines the owner of this entry in the hierarchy.
#if FFR_DEBUG > 2
  void setOwner(FFrEntryBase* owner);
#else
  void setOwner(FFrEntryBase* owner) { myOwner = owner; }
#endif
  //! \brief Defines this as a global scope entry.
  //! \details By default, all entries are considered to be in container scope.
  virtual void setGlobal() { myGlobal = true; }
  //! \brief Check if this entry is in global scope or not.
  bool isGlobal() const { return myGlobal; }

  //! \brief Returns whether this entry is an Object Group or not.
  virtual bool isOG() const     { return false; }
  //! \brief Returns whether this entry is a Super Object Group or not.
  virtual bool isSOG() const    { return false; }
  //! \brief Returns whether this entry is an Item Group or not.
  virtual bool isIG() const     { return false; }
  //! \brief Returns whether this entry is a Variable Reference or not.
  virtual bool isVarRef() const { return false; }

  //! \brief Returns \e true if this entry and all below it are empty.
  virtual bool isEmpty() const = 0;

  //! \brief Returns \e true if the first variable referred is a float.
  virtual bool isVariableFloat() const = 0;

  // "Private" public methods

  //! \brief Removes data associated with given containers for this entry.
  virtual void removeContainers(const std::set<FFrResultContainer*>& cont) = 0;

  //! \brief Traverses to build owner hierarchy and calculate binary sizes.
  virtual int traverse(FFrResultContainer*, FFrEntryBase*, FFrEntryBase*&,
		       int binPos) { return binPos; }

  //! \brief Merges another entry into \a *this.
  virtual bool merge(FFrEntryBase* objToMergeFrom) = 0;
  //! \brief Returns \e true if \a *this equals another specified entry.
  virtual bool compare(const FFrEntryBase*) const { return false; }
  //! \brief Returns \e true if \a *this and \a *obj are considered equal.
  virtual bool equal(const FFrEntryBase* obj) const = 0;
  //! \brief Returns \e true if \a *this is ordered before \a *obj.
  virtual bool less(const FFrEntryBase* obj) const = 0;

  //! \brief Reads double data from the hierarhcy under this entry.
  virtual int recursiveReadPosData(const double* vals, int nvals, int arrayPos) const = 0;
  //! \brief Reads float data from the hierarhcy under this entry.
  virtual int recursiveReadPosData(const float* vals, int nvals, int arrayPos) const = 0;
  //! \brief Reads integer data from the hierarhcy under this entry.
  virtual int recursiveReadPosData(const int* vals, int nvals, int arrayPos) const = 0;

  //! \brief Reads a variable from the results container for current time step.
  template<class T> bool readVar(T& value);

private:
  FFrEntryBase* myOwner; //!< Parent of this entry in the RDB hierchy
  bool myGlobal; //!< Global (extractor-level) or local (container level)

#if FFR_DEBUG > 2
protected:
  long int myCount; //!< Internal counter, for debug usage
#endif
};

#endif
