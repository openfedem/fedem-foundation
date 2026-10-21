// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

/*!
  \file FFaReference.H
  \brief Classes representing guarded pointers.
*/

#ifndef FFA_REFERENCE_H
#define FFA_REFERENCE_H

#include "FFaLib/FFaContainers/FFaField.H"
#include "FFaLib/FFaDynCalls/FFaDynCB.H"

class FFaFieldContainer;
class FFaReferenceListBase;

//! Callback searching for FFaFieldContainer objects when resolving references
using FFaSearcher = FFaDynCB4<FFaFieldContainer*&,int,int,const IntVec&>;


/*!
  \brief Base class for the template class FFaReference for guarded pointers.
*/

class FFaReferenceBase
{
  //! \brief Remove implicitly defined copy constructor.
  FFaReferenceBase(const FFaReferenceBase&) = delete;
  //! \brief Remove implicitly defined assignment operator.
  FFaReferenceBase& operator=(const FFaReferenceBase&) = delete;

public:
  //! \brief The default constructor initializes the members.
  FFaReferenceBase();
  //! \brief The destructor clears the cross references.
  virtual ~FFaReferenceBase();

  // Assigning

  //! \brief Sets the pointer value from a FFaFieldContainer pointer directly.
  bool setRef(FFaFieldContainer* ptr);
  //! \brief Sets the unresolved id and type of the referred object.
  void setRef(int objId, int typeId);
  //! \brief Sets the unresolved id and type of object in a subassembly.
  void setRef(int objId, int typeId, const IntVec& assemblyIDs);
  //! \brief Clears the reference.
  bool setPointerToNull() { return this->setRef(NULL); }

  //! \brief A copy method mostly used by FFaFieldContainer::copy().
  void copy(const FFaReferenceBase& aReference, bool unresolve = false);

  // Access

  //! \brief Returns the FFaFieldContainer pointer contained.
  FFaFieldContainer* getRef() const;
  //! \brief Returns whether this reference has been resolved or not.
  bool isResolved() const { return IAmResolved; }
  //! \brief Returns whether this reference points to an actual object.
  bool isNull() const { return IAmResolved && IAmBound ? !myPtr : true; }

  //! \brief Assigns the context name for this reference.
  void setContextName(const char* name) { myContextName = name; }
  //! \brief Returns the context name used by this reference.
  const char* getContextName() const;

  // Type info access

  //! \brief Returns the type ID that this reference is supposed to refer to.
  virtual int getRefClassTypeID() const { return -1; }

  //! \brief Returns the type name of the referred object.
  const char* getRefTypeName() const;
  //! \brief Returns the type id of the referred object.
  int getRefTypeID() const;
  //! \brief Returns the id number of the referred object.
  //! \details If the reference is not resolved, the read id returned.
  int getRefID() const;
  //! \brief Returns the assembly id array of the referred object.
  //! \details If the reference is not resolved, the read assembly id returned.
  bool getRefAssemblyID(IntVec& assID) const;

  // IO

  //! \brief Flags whether this reference should be printed even if NULL.
  void setPrintIfZero(bool doPrint) { printIfZero = doPrint; }
  //! \brief Returns if this reference should be printed even if NULL.
  bool printMeIfZero() const { return printIfZero; }

  //! \brief Writes the reference to specified output stream.
  void write(std::ostream& os) const;
  //! \brief Reads the reference from specified input stream.
  void read(std::istream& is);

protected:
  //! \brief Finds the object that has matching Type and ID as read from file.
  void resolve(FFaSearcher& findCB);
  //! \brief Does the opposite of resolve.
  void unresolve();
  //! \brief Updates the assembly reference when moving to another assembly.
  void updateAssemblyRef(int from, int to, size_t ind = 0);
  //! \brief Updates the assembly reference when moving to another assembly.
  void updateAssemblyRef(const IntVec& from, const IntVec& to);
  //! \brief Checks for equality between two references.
  bool isEqual(const FFaReferenceBase* p) const;

  //! \brief Tells the object this reference is pointing at that it is referred.
  void bind();
  //! \brief Tells the referred object that it is not being referred anymore.
  void unbind();
  //! \brief Deletes the variable used to hold temporary ID information.
  void clearResolveRef();

private:
  //! \brief Grants access to some private methods for resolving references.
  friend class FFaFieldContainer;
  //! \brief Grants access to some private methods for resolving references.
  friend class FFaReferenceListBase;

