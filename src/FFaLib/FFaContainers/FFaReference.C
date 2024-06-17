// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#include <iostream>
#include <iterator>
#include <string>
#include <cstring>
#include <cctype>

#include "FFaLib/FFaContainers/FFaReference.H"
#include "FFaLib/FFaContainers/FFaFieldContainer.H"
#include "FFaLib/FFaContainers/FFaReferenceList.H"


/*!
  Requirements:
  o Want a pointer from one object to another
  o From one to many
  o Automatic removal of connection on destruction of either side of reference
  o Automatic traversal/topology browsing
  o Automatic id->pointer resolving
  o Automatic IO
  o Runtime additions
  o Easy and robust to use

  In the referred object we need a table with those who refer to it.
  In object that prints the connection, the connection needs to be
  in the field container.


  TODO list :

  o Find out how to do Type::getClassTypeID()

  o For each object in HeadTable
    - Set up topology references

  o Testing :
    - FFaReference
      * IO and resolving
        - With written Type
        - Without written Type
      * Destruction and pointer updating
      * Changing pointer on the fly

    - FFaReferenceList
      * IO and resolving
        - With written Type
        - Without written Type
      * Destruction and pointer updating (removal of reference)
      * List access and editing
*/

FFaReferenceBase::FFaReferenceBase() : myOwnerFieldCont(NULL), myPtr(NULL)
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
  Sets the pointer to zero.
*/

void FFaReferenceBase::setPointerToNull()
{
  this->setRef(NULL);
}


/*!
  Sets the pointer value from a FFaFieldContainer pointer directly.
*/

void FFaReferenceBase::setRef(FFaFieldContainer* ptr)
{
  this->clearResolveRef();
  this->unbind();

  if (ptr && !ptr->isOfType(this->getRefClassTypeID()))
  {
    std::cerr <<"FFaReferenceBase::setRef: Incompatible field container types, "
              <<" this = "<< this->getRefClassTypeID()
              <<" ptr = "<< ptr->getTypeID() << "("<< ptr->getTypeIDName()
              <<")"<< std::endl;
    myPtr = NULL;
    return;
  }

  myPtr = ptr;
  this->bind();
}


/*!
  Sets the unresolved id and type of the referred object directly.
  Must be resolved later. Can be used when reading the references in another way
  than through the read() method.
*/

void FFaReferenceBase::setRef(int objId, int typeId)
{
  this->setRefID(objId);
  this->setRefTypeID(typeId);
}

void FFaReferenceBase::setRef(int objId, int typeId,
                              const std::vector<int>& assID)
{
  this->setRefID(objId);
  this->setRefTypeID(typeId);
  this->setRefAssemblyID(assID);
}


/*!
  Returns whether this reference has resolved its possible read ID.
  True as long as no myUnresolvedRef exists.
*/

bool FFaReferenceBase::isResolved() const
{
  return IAmResolved;
}


/*!
  Returns whether the pointer is zero.
*/

bool FFaReferenceBase::isNull() const
{
  if (IAmResolved && IAmBound)
    return !myPtr;

  return true;
}


/*!
  Returns the FFaFieldContainer pointer contained.
*/

FFaFieldContainer* FFaReferenceBase::getRef() const
{
  if (IAmResolved && IAmBound)
    return myPtr;

  if (myPtr && IAmResolved)
    std::cerr <<"FFaReferenceBase::getRef returning NULL because I am not bound ("
              << myPtr->getTypeIDName() <<" "<< myPtr->getResolvedID()
              <<")"<< std::endl;

  return NULL;
}


/*!
  Sets the context name for this reference. Used when doing topology browsing
  to know why a reference is referring to a certain FFaFieldContainer.
  Supposed to be set upon initialization when adding the reference to
  the book keeping in the FFaFieldContainer it is residing in.
  \sa FFaReferenceBase::getContextName
  \sa FFaReferenceListBase::setContextName
  \sa FFaReferenceListBase::getContextName
*/

void FFaReferenceBase::setContextName(const char* name)
{
  myContextName = name;
}


