// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

/*!
  \file FFaReference.C
  \brief Classes representing guarded pointers.
*/

#include <iostream>
#include <string>
#include <cstring>
#include <cctype>

#include "FFaLib/FFaContainers/FFaReference.H"
#include "FFaLib/FFaContainers/FFaFieldContainer.H"
#include "FFaLib/FFaContainers/FFaReferenceList.H"


/*!
  \class FFaReferenceBase

  Requirements:
  - Want a pointer from one object to another
  - From one to many
  - Automatic removal of connection on destruction of either side of reference
  - Automatic traversal/topology browsing
  - Automatic id to pointer resolving
  - Automatic I/O
  - Runtime additions
  - Easy and robust to use

  In the referred object we need a table with those who refer to it.
  In object that prints the connection, the connection needs to be
  in the field container.

  \todo
  - Find out how to do Type::getClassTypeID()
  - For each object in HeadTable
    - Set up topology references
  - Testing:
    - FFaReference
      - IO and resolving
        - With written Type
        - Without written Type
      - Destruction and pointer updating
      - Changing pointer on the fly
    - FFaReferenceList
      - IO and resolving
        - With written Type
        - Without written Type
      - Destruction and pointer updating (removal of reference)
      - List access and editing
*/

FFaReferenceBase::FFaReferenceBase() : myOwnerObj(NULL), myPtr(NULL)
{
  myContextName = NULL;
  printIfZero = IAmResolved = true;
  IAmBound = IAmInAList = false;
}


FFaReferenceBase::~FFaReferenceBase()
{
  this->clearResolveRef();
  this->unbind();
}


/*!
  Returns \e true if a new pointer value is assigned, otherwise \e false.
*/

bool FFaReferenceBase::setRef(FFaFieldContainer* ptr)
{
  if (ptr == myPtr && IAmResolved && IAmBound)
    return false; // unchanged

  this->clearResolveRef();
  this->unbind();

  if (ptr && !ptr->isOfType(this->getRefClassTypeID()))
  {
    std::cerr <<"FFaReferenceBase::setRef: Incompatible field container types, "
              <<" this = "<< this->getRefClassTypeID()
              <<" ptr = "<< ptr->getTypeID() << "("<< ptr->getTypeIDName()
              <<")"<< std::endl;
    myPtr = NULL;
  }
  else
  {
    myPtr = ptr;
    this->bind();
  }

  return true;
}


/*!
  Must be resolved later. Can be used when reading the references in another way
  than through the read() method.
*/

void FFaReferenceBase::setRef(int objId, int typeId)
{
  this->setRefID(objId);
  this->setRefTypeID(typeId);
}

void FFaReferenceBase::setRef(int objId, int typeId, const IntVec& assID)
{
  this->setRefID(objId);
  this->setRefTypeID(typeId);
  this->setRefAssemblyID(assID);
}


FFaFieldContainer* FFaReferenceBase::getRef() const
{
  if (IAmResolved && IAmBound)
    return myPtr;

  if (myPtr && IAmResolved)
    std::cerr <<"FFaReferenceBase::getRef returns NULL because I am not bound ("
              << myPtr->getTypeIDName() <<" "<< myPtr->getResolvedID()
              <<")"<< std::endl;

  return NULL;
}


/*!
  This method is used when doing topology browsing to know why a reference is
  referring to a certain FFaFieldContainer. Supposed to be set upon
  initialization when adding the reference to the book keeping in the
  FFaFieldContainer it is residing in.
  If this reference is a member of a FFaReferenceList,
  this method will return the context name of that FFaReferenceList.
  \sa FFaReferenceListBase::setContextName
  \sa FFaReferenceListBase::getContextName
*/

const char* FFaReferenceBase::getContextName() const
{
  if (IAmInAList && myOwnerList)
    return myOwnerList->getContextName();
  else if (myContextName)
    return myContextName;
  else
    return "";
}


const char* FFaReferenceBase::getRefTypeName() const
{
  if (this->getRef())
    return this->getRef()->getTypeIDName();
  else
    return NULL;
}


int FFaReferenceBase::getRefTypeID() const
{
  if (!IAmResolved)
    return myUnresolvedRef->front();
  else if (IAmBound && myPtr)
    return myPtr->getTypeID();
  else
    return 0;
}


