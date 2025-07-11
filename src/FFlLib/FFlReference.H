// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#ifndef FFL_REFERENCE_H
#define FFL_REFERENCE_H

#include <map>
#include <cstdint>

#include "FFlLib/FFlPartBase.H"

#ifdef FF_NAMESPACE
namespace FF_NAMESPACE {
#endif


template <class T> class FFlReference;
template <class T> bool operator< (const FFlReference<T>& lhs,
                                   const FFlReference<T>& rhs);

template <class T>
class FFlReference
{
public:
  FFlReference(T* ptr = NULL) : iAmResolved(true)
  {
    myResolvedRef = ptr;
    if (ptr) ptr->ref();
  }

  FFlReference(int ID) : iAmResolved(false) { myID = ID; }

  FFlReference(const FFlReference& ref) : iAmResolved(ref.iAmResolved)
  {
    if (iAmResolved)
    {
      myResolvedRef = ref.myResolvedRef;
      if (myResolvedRef) myResolvedRef->ref();
    }
    else
      myID = ref.myID;
  }

  ~FFlReference()
  {
    if (iAmResolved && myResolvedRef)
      myResolvedRef->unref();
  }

  FFlReference& operator=(const FFlReference& ref)
  {
    if (this != &ref)
    {
      if (!iAmResolved)
        myID = ref.getID();
      else if (ref.iAmResolved)
      {
        if (myResolvedRef) myResolvedRef->unref();
        myResolvedRef = ref.myResolvedRef;
        if (myResolvedRef) myResolvedRef->ref();
      }
      else
      {
        if (myResolvedRef) myResolvedRef->unref();
        myID = ref.myID;
        iAmResolved = false;
      }
    }

    return *this;
  }


  FFlReference& operator=(const T* objPtr) // fast resolving manually
  {
    if (myResolvedRef) myResolvedRef->unref();
    myResolvedRef = const_cast<T*>(objPtr);
    if (myResolvedRef) myResolvedRef->ref();
    iAmResolved = true;

    return *this;
  }

  FFlReference& operator=(const int& objID) // fast assignment manualy
  {
    myID = objID;
    iAmResolved = false;

    return *this;
  }


  bool isResolved() const { return iAmResolved && myResolvedRef != NULL; }


  bool resolve(const std::map<int,T*>& possibleRefs)
  {
    if (iAmResolved) return true;

    using RefIt = typename std::map<int,T*>::const_iterator;

    RefIt r = possibleRefs.find(static_cast<int>(myID));
    if (r != possibleRefs.end())
    {
      myResolvedRef = r->second;
      myResolvedRef->ref();
      iAmResolved = true;
    }

    return iAmResolved;
  }

  bool resolve(const std::vector<T*>& possibleRefs)
  {
    if (iAmResolved) return true;

    using RefIt = typename std::vector<T*>::const_iterator;

    RefIt r = FFlPartFinder<T>::findObject(possibleRefs,static_cast<int>(myID));
    if (r != possibleRefs.end())
    {
      myResolvedRef = *r;
      myResolvedRef->ref();
      iAmResolved = true;
    }

    return iAmResolved;
  }


  void unresolve()
  {
    if (!iAmResolved) return;

    if (myResolvedRef) myResolvedRef->unref();
    int tmpID = myResolvedRef ? myResolvedRef->getID() : 0;
    myID = tmpID;

    iAmResolved = false;
  }


  int getID() const
  {
    if (iAmResolved && myResolvedRef)
      return myResolvedRef->getID();

    return static_cast<int>(myID);
  }


  T* getReference() const
  {
    return iAmResolved ? myResolvedRef : NULL;
  }

  T* operator->() const
  {
    if (!iAmResolved || !myResolvedRef)
      abort(); // force a core dump (internal logic error)

    return myResolvedRef;
  }

private:
  bool iAmResolved;

  union {
    T* myResolvedRef;
    std::intptr_t myID;
  };
};


template <class T>
bool operator< (const FFlReference<T>& lhs, const FFlReference<T>& rhs)
{
  return lhs.getID() < rhs.getID();
}

#ifdef FF_NAMESPACE
} // namespace
#endif
#endif
