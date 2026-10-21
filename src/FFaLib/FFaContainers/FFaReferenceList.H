// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

/*!
  \file FFaReferenceList.H
  \brief Classes representing lists of guarded pointers.
*/

#ifndef FFA_REFERENCE_LIST_H
#define FFA_REFERENCE_LIST_H

#include <list>

#include "FFaLib/FFaContainers/FFaReference.H"


/*!
  \brief The base class for FFaReferenceList.

  \details When #autoSizing is \e true, the entries in the list are deleted when
  the object pointed to is deleted. Otherwise, the reference is set to zero.

  The context name of a reference list is used when doing topological browsing
  to know why a reference is referring to a certain FFaFieldContainer object.
  It is supposed to be set upon initializing when adding the list to the
  book-keeping in the FFaFieldContainer object it resides in.
*/

class FFaReferenceListBase
{
  //! \brief Remove implicitly defined copy constructor.
  FFaReferenceListBase(const FFaReferenceListBase&) = delete;
  //! \brief Remove implicitly defined assignment operator,
  FFaReferenceListBase& operator=(const FFaReferenceListBase&) = delete;

protected:
  //! \brief The default constructor initializes the members.
  FFaReferenceListBase() : myOwner(NULL), myContext(NULL), autoSizing(true) {}
  //! \brief The destructor clears the list.
  virtual ~FFaReferenceListBase() { this->clear(); }

public:
  //! \brief Sets the #autoSizing flag.
  void setAutoSizing(bool onOrOff) { autoSizing = onOrOff; }
  //! \brief Returns the value of the #autoSizing flag.
  bool isAutoSizing() const { return autoSizing; }

  //! \brief Assigns the context name #myContext for this reference.
  void setContextName(const char* name) { myContext = name; }
  //! \brief Returns the context name #myContext used by this reference.
  const char* getContextName() const { return myContext ? myContext : ""; }

  //! \brief Erases entry \a index from the list, decrementing its size by one.
  bool erase(int index);
  //! \brief Clears the reference list.
  void clear();

  //! \brief Reorders the elements in the list.
  bool shuffle(size_t newFirst);
  //! \brief Inverts the element order in the list.
  void reverse() { myRefs.reverse(); }
  //! \brief Removes all instances of \a ptr from this list.
  void removePtr (const FFaFieldContainer* ptr) { this->removePtr(ptr,true); }
  //! \brief Sets all instances of \a ptr in this list to zero.
  void zeroOutPtr(const FFaFieldContainer* ptr) { this->zeroOutPtr(ptr,true); }
  //! \brief Returns \e true if \a ptr is present in the list.
  bool hasPtr(const FFaFieldContainer* ptr, int* index = NULL) const;

  //! \brief Fills provided vector with the non-NULL pointers in the list.
  void getBasePtrs(std::vector<FFaFieldContainer*>& toFill) const;
  //! \brief Returns the pointer to the \a idx'th member in the list.
  FFaFieldContainer* getBasePtr(int idx) const;

  //! \brief Returns the length of #myRefs,
  size_t size() const { return myRefs.size(); }
  //! \brief Returns \e true if #myRefs is empty (zero length).
  bool empty() const { return myRefs.empty(); }

  //! \brief Writes the reference list to specified output stream.
  void write(std::ostream& os) const;
  //! \brief Reads the reference list from specified input stream.
  void read(std::istream& is);

protected:
  //! Convenience type alias
  using FFaReferenceList = std::list<FFaReferenceBase*>;

  FFaReferenceList myRefs; //!< The actual list of references

  //! \brief Appends a reference to the end of #myRefs.
  void insertRefLast(FFaReferenceBase* ref);
  //! \brief Virtual method for creating a FFaReference of correct type.
  virtual FFaReferenceBase* createNewReference() = 0;

  //! \brief Returns a const_iterator matching a FFaFieldContainer pointer.
  FFaReferenceList::const_iterator findPtr(const FFaFieldContainer* ptr) const;
  //! \brief Returns an iterator matching a FFaFieldContainer pointer.
  FFaReferenceList::iterator       findPtr(const FFaFieldContainer* ptr);

  //! \brief Returns the pointer at index \a idx.
  //! \details If \a idx is out of range, NULL is returned.
  FFaReferenceBase* getRefBase(int idx) const;

  //! \brief Sets this list as the owner of the reference \a pt.
  void setOwnerOnRef(FFaReferenceBase* pt) { pt->setOwnerReferenceList(this); }

  //! \brief Copy method used by copy constructors, etc.
  void copy(const FFaReferenceListBase* other, bool unresolve = false);

private:
  //! \brief Grants access to some private methods for resolving references.
  friend class FFaFieldContainer;
  //! \brief Grants access to some private methods for resolving references.
  friend class FFaReferenceBase;

  //! \brief Sets the FFaFieldContainer that this reference list is a member of.
  void setOwnerFieldContainer(FFaFieldContainer* owner) { myOwner = owner; }
  //! \brief Returns the FFaFieldContainer this reference list is a member of.
  FFaFieldContainer* getOwnerFieldContainer() const { return myOwner; }