/*!
  Returns the contextName used by this reference.
  If this reference is a member of a FFaReferenceList, this method
  will return the context name of that FFaReferenceList.
  \sa FFaReferenceBase::setContextName
  \sa FFaReferenceListBase::setContextName
  \sa FFaReferenceListBase::getContextName
*/

const char* FFaReferenceBase::getContextName() const
{
  if (IAmInAList && myOwnerReferenceList)
    return myOwnerReferenceList->getContextName();
  else if (myContextName)
    return myContextName;
  else
    return "";
}


/*!
  Returns the type name (as of the FFaTypeCheck system) of the referred object.
*/

const char* FFaReferenceBase::getRefTypeName() const
{
  if (this->getRef())
    return this->getRef()->getTypeIDName();
  else
    return NULL;
}


/*!
  Returns the type id (as of the FFaTypeCheck system) of the referred object.
*/

int FFaReferenceBase::getRefTypeID() const
{
  if (!IAmResolved)
    return myUnresolvedRef->front();
  else if (IAmBound && myPtr)
    return myPtr->getTypeID();
  else
    return 0;
}


/*!
  Returns the id number of the referred object, or the read id if not resolved.
*/

int FFaReferenceBase::getRefID() const
{
  if (!IAmResolved)
    return myUnresolvedRef->operator[](1);
  else if (IAmBound && myPtr)
    return myPtr->getResolvedID();
  else
    return 0;
}


/*!
  Returns the assembly id array of the referred object,
  or the read assembly id if not resolved.
*/

void FFaReferenceBase::getRefAssemblyID(std::vector<int>& assID) const
{
  if (!IAmResolved)
    assID.assign(myUnresolvedRef->begin()+2,myUnresolvedRef->end());
  else if (IAmBound && myPtr)
    myPtr->getResolvedAssemblyID(assID);
  else
    assID.clear();
}


/*!
  Finds the object that has the correct type, and the correct ID compared to
  what was read from file, and sets up the pointer relation.
*/

void FFaReferenceBase::resolve(FFaDynCB4<FFaFieldContainer*&,int,int,
                                         const std::vector<int>&>& findCB)
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
  std::vector<int> assID;
  this->getRefAssemblyID(assID);
  findCB.invoke(foundObj, typeID, this->getRefID(), assID);

  if (foundObj)
    this->setRef(foundObj);
  else
  {
    FFaFieldContainer* owner = this->getOwnerFieldContainer();
    std::cerr <<"FFaReferenceBase::resolve: Resolve failure (TypeID="
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
      for (size_t i = 0; i < assID.size(); i++)
        std::cerr <<","<< assID[i];
      std::cerr <<" ("<< this->getContextName();
    }
    std::cerr <<")"<< std::endl;
  }

#if FFA_DEBUG > 1
  FFaFieldContainer* ofc = this->getOwnerFieldContainer();
  if (ofc && this->getRef())
    std::cout <<"FFaReferenceBase: "<< this->getContextName()
              <<" in " << ofc->getTypeIDName() <<" "<< ofc->getResolvedID()
              <<" resolved to "<< foundObj->getTypeIDName()
              <<" "<< foundObj->getResolvedID() << std::endl;
#endif
}


/*!
  Does the opposite of resolve.
*/