int FFaReferenceBase::getRefID() const
{
  if (!IAmResolved)
    return myUnresolvedRef->operator[](1);
  else if (IAmBound && myPtr)
    return myPtr->getResolvedID();
  else
    return 0;
}


bool FFaReferenceBase::getRefAssemblyID(IntVec& assID) const
{
  if (!IAmResolved)
    assID.assign(myUnresolvedRef->begin()+2,myUnresolvedRef->end());
  else if (IAmBound && myPtr)
    myPtr->getResolvedAssemblyID(assID);
  else
    assID.clear();

  return !assID.empty();
}


/*!
  This method sets up the pointer relation to the object with correct
  Type and ID compared to what was read from file.
*/

void FFaReferenceBase::resolve(FFaSearcher& findCB)
{
  if (IAmResolved)
    return;

  if (this->getRefID() == 0)
  {
    // This reference has no value - no need to search for an object
    this->setRef(NULL);
    return;
  }

  int typeID = this->getRefTypeID();
  if (typeID == 0)
    typeID = this->getRefClassTypeID();
  else if (typeID < 0)
    return; // invalid typeID - cannot resolve

  FFaFieldContainer* foundObj = NULL;
  IntVec assID;
  this->getRefAssemblyID(assID);
  findCB.invoke(foundObj, typeID, this->getRefID(), assID);

  if (foundObj)
    this->setRef(foundObj);
  else
  {
    FFaFieldContainer* owner = this->getOwnerFieldContainer();
    std::cerr <<"FFaReferenceBase::resolve(): Resolve failure (TypeID="
              << typeID <<" "<< FFaTypeCheck::getTypeNameFromID(typeID)
              <<", ID="<< this->getRefID();
    if (!assID.empty())
    {
      std::cerr <<" AssId="<< assID.front();
      for (size_t i = 1; i < assID.size(); i++)
        std::cerr <<","<< assID[i];
    }
    if (owner)
    {
      assID.clear();
      owner->getResolvedAssemblyID(assID);
      std::cerr <<")\n                           Referred by "
                << owner->getTypeIDName() <<" "<< owner->getResolvedID();
      for (int id : assID) std::cerr <<","<< id;
      std::cerr <<" ("<< this->getContextName();
    }
    std::cerr <<")"<< std::endl;
  }

#if FFA_DEBUG > 1
  if (this->getRef())
    if (FFaFieldContainer* ofc = this->getOwnerFieldContainer(); ofc)
      std::cout <<"FFaReferenceBase::resolve():  "<< this->getContextName()
                <<" in " << ofc->getTypeIDName() <<" "<< ofc->getResolvedID()
                <<" resolved to "<< foundObj->getTypeIDName()
                <<" "<< foundObj->getResolvedID() << std::endl;
#endif
}


void FFaReferenceBase::unresolve()
{
  if (!IAmResolved || !myPtr)
    return;

#ifdef FFA_DEBUG
  if (FFaFieldContainer* ofc = this->getOwnerFieldContainer(); ofc)
    std::cout <<"FFaReferenceBase::unresolve(): "<< this->getContextName()
              <<" in " << ofc->getTypeIDName() <<" "<< ofc->getResolvedID()
              << std::endl;
#endif

  UnResolvedID* uResRef = new UnResolvedID({ myPtr->getTypeID(), myPtr->getResolvedID() });
  myPtr->getResolvedAssemblyID(*uResRef);

  this->unbind();
  myUnresolvedRef = uResRef;
  IAmResolved = false;
}


void FFaReferenceBase::updateAssemblyRef(int from, int to, size_t ind)
{
  if (IAmResolved || !myUnresolvedRef)
    return;

  if (from == 0 && to > 0)
  {
    // We are extending the assembly hierarchy one level
#ifdef FFA_DEBUG
    std::cout <<"FFaReferenceBase::updateAssemblyRef("<< from <<","<< to
              <<") for "<< this->getContextName();
    if (FFaFieldContainer* ofc = this->getOwnerFieldContainer(); ofc)
      std::cout <<" in "<< ofc->getTypeIDName() <<" "<< ofc->getResolvedID();
    std::cout << std::endl;
#endif

    if (strcmp(this->getContextName(),"myParentAssembly") == 0)
      if (myUnresolvedRef->size() == 2 && myUnresolvedRef->back() == to)
        return; // The parent assembly reference is already up to date

    myUnresolvedRef->insert(myUnresolvedRef->begin()+2,0);
  }

  if (ind+2 >= myUnresolvedRef->size())
    return;

  int& unResolvedRef = myUnresolvedRef->operator[](ind+2);
  if (unResolvedRef == from)
    unResolvedRef = to;
}


