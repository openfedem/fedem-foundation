// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#ifndef FFA_REFERENCE_LIST_H
#define FFA_REFERENCE_LIST_H

#include <list>

#include "FFaLib/FFaContainers/FFaReference.H"


/*!
  \brief The base class for FFaReferenceList.

  When #IAmAutoSizing is \e true, the entries in the list are deleted when
  the object pointed to is deleted. If \a false, the reference is set to zero.

  The context name of a reference list is used when doing topological browsing
  to know why a reference is referring to a certain FFaFieldContainer object.
  It is supposed to be set upon initializing when adding the list to the
  book-keeping in the FFaFieldContainer object it resides in.
*/

class FFaReferenceListBase
{
  // Remove implicitly defined copy constructor and assignment operator
  FFaReferenceListBase(const FFaReferenceListBase&) = delete;
  FFaReferenceListBase& operator=(const FFaReferenceListBase&) = delete;

protected:
  FFaReferenceListBase();
  virtual ~FFaReferenceListBase();

public:
  void setAutoSizing(bool onOrOff) { IAmAutoSizing = onOrOff; }
  bool isAutoSizing() const { return IAmAutoSizing; }

  void setContextName(const char* name) { myContextName = name; }
  const char* getContextName() const { return myContextName ? myContextName : ""; }

  void clear();
  bool erase(int index);

  bool shuffle(size_t newFirst);
  void reverse() { myRefs.reverse(); }
  void removePtr (const FFaFieldContainer* ptr) { this->removePtr(ptr,true); }
  void zeroOutPtr(const FFaFieldContainer* ptr) { this->zeroOutPtr(ptr,true); }
  bool hasPtr    (const FFaFieldContainer* ptr, int* index = NULL) const;

  void getBasePtrs(std::vector<FFaFieldContainer*>& toFill) const;
  FFaFieldContainer* getBasePtr(int idx) const;

  size_t size() const { return myRefs.size(); }
  bool empty() const { return myRefs.empty(); }

  void write(std::ostream& os) const;
  void read(std::istream& is);

protected:
  std::list<FFaReferenceBase*> myRefs; //!< The real list of references

  void insertRefLast(FFaReferenceBase* ref);
  virtual FFaReferenceBase* createNewReference() = 0;

  std::list<FFaReferenceBase*>::const_iterator findPtr(const FFaFieldContainer* ptr) const;
  std::list<FFaReferenceBase*>::iterator       findPtr(const FFaFieldContainer* ptr);

  FFaReferenceBase* getRefBase(int idx) const;

  void setOwnerOnRef(FFaReferenceBase* ref) { ref->setOwnerReferenceList(this); }

  void copy(const FFaReferenceListBase* other, bool unresolve = false);

private:
  friend class FFaFieldContainer;
  friend class FFaReferenceBase;

  void setOwnerFieldContainer(FFaFieldContainer* owner) { myOwner = owner; }
  FFaFieldContainer* getOwnerFieldContainer() const { return myOwner; }

  void eraseReferenceIfNeeded(FFaReferenceBase* ref);

  void resolve(FFaSearcher& findCB);
  void unresolve();
  void updateAssemblyRef(int from, int to, size_t ind = 0);
  void updateAssemblyRef(const IntVec& from, const IntVec& to);

  void removePtr (const FFaFieldContainer* ptr, bool notifyContainer);
  void zeroOutPtr(const FFaFieldContainer* ptr, bool notifyContainer);

  FFaFieldContainer* myOwner;
  const char* myContextName;

  //! Tells whether a reference in the list is deleted or set to zero
  //! when the referred object disappears
  bool IAmAutoSizing;
};


/*!
  \brief A container of FFaReference objects (guarded pointers).

  This class provides a container of guarded pointers (FFaReference) that can be
  used in classes derived from FFaFieldContainer to refer to several objects of
  classes derived from FFaFieldContainer. The class can be used in two modes:
  Either like a vector/array with elements automatically set to zero when
  referred objects are deleted, or like a list where the references/pointers are
  deleted from the list when the objects referred to cease to exist.

  One of the main reasons to use this class is to use it together with FFaField
  and get automated IO of the list of references.
  To use this feature, the FFaFieldContainer derived class containing the
  reference needs an FFaField<FFaReferenceListBase*> object that is initialized
  with the address of the FFaReferenceList to read/write:

  \code
  class FmNewClass : FFaFieldContainer
  {
    // ...

    FFaReferenceList<FmSomeClass>   someClassReferences;
    FFaField<FFaReferenceListBase*> someWrittenReferences;

    // ...
  };

  FmNewClass::FmNewClass()
  {
    // ...

    // Add reference list to book keeping in the FFaFieldContainer
    this->addRefList(&someClassReferences);
    // Add field to book keeping in FFaFieldContainer
    FFA_FIELD_INIT(someWrittenReferences,&someClassReferences,"FILE_KEY_WORD");

    // ...
  }
  \endcode
*/

template<class Type>
class FFaReferenceList : public FFaReferenceListBase
{
public:
  FFaReferenceList() {}
  virtual ~FFaReferenceList() {}

  //! Assignment from vector of pointers to FFaFieldContainer derived classes.
  FFaReferenceList<Type>&  operator=(const std::vector<Type*>& ptrs)
  { this->setPtrs(ptrs); return *this; }

  //! Inserts ptr first.
  void push_front(Type* ptr, bool insertNullToo = false)
  { if (ptr || insertNullToo) myRefs.push_front(this->makeNewRef(ptr)); }

  //! Inserts ptr at the end.
  void push_back(Type* ptr, bool insertNullToo = false)
  { if (ptr || insertNullToo) myRefs.push_back(this->makeNewRef(ptr)); }

