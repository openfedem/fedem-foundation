// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

/*!
  \file FFrVariableReference.H
  \brief Class representing a variable reference in a result file hierarchy.
*/

#ifndef FFR_VARIABLE_REFERENCE_H
#define FFR_VARIABLE_REFERENCE_H

#include "FFrLib/FFrEntryBase.H"

class FFaOperationBase;
class FFrVariable;


/*!
  \brief Class representing a variable reference in a result file hierarchy.
  \details This class does not represent the variable itself, but a reference
  to one through a pointer. There may be many references pointing to the same
  physical variable, which are represented by the FFrVariable class.
*/

class FFrVariableReference : public FFrEntryBase
{
public:
#if FFR_DEBUG > 2
  //! \brief Constructor with some debug messages.
  explicit FFrVariableReference(FFrVariable* var);
  //! \brief Copy constructor with some debug messages.
  explicit FFrVariableReference(const FFrVariableReference& varRef);
  //! \brief Destructor with some debug messages.
  virtual ~FFrVariableReference();
#else
  //! \brief The constructor initializes \ref variableDescr.
  explicit FFrVariableReference(FFrVariable* var) : variableDescr(var) {}
#endif

  //! \brief Returns the variable name.
  virtual const std::string& getDescription() const;
  //! \brief Returns the variable type, e.g., SCALAR, VEC3, etc.
  virtual const std::string& getType() const;

  //! \brief Returns \e true if this variable contain float data.
  virtual bool isVariableFloat() const;
  //! \brief Returns \e true since this entry is a Variable Reference.
  virtual bool isVarRef() const { return true; }
  //! \brief Returns \e true if this variable has no result data.
  virtual bool isEmpty() const { return containers.empty(); }

  //! \brief Creates an operation reading the data of the variable from file.
  FFaOperationBase* getReadOperation();

  //! \brief Returns the (most recent) time stamp of this variable.
  unsigned int getTimeStamp() const;

  //! \brief Returns the smallest distance to current extraxtor position.
  double getDistanceFromResultPoint(const bool usePositionedKey = false) const;
  //! \brief Checks if this entry has any data at current extractor position.
  bool hasDataForCurrentKey(const bool usePositionedKey = false) const;

  //! \brief Extracts a list of time steps for which this entry has data.
  void getValidKeys(std::set<double>& validValues) const;

  //! \brief Prints out the positioning data of this variable.
  virtual void printPosition(std::ostream& os) const;

  //! \brief Shared data memory segment for \ref variableDescr and \ref next.
  union {
    FFrVariable* variableDescr; //!< Points to the actual variable with data
    FFrVariableReference* next; //!< Used in memory management
  };

  //! Convenience type alias
  using FFrResultContainerRef = std::pair<FFrResultContainer*,size_t>;
  //! List of result file containers in which this variable exists
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

public:
  //! \brief Reads a variable from the results container for current time step.
  template<class T> bool readVariable(T& value);

private:
  //! \brief Returns the index of the nearest container for the current setting.
  //! \details Returns -1 if no containers are found.
  int getNearestContainer() const;

#ifdef FFR_NEWALLOC
  //! \cond MEMORY_MANAGEMENT
public:
  // overloading the new & delete operators
  static void* operator new(size_t size);
  static void operator delete(void* deadObject, size_t size);

  static void releaseMemBlocks();

private:
  static size_t newed;
  static FFrVariableReference* headOfFreeList;
  static std::vector<FFrVariableReference*> memBlocks;
  //! \endcond
#endif

#if FFR_DEBUG > 2
  static long int count; //!< Internal counter, for debug usage
#endif
};

#endif