void FFaReferenceBase::updateAssemblyRef(const IntVec& from,
                                         const IntVec& to)
{
  if (IAmResolved || !myUnresolvedRef)
    return;

  if (from.empty() || from.size() != to.size())
    return;

  for (size_t i = 2; i < myUnresolvedRef->size() && i-2 < from.size(); i++)
    if (myUnresolvedRef->operator[](i) != from[i-2])
      return;

  for (size_t i = 2; i < myUnresolvedRef->size() && i-2 < to.size(); i++)
    myUnresolvedRef->operator[](i) = to[i-2];
 }


void FFaReferenceBase::write(std::ostream& os) const
{
  if (!this->isNull())
    if (IntVec assID; this->getRefAssemblyID(assID))
    {
      os <<"aID: ";
      for (int id : assID) os << id <<" ";
      os <<"uID: ";
    }

  os << this->getRefID();

  if (this->isNull())
    return;
  if (this->getRefTypeID() == this->getRefClassTypeID())
    return;

  os <<" "<< this->getRefTypeName();
}


void FFaReferenceBase::read(std::istream& is)
{
  // Check if we can read the reference ID directly,
  // or whether we must read an assembly ID first

  char c;
  bool valid = true;
  std::string tmpStr;
  int id = 0;

  is >> id;

  if (is)
    this->setRefID(id);
  else
  {
    valid = false;
    char token[10];

    // No int was found, try to read assembly ID start token
    is.clear();
    is.getline(token,10,':');
    if (is && strcmp(token,"aID") == 0)
    {
      // Read the vector of assembly IDs
      IntVec assID;
      is >> id;
      while (is)
      {
        assID.push_back(id);
        is >> id;
      }
      this->setRefAssemblyID(assID);

      // End has been reached : check if it is valid
      is.clear();
      is.getline(token,10,':');
      if (is && strcmp(token,"uID") == 0)
      {
        is >> id;
        if (is)
        {
          valid = true;
          this->setRefID(id);
        }
        else
          std::cerr <<"FFaReferenceBase::read: Error reading user ID "
                    <<"after reading the assembly ID"<< std::endl;
      }
      else
        std::cerr <<"FFaReferenceBase::read: Error reading the reference ID, "
                  <<"expected \"uID:\" for assembly id end but got \""
                  << tmpStr <<"\""<< std::endl;
    }
    else
      std::cerr <<"FFaReferenceBase::read: Error reading the reference ID, "
                <<"expected ID number or \"aID:\" for assembly id start, got \""
                << tmpStr <<"\""<< std::endl;
  }

  if (!valid)
    return;

  // Read optional reference typename

  tmpStr.clear();

  // Skip whitespaces
  while (is.get(c) && isspace(c));

  // Read the type name, if any.
  // Fixed 060712 (kmo): Must check against 'a' in case this reference is in a
  // list where we just read an ID without any type name or sub-assembly id,
  // and the next one is in a sub-assembly.
  if (isalpha(c) && c != 'a')
    while (isalnum(c) || c == '_')
    {
      tmpStr += c;
      is.get(c);
    }
  is.putback(c); // putting back endsign, space or whatever

  if (tmpStr.empty())
    return; // no reference typename given, use RefClassTypeId

  // This sets typeID to -1 (and not 0) if tmpStr contains an invalid type name.
  // If it is set to 0, we may risk resolving it to an object of wrong type if
  // an object with the same user ID is found.
  this->setRefTypeID(FFaTypeCheck::getTypeIDFromName(tmpStr.c_str()));
}


void FFaReferenceBase::bind()
{
  if (IAmResolved && !IAmBound && myPtr && myOwnerObj)
  {
    IAmBound = true;
    myPtr->insertInRefBy(this);
  }
  else if (IAmResolved && myPtr && !myOwnerObj)
    std::cerr <<"FFaReferenceBase::bind failed because myOwnerObj==NULL ("
              << myPtr->getTypeIDName() <<" "<< myPtr->getResolvedID()
              <<")"<< std::endl;
}