void FFaReferenceBase::unresolve()
{
  if (!IAmResolved || !myPtr)
    return;

#ifdef FFA_DEBUG
  FFaFieldContainer* ofc = this->getOwnerFieldContainer();
  if (ofc)
    std::cout <<"FFaReferenceBase: "<< this->getContextName() <<" in "
              << ofc->getTypeIDName() <<" "<< ofc->getResolvedID()
              <<" unresolved"<< std::endl;
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
    FFaFieldContainer* ofc = this->getOwnerFieldContainer();
    if (ofc)
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


void FFaReferenceBase::updateAssemblyRef(const std::vector<int>& from,
                                         const std::vector<int>& to)
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
  {
    std::vector<int> assID;
    this->getRefAssemblyID(assID);
    if (!assID.empty())
    {
      os <<"aID: ";
      std::copy(assID.begin(),assID.end(),std::ostream_iterator<int>(os," "));
      os <<"uID: ";
    }
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
      std::vector<int> assID;
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


/*!
  Tells the object that this reference is pointing at that it is pointed at.
*/

void FFaReferenceBase::bind()
{
  if (IAmResolved && !IAmBound && myPtr && myOwnerFieldCont)
  {
    IAmBound = true;
    myPtr->insertInRefBy(this);
  }
  else if (IAmResolved && myPtr && !myOwnerFieldCont)
    std::cerr <<"FFaReferenceBase::bind failed because myOwnerFieldCont==NULL ("
              << myPtr->getTypeIDName() <<" "<< myPtr->getResolvedID()
              <<")"<< std::endl;
}


/*!
  Tells the referred object that it is not referred anymore.
*/

void FFaReferenceBase::unbind()
{
  if (IAmResolved && IAmBound && myPtr && myOwnerFieldCont)
    myPtr->deleteFromRefBy(this);

  IAmBound = false;
}


/*!
  Deletes the struct used to hold the temporary information read from disk.
*/

void FFaReferenceBase::clearResolveRef()
{
  if (!IAmResolved && myUnresolvedRef)
  {
    delete myUnresolvedRef;
    myUnresolvedRef = NULL;
    IAmResolved = true;
  }
}


/*!
  A copy method mostly used by FFaFieldContainer::copy().
*/

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
  else
    this->setRef(aRef.getRef());
}


/*!
  Convenience method used when reading from file.
*/

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


/*!
  Convenience method used when reading from file.
*/

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


/*!
  Convenience method used when reading from file.
*/

void FFaReferenceBase::setRefAssemblyID(const std::vector<int>& assID)
{
  this->unresolve();

  if (IAmResolved)
  {
    myUnresolvedRef = new UnResolvedID({ 0, 0 });
    IAmResolved = false;
  }

  myUnresolvedRef->insert(myUnresolvedRef->end(),assID.begin(),assID.end());
}


/*!
  Only used by the FFaReferenceListBase to clean a ToRef.
*/

void FFaReferenceBase::zeroOut()
{
  if (IAmResolved)
  {
    myPtr = NULL;
    IAmBound = false;
  }
}


/*!
  Only used by FFaFieldContainer to notify that the FieldContainer is obsolete.
*/

void FFaReferenceBase::zeroOutOrRemoveFromList()
{
  this->zeroOut();

  if (IAmInAList && myOwnerReferenceList)
    myOwnerReferenceList->eraseReferenceIfNeeded(this);
}


/*!
  Sets what FFaFieldContainer this reference is a member of.
*/

void FFaReferenceBase::setOwnerFieldContainer(FFaFieldContainer* owner)
{
  IAmInAList = false;
  myOwnerFieldCont = owner;
}


/*!
  Returns what FFaFieldContainer this reference is a member of (if any).
  This could be the FFaFieldContainer of the FFaReferenceListBase
  that this reference is a member of.
*/

FFaFieldContainer* FFaReferenceBase::getOwnerFieldContainer() const
{
  if (!IAmInAList)
    return myOwnerFieldCont;
  else if (myOwnerReferenceList)
    return myOwnerReferenceList->getOwnerFieldContainer();
  else
    return static_cast<FFaFieldContainer*>(NULL);
}


/*!
  Sets what FFaReferenceListBase this reference is a member of (if any).
*/

void FFaReferenceBase::setOwnerReferenceList(FFaReferenceListBase* owner)
{
  IAmInAList = true;
  myOwnerReferenceList = owner;
}


/*!
  Returns what FFaReferenceListBase this reference is a member of (if any).
*/

FFaReferenceListBase* FFaReferenceBase::getOwnerReferenceList() const
{
  if (IAmInAList)
    return myOwnerReferenceList;
  else
    return static_cast<FFaReferenceListBase*>(NULL);
}


/*!
  Checks for equality between two FFaReferences.
*/

bool FFaReferenceBase::isEqual(const FFaReferenceBase* p) const
{
  if (IAmResolved && p->IAmResolved)
    return this->getRef() == p->getRef();
  else if (IAmResolved || p->IAmResolved)
    return false;

  return *myUnresolvedRef == *(p->myUnresolvedRef);
}