  //! \brief Only used by the FFaReferenceListBase to clean a ToRef.
  void zeroOut();
  //! \brief Only used by FFaFieldContainer to notify that it is obsolete.
  void zeroOutOrRemoveFromList();

  //! \brief Sets the FFaReferenceListBase that this reference is a member of.
  void setOwnerReferenceList(FFaReferenceListBase*);
  //! \brief Returns the FFaReferenceListBase this reference is a member of.
  FFaReferenceListBase* getOwnerReferenceList() const;
  //! \brief Sets the FFaFieldContainer that this reference is a member of.
  void setOwnerFieldContainer(FFaFieldContainer*);
  //! \brief Returns the FFaFieldContainer this reference is a member of.
  //! \details This could also be the FFaFieldContainer of the
  //! FFaReferenceListBase that this reference is a member of.
  FFaFieldContainer* getOwnerFieldContainer() const;

  //! \brief Convenience method used when reading from file.
  void setRefID(int id);
  //! \brief Convenience method used when reading from file.
  void setRefTypeID(int id);
  //! \brief Convenience method used when reading from file.
  void setRefAssemblyID(const IntVec& assemblyID);

  //! Type of unique identification of the unresolved references
  using UnResolvedID = std::vector<int>;

  //! \e false if we have an UnResolvedID object pointed to by #myUnresolvedRef
  bool IAmResolved;
  //! \e true if #myPtr is pointing to a container, and the container knows it
  bool IAmBound;
  //! \e true if this reference is maintained by a FFaReferenceList
  bool IAmInAList;

  //! \brief Shared memory segment for alternative owner types
  union {
    FFaFieldContainer*    myOwnerObj;  //!< Points to the owner object
    FFaReferenceListBase* myOwnerList; //!< Points to the owner reference list
  };

  //! \brief Shared memory segment for pointers to resolved and unresolved data
  union {
    FFaFieldContainer* myPtr; //!< Points to the referred object when resolved
    UnResolvedID* myUnresolvedRef; //!< Id of referred object before resolving
  };

  const char* myContextName; //!< Typically, this will be the variable name
  bool        printIfZero;   //!< If \e true, print the reference also when NULL
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

template<class Type> class FFaReference : public FFaReferenceBase
{
public:

  // Assigning

  //! \brief Assigns \a ptr to this reference.
  bool setPointer(Type* ptr) { return this->setRef(ptr); }
  //! \brief Overloaded assignment operator.
  FFaReference<Type>& operator=(Type* ptr) { this->setRef(ptr); return *this; }

  // Access

  //! \brief Returns the type-casted pointer this reference points to.
  Type* getPointer() const { return (Type*)this->getRef(); }
  //! \brief Pointer operator.
  operator Type*()   const { return  this->getPointer(); }
  //! \brief Pointer operator.
  Type* operator->() const { return  this->getPointer(); }
  //! \brief Dereference operator.
  Type& operator*()  const { return *this->getPointer(); }

  //! \brief Equality operator.
  bool operator==(const FFaReference<Type>& p) const { return this->isEqual(&p); }
  //! \brief Inequality operator.
  bool operator!=(const FFaReference<Type>& p) const { return !this->isEqual(&p); }

  //! \brief Returns the class type ID this reference is instantiated over.
  virtual int getRefClassTypeID() const { return Type::getClassTypeID(); }
};


// Specialized instantiations of FFaField methods.
// The pointer value in fields of reference pointers is not supposed to change.

//! \cond TEMPLATE_SPECIALIZATION
using FFaRefPtr = FFaReferenceBase*; 


template<>
inline FFaField<FFaRefPtr>::FFaField()
{
  myData = myDefaultValue = NULL;
}

template<>
inline bool FFaField<FFaRefPtr>::isDataField() const
{
  return false;
}

template<>
inline bool FFaField<FFaRefPtr>::isPrintable() const
{
  return myData ? (myData->isNull() ? myData->printMeIfZero() : true) : false;
}

template<>
inline void FFaField<FFaRefPtr>::write(std::ostream& os) const
{
  if (myData) myData->write(os);
}

template<>
inline void FFaField<FFaRefPtr>::read(std::istream& is, FFaRefPtr& v) const
{
  if (v) v->read(is);
}

//! \endcond
#endif