void FFaReferenceBase::unbind()
{
  if (IAmResolved && IAmBound && myPtr && myOwnerObj)
    myPtr->deleteFromRefBy(this);

  IAmBound = false;
}


void FFaReferenceBase::clearResolveRef()
{
  if (!IAmResolved && myUnresolvedRef)
  {
    delete myUnresolvedRef;
    myUnresolvedRef = NULL;
    IAmResolved = true;
  }
}


void FFaReferenceBase::copy(const FFaReferenceBase& aRef, bool unresolve)
{
  this->clearResolveRef();
  this->unbind();

  if (!aRef.IAmResolved)
  {
    myUnresolvedRef = new UnResolvedID(*aRef.myUnresolvedRef);
    IAmResolved = false;
  }
  else if (unresolve)
  {
    myUnresolvedRef = new UnResolvedID({ aRef.getRefTypeID(), aRef.getRefID() });
    aRef.getRefAssemblyID(*myUnresolvedRef);
    IAmResolved = false;
  }
  else if (aRef.isNull())
    this->setRef(NULL);
  else
  {
    this->setRef(aRef.getRef());
#if FFA_DEBUG > 1
    std::cout <<"FFaReferenceBase::copy(): "
              << this->getRefTypeName() <<" ["<< this->getRefID() <<"] "
              << aRef.getContextName() <<" to "<< this->getContextName();
    if (FFaFieldContainer* ofc = this->getOwnerFieldContainer(); ofc)
      std::cout <<" in "<< ofc->getTypeIDName() <<" "<< ofc->getResolvedID();
    std::cout << std::endl;
#endif
  }
}


void FFaReferenceBase::setRefID(int id)
{
  this->unresolve();

  if (IAmResolved)
  {
    myUnresolvedRef = new UnResolvedID({ 0, id });
    IAmResolved = false;
  }
  else
    myUnresolvedRef->operator[](1) = id;
}


void FFaReferenceBase::setRefTypeID(int id)
{
  this->unresolve();

  if (IAmResolved)
  {
    myUnresolvedRef = new UnResolvedID({ id, 0 });
    IAmResolved = false;
  }
  else
    myUnresolvedRef->front() = id;
}


void FFaReferenceBase::setRefAssemblyID(const IntVec& assID)
{
  this->unresolve();

  if (IAmResolved)
  {
    myUnresolvedRef = new UnResolvedID({ 0, 0 });
    IAmResolved = false;
  }

  myUnresolvedRef->resize(2);
  myUnresolvedRef->insert(myUnresolvedRef->end(),assID.begin(),assID.end());
}


void FFaReferenceBase::zeroOut()
{
  if (IAmResolved)
  {
    myPtr = NULL;
    IAmBound = false;
  }
}


void FFaReferenceBase::zeroOutOrRemoveFromList()
{
  this->zeroOut();

  if (IAmInAList && myOwnerList)
    myOwnerList->eraseReferenceIfNeeded(this);
}


void FFaReferenceBase::setOwnerFieldContainer(FFaFieldContainer* owner)
{
  IAmInAList = false;
  myOwnerObj = owner;
}


FFaFieldContainer* FFaReferenceBase::getOwnerFieldContainer() const
{
  if (!IAmInAList)
    return myOwnerObj;
  else if (myOwnerList)
    return myOwnerList->getOwnerFieldContainer();
  else
    return static_cast<FFaFieldContainer*>(NULL);
}


void FFaReferenceBase::setOwnerReferenceList(FFaReferenceListBase* owner)
{
  IAmInAList = true;
  myOwnerList = owner;
}


FFaReferenceListBase* FFaReferenceBase::getOwnerReferenceList() const
{
  if (IAmInAList)
    return myOwnerList;
  else
    return static_cast<FFaReferenceListBase*>(NULL);
}


bool FFaReferenceBase::isEqual(const FFaReferenceBase* p) const
{
  if (IAmResolved && p->IAmResolved)
    return this->getRef() == p->getRef();
  else if (IAmResolved || p->IAmResolved)
    return false;

  return *myUnresolvedRef == *(p->myUnresolvedRef);
}