  //! Insert pointer after objInList or last if objInList == NULL or not found.
  void insertPtrAfter(Type* ptr, Type* objInList = NULL)
  {
    std::list<FFaReferenceBase*>::iterator it = myRefs.end();
    if (objInList)
      it = this->findPtr(static_cast<const FFaFieldContainer*>(objInList));

    if (it != myRefs.end())
      myRefs.insert(++it,this->makeNewRef(ptr));
    else
      this->push_back(ptr);
  }

  //! Insert pointer at \a idx or at the end if \a idx is larger than size().
  void insert(Type* ptr, size_t idx = 0)
  {
    size_t i = 0;
    std::list<FFaReferenceBase*>::iterator it;
    for (it = myRefs.begin(); it != myRefs.end(); i++, it++)
      if (i == idx)
      {
        myRefs.insert(it,this->makeNewRef(ptr));
        return;
      }

    this->push_back(ptr);
  }

  //! Fills the provided vector with the pointers contained in the list.
  void getPtrs(std::vector<Type*>& toFill, bool includeNullToo = false) const
  {
    toFill.clear();
    for (FFaReferenceBase* ref : myRefs)
      if (includeNullToo || !ref->isNull())
        toFill.push_back(static_cast<FFaReference<Type>*>(ref)->getPointer());
  }

  //! Replaces the current contents with the contents of the provided vector.
  void setPtrs(const std::vector<Type*>& ptrs)
  {
    this->clear();
    for (Type* ptr : ptrs)
      myRefs.push_back(this->makeNewRef(ptr));
  }

  //! Replaces the current contents with the provided new references.
  void setRefs(const IntVec& IDs)
  {
    this->clear();
    for (int id : IDs)
      myRefs.push_back(this->makeNewRef(NULL,id));
  }

  /*!
    \brief Indexing operators.
    The returned FFaReference reference can be used almost as a pointer,
    making it possible to do:
    \code
    FFaReferenceList<FmSomeClass> referenceList;
    FmSomeClass* somePointer = referenceList[i];
    referenceList[i] = somePointer;
    \endcode
  */
  FFaReference<Type>& operator[] (int idx)
  {
    FFaReferenceBase* refp = this->getRefBase(idx);
    if (refp) return *static_cast<FFaReference<Type>*>(refp);

    std::cerr <<"FFaReference::operator[]: Index "<< idx
              <<" out of range [0,"<< this->size() <<"]"<< std::endl;
    static FFaReference<Type> dummy;
    return dummy;
  }

  const FFaReference<Type>& operator[](int idx) const
  {
    FFaReferenceBase* refp = this->getRefBase(idx);
    if (refp) return *static_cast<const FFaReference<Type>*>(refp);

    std::cerr <<"FFaReference::operator[]: Index "<< idx
              <<" out of range [0,"<< this->size() <<"]"<< std::endl;

    static FFaReference<Type> dummy;
    return dummy;
  }

  //! Assigns ptr at index idx, and pads the list with zeros if neccesary.
  void setPtr(Type* ptr, int idx)
  {
    int i = 0;
    std::list<FFaReferenceBase*>::iterator it;
    for (it = myRefs.begin(); it != myRefs.end(); i++, it++)
      if (i == idx)
      {
        *static_cast<FFaReference<Type>*>(*it) = ptr;
        return;
      }

    while (i++ < idx)
      myRefs.push_back(this->makeNewRef(NULL));

    myRefs.push_back(this->makeNewRef(ptr));
  }

  //! Returns pointer at index idx, or NULL if idx is outside the range.
  Type* getPtr(int idx) const
  {
    FFaReferenceBase* refp = this->getRefBase(idx);
    if (refp) return static_cast<FFaReference<Type>*>(refp)->getPointer();

    return NULL;
  }

  //! Returns first pointer, or NULL if list is empty.
  Type* getFirstPtr() const
  {
    if (myRefs.empty()) return NULL;

    return static_cast<FFaReference<Type>*>(myRefs.front())->getPointer();
  }

  //! Returns last pointer, or NULL if list is empty.
  Type* getLastPtr() const
  {
    if (myRefs.empty()) return NULL;

    return static_cast<FFaReference<Type>*>(myRefs.back())->getPointer();
  }

private:
  //! Convenience method to create and set up a new reference properly.
  FFaReference<Type>* makeNewRef(Type* ptr, int ID = 0)
  {
    FFaReference<Type>* sr = new FFaReference<Type>();
    this->setOwnerOnRef(sr);
    sr->setPointer(ptr);
    if (ID) sr->setRef(ID,Type::getClassTypeID());
    return sr;
  }

  //! Virtual method used to create an FFaReference of correct type.
  virtual FFaReferenceBase* createNewReference()
  { return static_cast<FFaReferenceBase*>(new FFaReference<Type>()); }
};


// Specialized instantiations of FFaField methods.

using FFaRefListPtr = FFaReferenceListBase*;

template<> inline
FFaField<FFaRefListPtr>::FFaField()
{
  myData = myDefaultValue = NULL;
}

template<> inline bool
FFaField<FFaRefListPtr>::isDataField() const
{
  return false;
}

template<> inline bool
FFaField<FFaRefListPtr>::isPrintable() const
{
  return myData ? !myData->empty() : false;
}

template<> inline void
FFaField<FFaRefListPtr>::write(std::ostream& os) const
{
  if (myData) myData->write(os);
}

template<> inline void
FFaField<FFaRefListPtr>::read(std::istream& is, FFaRefListPtr& v) const
{
  if (v) v->read(is);
}

#endif
