// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#include "FFaReferenceList.H"
#include <cctype>


FFaReferenceListBase::FFaReferenceListBase() : IAmAutoSizing(true)
{
  myOwner = NULL;
  myContextName = NULL;
}


FFaReferenceListBase::~FFaReferenceListBase()
{
  this->clear();
}


/*!
  Reorders the elements in this list such that the one pointed to by \a newFirst
  becomes the first member.
*/

bool FFaReferenceListBase::shuffle(size_t newFirst)
{
  if (newFirst == 0) return true;
  if (newFirst >= myRefs.size()) return false;

  std::list<FFaReferenceBase*>::iterator first = myRefs.begin();
  for (size_t i = 0; i < newFirst; i++) ++first;
  myRefs.splice(myRefs.begin(),myRefs,first,myRefs.end());
  return true;
}


/*!
  Removes all instances of \a ptr from this list. If \a notifyContainer is
  false the reference is removed without decrementing the FFaFieldContainer it
  points to, telling that it no longer does so.
  This is to be used in cleanup during destruction of FFaFieldContainer.
*/

void FFaReferenceListBase::removePtr(const FFaFieldContainer* ptr,
				     bool notifyContainer)
{
  std::list<FFaReferenceBase*>::iterator it;
  std::list<FFaReferenceBase*>::iterator tmp;
  for (it = myRefs.begin(); it != myRefs.end(); ++it)
    if (ptr == (*it)->getRef())
    {
      if (notifyContainer)
        (*it)->setPointerToNull();
      else
        (*it)->zeroOut();
      tmp = it++;
      delete *tmp;
      myRefs.erase(tmp);
      if (it == myRefs.end()) break;
    }
}


/*!
  Sets all instances of \a ptr in this list to zero. If \a notifyContainer is
  false the reference is set to zero without decrementing the FFaFieldContainer
  it points to, to say that it no longer points on it.
  This is to be used in cleaning during destruction of FFaFieldContainer.
*/

void FFaReferenceListBase::zeroOutPtr(const FFaFieldContainer* ptr,
				      bool notifyContainer)
{
  for (FFaReferenceBase* ref : myRefs)
    if (ptr == ref->getRef())
    {
      if (notifyContainer)
        ref->setPointerToNull();
      else
        ref->zeroOut();
    }
}


std::list<FFaReferenceBase*>::const_iterator
FFaReferenceListBase::findPtr(const FFaFieldContainer* ptr) const
{
  std::list<FFaReferenceBase*>::const_iterator it = myRefs.end();

  if (ptr)
    for (it = myRefs.begin(); it != myRefs.end(); ++it)
      if ((*it)->getRef() == ptr) break;

  return it;
}


std::list<FFaReferenceBase*>::iterator
FFaReferenceListBase::findPtr(const FFaFieldContainer* ptr)
{
  std::list<FFaReferenceBase*>::iterator it = myRefs.end();

  if (ptr)
    for (it = myRefs.begin(); it != myRefs.end(); ++it)
      if ((*it)->getRef() == ptr) break;

  return it;
}


/*!
  Returns true if \a ptr is present in list.
  If \a idx is not NULL, \a *idx is set to the index of \a ptr within the list.
*/

bool FFaReferenceListBase::hasPtr(const FFaFieldContainer* ptr, int* idx) const
{
  std::list<FFaReferenceBase*>::const_iterator it = this->findPtr(ptr);
  if (it == myRefs.end()) return false;

  if (idx)
  {
    std::list<FFaReferenceBase*>::const_iterator jt = myRefs.begin();
    for (*idx = 0; jt != it; ++jt, (*idx)++);
  }

  return true;
}


/*!
  Returns pointer at index \a idx. If \a idx is out of range, NULL is returned.
*/

FFaReferenceBase* FFaReferenceListBase::getRefBase(int idx) const
{
  int i = 0;
  for (FFaReferenceBase* ref : myRefs)
    if (i++ == idx) return ref;

  return NULL;
}


FFaFieldContainer* FFaReferenceListBase::getBasePtr(int idx) const
{
  FFaReferenceBase* ptr = this->getRefBase(idx);
  return ptr ? ptr->getRef() : NULL;
}


/*!
  Copy method used by copy constructors, etc.
*/

