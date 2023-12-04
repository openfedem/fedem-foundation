// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#ifndef FFR_VARIABLE_REFERENCE_H
#define FFR_VARIABLE_REFERENCE_H

#include "FFrLib/FFrEntryBase.H"

class FFaOperationBase;
class FFrVariable;


class FFrVariableReference : public FFrEntryBase
{
public:
#if FFR_DEBUG > 2
  FFrVariableReference(FFrVariable* var);
  FFrVariableReference(const FFrVariableReference& varRef);
  virtual ~FFrVariableReference();
#else
  FFrVariableReference(FFrVariable* var) : variableDescr(var) {}
  virtual ~FFrVariableReference() {}
#endif

  virtual const std::string& getDescription() const;
  virtual const std::string& getType() const;

  virtual bool isVariableFloat() const;
  virtual bool isVarRef() const { return true; }
  virtual bool isEmpty() const { return containers.empty(); }

  FFaOperationBase* getReadOperation();

  //! \brief Returns the (most recent) time stamp of this variable.
  unsigned int getTimeStamp() const;

  // If usePositionedKey is true, the current physical position of the
  // owning extractor object is used in the following two functions.
  // If usePositionedKey is false, the requested position is used.
  double getDistanceFromResultPoint(const bool usePositionedKey = false) const;
  bool hasDataForCurrentKey(const bool usePositionedKey = false) const;

  void getValidKeys(std::set<double>& validValues) const;

  //! \brief Prints out the positioning data of this variable.
  virtual void printPosition(std::ostream& os) const;

  union {
    FFrVariable* variableDescr;
    FFrVariableReference* next; // used in memory management
  };

  typedef std::pair<FFrResultContainer*,size_t> FFrResultContainerRef;

  std::vector<FFrResultContainerRef> containers;

  virtual void removeContainers(const std::set<FFrResultContainer*>& cont);

  virtual int traverse(FFrResultContainer* resultContainer,
		       FFrEntryBase* owner,
		       FFrEntryBase*& objToBeModified,
		       int binPos);

  virtual bool merge(FFrEntryBase* obj);
  virtual bool compare(const FFrEntryBase* obj) const { return this->equal(obj); }
  virtual bool equal(const FFrEntryBase* obj) const;
  virtual bool less(const FFrEntryBase* obj) const;

protected:
  virtual int recursiveReadPosData(const double* vals, int nvals, int arrayPos) const;
  virtual int recursiveReadPosData(const float* vals, int nvals, int arrayPos) const;
  virtual int recursiveReadPosData(const int* vals, int nvals, int arrayPos) const;

private:
  //! \brief Returns the index of the nearest container for the current setting.
  //! \details Returns -1 if no containers are found.
  int getNearestContainer() const;

#ifdef FFR_NEWALLOC
public:
  // overloading the new & delete operators
  static void* operator new(size_t size);
  static void operator delete(void* deadObject, size_t size);

  static void releaseMemBlocks();

private:
  static size_t newed;
  static FFrVariableReference* headOfFreeList;
  static std::vector<FFrVariableReference*> memBlocks;
#endif
#if FFR_DEBUG > 2
  static long int count;
#endif
};

#endif
