// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

/*!
  \file FFrFieldEntryBase.H
  \brief Generic base class for non-leaf result file entries.
*/

#ifndef FFR_FIELD_ENTRY_BASE_H
#define FFR_FIELD_ENTRY_BASE_H

#include "FFrLib/FFrEntryBase.H"

class FFrCreatorData;


/*!
  \brief Generic base class for non-leaf result file entries.
  \details This class can represent any result database entry in the hierarchy,
  except for the leaf entries (which are the FFrVariableReference entries).
  It has a list of references to child entries as member, and a method
  to resolve these references from a text string read from the file header.
*/

class FFrFieldEntryBase : public FFrEntryBase
{
public:
  //! \brief The destructor deletes the heap-allocated data fields.
  virtual ~FFrFieldEntryBase();

  //! \brief Sorts the data fields based on their user ID.
  void sortDataFieldsByUserID();

  //! \brief Resolves a string of references into datafield entries.
  bool resolve(const std::string& references, FFrCreatorData& cd,
               bool inlined = true);

  virtual const FFrEntries* getDataFields() const { return &dataFields; }
  virtual bool hasDataFields() const { return !dataFields.empty(); }

  virtual bool isEmpty() const;
  virtual bool isVariableFloat() const;

  virtual void removeContainers(const std::set<FFrResultContainer*>& cont);

  virtual bool merge(FFrEntryBase* objToMergeFrom);
  virtual bool equal(const FFrEntryBase* obj) const;
  virtual bool less(const FFrEntryBase* obj) const;

  //! \brief Prints out the positioning data of this field.
  virtual void printPosition(std::ostream& os) const;

  FFrEntries dataFields; //!< List of data fields referred by this entry

protected:
  virtual int readPosData(const double* vals, int nvals, int pos) const;
  virtual int readPosData(const float* vals, int nvals, int pos) const;
  virtual int readPosData(const int* vals, int nvals, int pos) const;
};

#endif
