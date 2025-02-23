// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#ifndef FFA_REFERENCE_H
#define FFA_REFERENCE_H

#include "FFaLib/FFaContainers/FFaField.H"
#include "FFaLib/FFaDynCalls/FFaDynCB.H"

class FFaFieldContainer;
class FFaReferenceListBase;

//! Callback searching for FFaFieldContainer objects when resolving references
using FFaSearcher = FFaDynCB4<FFaFieldContainer*&,int,int,const IntVec&>;


/*!
  \brief Base class for the guarded pointer FFaReference class.
*/

class FFaReferenceBase
{
  // Remove implicitly defined copy constructor and assignment operator
  FFaReferenceBase(const FFaReferenceBase&) = delete;
  FFaReferenceBase& operator=(const FFaReferenceBase&) = delete;

public:
  FFaReferenceBase();
  virtual ~FFaReferenceBase();

  // Assigning

  void setRef(FFaFieldContainer* ptr);
  void setRef(int objId, int typeId);
  void setRef(int objId, int typeId, const IntVec& assemblyIDs);
  void setPointerToNull();

  void copy(const FFaReferenceBase& aReference, bool unresolve = false);

  // Access

  FFaFieldContainer* getRef() const;
  bool isResolved() const;
  bool isNull() const;

  void setContextName(const char* name);
  const char* getContextName() const;

  // Type info access

  virtual int getRefClassTypeID() const { return -1; }

  const char* getRefTypeName() const;
  int getRefTypeID() const;
  int getRefID() const;
  void getRefAssemblyID(IntVec& assID) const;

  // IO

  void setPrintIfZero(bool doPrint) { printIfZero = doPrint; }
  bool printMeIfZero() const { return printIfZero; }

  void write(std::ostream& os) const;
  void read(std::istream& is);

protected:
  void resolve(FFaSearcher& findCB);
  void unresolve();
  void updateAssemblyRef(int from, int to, size_t ind = 0);
  void updateAssemblyRef(const IntVec& from, const IntVec& to);
  bool isEqual(const FFaReferenceBase* p) const;

  void bind();
  void unbind();
  void clearResolveRef();

private:
  // Friending FFaFieldContainer and FFaReferenceListBase
  // to let them access the methods below
  friend class FFaFieldContainer;
  friend class FFaReferenceListBase;

  void zeroOut();
  void zeroOutOrRemoveFromList();

  void setOwnerReferenceList(FFaReferenceListBase*);
  FFaReferenceListBase* getOwnerReferenceList() const;
  void setOwnerFieldContainer(FFaFieldContainer*);
  FFaFieldContainer* getOwnerFieldContainer() const;

  void setRefID(int id);
  void setRefTypeID(int id);
  void setRefAssemblyID(const IntVec& assemblyID);

  //! Type of unique identification of the unresolved references
  using UnResolvedID = std::vector<int>;

  bool IAmResolved; //!< False if we have an UnResolvedID object pointed to by myUnresolvedRef
  bool IAmBound;    //!< True if myPtr is pointing to a container, and the container knows it
  bool IAmInAList;  //!< True if this FFaReference is maintained by a FFaReferenceList

  union {
    FFaFieldContainer*    myOwnerFieldCont;
    FFaReferenceListBase* myOwnerReferenceList;
  };

  union {
    FFaFieldContainer* myPtr;
    UnResolvedID*      myUnresolvedRef;
  };

  const char* myContextName;
  bool printIfZero;
};


/*!
  \brief A "guarded pointer" like class.

  This class can be used in classes derived from FFaFieldContainer
  to refer to other classes derived from FFaFieldContainer. The class can be
  used much like a normal pointer because of the reimplemented operators.
  The pointer value encapsulated by this class will automatically be set to zero
  if the referred object is deleted.

  One of the main applications of this class is to use it together with FFaField
  and get automated IO of the reference.
  To use this feature, the FFaFieldContainer-derived class containing the
  reference needs a FFaField<FFaReferenceBase*> object that is initialized with
  the address of the FFaReference to read/write:

  \code
  class FmNewClass : FFaFieldContainer
  {
    // ...

    FFaReference<FmSomeClass>   someClassReference;
    FFaField<FFaReferenceBase*> someWrittenReference;

    // ...
  }

  FmNewClass::FmNewClass()
  {
    // ...

    // Adds field to book keeping in FFaFieldContainer Initializes

    FFA_REFERENCE_FIELD_INIT(someWrittenReference, someClassReference, "FILE_KEY_WORD");

    // ...
  }
  \endcode

  You can also use it as a guarded pointer outside any FFaFieldContainer.
  Any FFaReference can be assigned a contextName that can be used to look up
  why a particular reference is pointing to a FFaFieldContainer. When using the
  macro used above, the context name is automatically set to the same as the
  name of the reference. But it can be set to anything appropriate.
  \sa FFaReferenceBase::setContextName
*/

template<class Type>
class FFaReference : public FFaReferenceBase
{
public:
  FFaReference() {}
  virtual ~FFaReference() {}

  // Assigning

  void setPointer(Type* ptr) { this->setRef(ptr); }

  FFaReference<Type>& operator=(Type* ptr) { this->setRef(ptr); return *this; }

  // Access

  Type* getPointer() const { return   (Type*)this->getRef(); }
  Type* operator->() const { return   (Type*)this->getRef(); }
  Type& operator*()  const { return *((Type*)this->getRef()); }
  operator Type*()   const { return   (Type*)this->getRef();}

  // Equality

  bool operator==(const FFaReference<Type>& p) const { return this->isEqual(&p); }
  bool operator!=(const FFaReference<Type>& p) const { return !this->isEqual(&p); }

  // Returns the type ID number that this reference is supposed to refer to

  virtual int getRefClassTypeID() const { return Type::getClassTypeID(); }
};


// Specialized instantiations of FFaField methods.
// The pointer value in fields of reference pointers is not supposed to change.

using FFaRefPtr = FFaReferenceBase*;

template<> inline
FFaField<FFaRefPtr>::FFaField()
{
  myData = myDefaultValue = NULL;
}

template<> inline bool
FFaField<FFaRefPtr>::isDataField() const
{
  return false;
}

template<> inline bool
FFaField<FFaRefPtr>::isPrintable() const
{
  return myData ? (myData->isNull() ? myData->printMeIfZero() : true) : false;
}

template<> inline void
FFaField<FFaRefPtr>::write(std::ostream& os) const
{
  if (myData) myData->write(os);
}

template<> inline void
FFaField<FFaRefPtr>::read(std::istream& is, FFaRefPtr& v) const
{
  if (v) v->read(is);
}

#endif
