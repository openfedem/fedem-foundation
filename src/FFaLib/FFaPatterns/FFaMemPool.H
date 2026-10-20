// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

/*!
  \file FFaMemPool.H
  \brief Streamlined efficient memory allocation.
*/

#ifndef FFA_MEM_POOL_H
#define FFA_MEM_POOL_H

#include <cstddef>
#include <vector>
#include <list>
#include <map>

class FFaMemPoolMgr;


/*!
  \brief Class to streamline efficient memory allocation.

  \details
  This class is used to help the programmer to reimplement the operators
  new and delete for small classes that will be instantiated a lot of times.
  To use, simply make a static member of FFaMemPool in your class, reimplement
  the operators new and delete in that class by forwarding the calls to
  FFaMemPool::doNew() and FFaMemPool::doDelete(), respectively:

  \code
  #include "FFaLib/FFaPatterns/FFaMemPool.H"

  class foo
  {
  public:
    static void* operator new(size_t n) { return ourMemPool.doNew(n); }
    static void operator delete(void* p, size_t n) { ourMemPool.doDelete(p,n); }
    static void freePool() { ourMemPool.freePool(); }

  private:
    static FFaMemPool ourMemPool;
  };
  \endcode

  In the implementation file :

  \code
  FFaMemPool foo::ourMemPool(sizeof(foo));
  \endcode
  \note The private members of this class are not documented.

  \author Jacob Storen
*/

class FFaMemPool
{
public:
  //! \brief The constructor intializes the data members.
  explicit FFaMemPool(size_t objSize, FFaMemPoolMgr* mgr = NULL,
                      int blockSize = 512);
  //! \brief Copy constructor.
  FFaMemPool(const FFaMemPool& mempool);
  //! \brief The destructor releases all heap-allocated memory.
  ~FFaMemPool();

  //! \brief Allocates a chunk of memory of size \a objSize.
  void* doNew(size_t objSize);
  //! \brief Deallocates a memory chunk.
  void  doDelete(void* p, size_t objSize);

  //! \brief Releases all heap-allocated memory in this pool.
  void  freePool(bool release = false);
  //! \brief Releases heap-allocated memory associated with specified object.
  void  freePartOfPool(void* objPtrAsId);

  //! \brief Finds or creates a memory pool for specified object.
  void  usePartOfPool(void* objPtrAsId);
  //! \brief Resets the current memory pool the default instance.
  void  useDefaultPartOfPool() { myCurrentPP = &myDefaultPP; }

private:
  //! \cond MEMORY_POOL_DETAILS
  struct PoolPart
  {
    PoolPart() { myHeadOfFreeList = NULL; }
    ~PoolPart() { this->freePoolPart(); }

    void freePoolPart();

    void* myHeadOfFreeList;
    std::vector<void*> myMemBlocks;

  private:
    PoolPart(const PoolPart&) = delete;
    PoolPart& operator=(const PoolPart&) = delete;
  };

  PoolPart* myCurrentPP;
  PoolPart  myDefaultPP;

  std::map<void*,PoolPart*> myPoolParts;

  int    myBlockSize;
  size_t myObjSize;

  FFaMemPoolMgr* myManager;

  FFaMemPool& operator=(const FFaMemPool&) { return *this; }
  //! \endcond
};


/*!
  \brief Memory pool manager.
*/

class FFaMemPoolMgr
{
public:
  //! \brief The destructor frees all memory pools.
  ~FFaMemPoolMgr() { this->freeMemPools(true); }
  //! \brief Frees all memory pools.
  void freeMemPools(bool release = false);
  //! \brief Removes a memory pool from #myMemPools.
  void remove(FFaMemPool* p) { myMemPools.remove(p); }
  //! \brief Adds a memory pool to #myMemPools.
  void insert(FFaMemPool* p) { if (p) myMemPools.push_front(p); }

private:
  std::list<FFaMemPool*> myMemPools; //!< Memory pool container.
};


//! \brief Macro adding memory pool features to a class.
#define FFA_MAKE_MEMPOOL public: \
  static void* operator new(size_t s) { return ourMemPool.doNew(s); } \
  static void operator delete(void* p, size_t s) { ourMemPool.doDelete(p,s); } \
  static void freePool()              { ourMemPool.freePool(); } \
  static void freePartOfPool(void* p) { ourMemPool.freePartOfPool(p); } \
  static void usePartOfPool(void* p)  { ourMemPool.usePartOfPool(p); } \
  static void useDefaultPartOfPool()  { ourMemPool.useDefaultPartOfPool();} \
private: \
  static FFaMemPool ourMemPool

#endif