void FFaReferenceListBase::copy(const FFaReferenceListBase* other,
                                bool unresolve)
{
  this->clear();

  if (!other) return;

  for (FFaReferenceBase* ref : other->myRefs)
    if (ref)
    {
      FFaReferenceBase* newRef = this->createNewReference();
      this->setOwnerOnRef(newRef);
      myRefs.push_back(newRef);
      newRef->copy(*ref, unresolve && ref->getRefID() != -1);
    }
}


void FFaReferenceListBase::insertRefLast(FFaReferenceBase* ref)
{
  if (!ref) return;

  this->setOwnerOnRef(ref);
  myRefs.push_back(ref);
}



/*!
  Erases entry \a index from the list, decrementing its size by one.
*/

bool FFaReferenceListBase::erase(int index)
{
  if (index < 0 || index >= (int)myRefs.size()) return false;

  std::list<FFaReferenceBase*>::iterator it = myRefs.begin();
  while (--index >= 0) ++it;

  (*it)->setPointerToNull();
  delete *it;

  myRefs.erase(it);
  return true;
}


/*!
  Clears the list.
*/

void FFaReferenceListBase::clear()
{
  for (FFaReferenceBase* ref : myRefs)
  {
    ref->setPointerToNull();
    delete ref;
  }
  myRefs.clear();
}


/*!
  Fills provided vector with the non-NULL pointers in the list.
*/

void FFaReferenceListBase::getBasePtrs(std::vector<FFaFieldContainer*>& toFill) const
{
  toFill.clear();
  for (const FFaReferenceBase* ref : myRefs)
    if (!ref->isNull())
      toFill.push_back(ref->getRef());
}


/*!
  Used after reading to translate IDs to actual pointers.
*/

void FFaReferenceListBase::resolve(FFaDynCB4<FFaFieldContainer*&,int,int,
				   const std::vector<int>&>& findCB)
{
  for (FFaReferenceBase* ref : myRefs)
    ref->resolve(findCB);
}


/*!
  Makes references in list remove pointer binding to object,
  and replace it with the ID as reference instead.
*/

void FFaReferenceListBase::unresolve()
{
  for (FFaReferenceBase* ref : myRefs)
    ref->unresolve();
}


void FFaReferenceListBase::updateAssemblyRef(int from, int to, size_t ind)
{
  for (FFaReferenceBase* ref : myRefs)
    ref->updateAssemblyRef(from,to,ind);
}


void FFaReferenceListBase::updateAssemblyRef(const std::vector<int>& from,
                                             const std::vector<int>& to)
{
  for (FFaReferenceBase* ref : myRefs)
    ref->updateAssemblyRef(from,to);
}


void FFaReferenceListBase::write(std::ostream& os) const
{
  if (myRefs.empty()) return;

  std::list<FFaReferenceBase*>::const_iterator it = myRefs.begin();
  (*it)->write(os);

  while (++it != myRefs.end())
  {
    os <<" ";
    (*it)->write(os);
  }
}


void FFaReferenceListBase::read(std::istream& is)
{
  this->clear();

  char c;
  bool readAll = false;
  while (!is.eof() && !readAll)
  {
    // Skip white space
    while (is.get(c) && isspace(c));

    // Read until a non-digit character is found after last read reference
    if (isdigit(c) || c == '-' || c == 'a')
    {
      is.putback(c);
      FFaReferenceBase* ref = this->createNewReference();
      ref->read(is);
      // Do not insert zero references in autoSizing mode
      if (ref->getRefID() == 0 && IAmAutoSizing)
        delete ref;
      else
        this->insertRefLast(ref);
    }
    else
    {
      readAll = true;
      is.putback(c);
    }
  }
}


/*!
  Method used by FFaReferenceBase to notify this container
  (which the FFaReferenceBase object is a member of), that it has been zeroed
  out due to the death of the FFaFieldContainer object it was referring to.
  Takes the autoSizing mode into account (\sa setAutoSizing),
  it either does nothing, or deletes the reference.
*/

void FFaReferenceListBase::eraseReferenceIfNeeded(FFaReferenceBase* ref)
{
  if (!IAmAutoSizing) return;

  std::list<FFaReferenceBase*>::iterator it;
  for (it = myRefs.begin(); it != myRefs.end(); ++it)
    if (ref == *it)
    {
      delete *it;
      myRefs.erase(it);
      break;
    }
}