  //! \brief Removes a reference from the list.
  void eraseReferenceIfNeeded(FFaReferenceBase* ref);

  //! \brief Used after reading to translate IDs to actual pointers.
  void resolve(FFaSearcher& findCB);
  //! \brief Does the opposite of resolve.
  void unresolve();
  //! \brief Updates the assembly reference when moving to another assembly.
  void updateAssemblyRef(int from, int to, size_t ind = 0);
  //! \brief Updates the assembly reference when moving to another assembly.
  void updateAssemblyRef(const IntVec& from, const IntVec& to);

  //! \brief Removes all instances of \a ptr from this list.
  void removePtr (const FFaFieldContainer* ptr, bool notifyContainer);
  //! \brief Sets all instances of \a ptr in this list to zero.
  void zeroOutPtr(const FFaFieldContainer* ptr, bool notifyContainer);

  FFaFieldContainer* myOwner;   //!< Points to the owner object
  const char*        myContext; //!< Typically, this will be the variable name

  //! Tells whether a reference in the list is deleted or set to zero
  //! when the referred object disappears
  bool autoSizing;
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
  //! \brief Overloaded assignment operator.
  FFaReferenceList<Type>&  operator=(const std::vector<Type*>& ptrs)
  { this->setPtrs(ptrs); return *this; }

  //! \brief Inserts \a ptr in the front of the list.
  void push_front(Type* ptr, bool insertNullToo = false)
  { if (ptr || insertNullToo) myRefs.push_front(this->makeNewRef(ptr)); }

  //! \brief Inserts \a ptr at the end of the list.
  void push_back(Type* ptr, bool insertNullToo = false)
  { if (ptr || insertNullToo) myRefs.push_back(this->makeNewRef(ptr)); }

  //! \brief Insert \a ptr after \a objInList in the list.
  //! \details If \a objInList is NULL or not found in the list,
  //! \a ptr is inserted at the end instead.
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

  //! \brief Inserts \a ptr at position \a idx in the list.
  //! \details If \a idx is larger than the length of the list,
  //! \a ptr is inserted at the end instead.
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

  //! \brief Fills the provided vector with the pointers contained in the list.
  void getPtrs(std::vector<Type*>& toFill, bool includeNullToo = false) const
  {
    toFill.clear();
    for (FFaReferenceBase* ref : myRefs)
      if (includeNullToo || !ref->isNull())
        toFill.push_back(static_cast<FFaReference<Type>*>(ref)->getPointer());
  }

  //! \brief Replaces the current contents with the provided vector of pointers.
  void setPtrs(const std::vector<Type*>& ptrs)
  {
    this->clear();
    for (Type* ptr : ptrs)
      myRefs.push_back(this->makeNewRef(ptr));
  }

  //! \brief Replaces the current contents with the provided new references.
  void setRefs(const IntVec& IDs)
  {
    this->clear();
    for (int id : IDs)
      myRefs.push_back(this->makeNewRef(NULL,id));
  }

  /*!
    \brief Indexing operator (for update).
    \details The returned reference can be used almost as a pointer,
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

  //! \brief Indexing operator (for read only).
  const FFaReference<Type>& operator[](int idx) const
  {
    FFaReferenceBase* refp = this->getRefBase(idx);
    if (refp) return *static_cast<const FFaReference<Type>*>(refp);

    std::cerr <<"FFaReference::operator[]: Index "<< idx
              <<" out of range [0,"<< this->size() <<"]"<< std::endl;

    static FFaReference<Type> dummy;
    return dummy;
  }

  //! \brief Assigns \a ptr at position \a idx.
  //! \details #myRefs is padded with NULL before \a idx if it is shorter.
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

  //! \brief Returns the pointer at position \a idx.
  //! \details Returns NULL if \a idx is outside the range.
  Type* getPtr(int idx) const
  {
    FFaReferenceBase* refp = this->getRefBase(idx);
    if (refp) return static_cast<FFaReference<Type>*>(refp)->getPointer();

    return NULL;
  }

  //! \brief Returns the first pointer, or NULL if #myRefs is empty.
  Type* getFirstPtr() const
  {
    if (myRefs.empty()) return NULL;

    return static_cast<FFaReference<Type>*>(myRefs.front())->getPointer();
  }

  //! \brief Returns the last pointer, or NULL if #myRefs is empty.
  Type* getLastPtr() const
  {
    if (myRefs.empty()) return NULL;

    return static_cast<FFaReference<Type>*>(myRefs.back())->getPointer();
  }

private:
  //! \brief Convenience method to create and set up a new reference properly.
  FFaReference<Type>* makeNewRef(Type* ptr, int ID = 0)
  {
    FFaReference<Type>* sr = new FFaReference<Type>();
    this->setOwnerOnRef(sr);
    sr->setPointer(ptr);
    if (ID) sr->setRef(ID,Type::getClassTypeID());
    return sr;
  }

  virtual FFaReferenceBase* createNewReference()
  { return static_cast<FFaReferenceBase*>(new FFaReference<Type>()); }
};


// Specialized instantiations of FFaField methods.

//! \cond TEMPLATE_SPECIALIZATION
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

//! \endcond
#endif
