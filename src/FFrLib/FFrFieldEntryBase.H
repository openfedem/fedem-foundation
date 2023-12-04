// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#ifndef FFR_FIELD_ENTRY_BASE_H
#define FFR_FIELD_ENTRY_BASE_H

#include "FFrLib/FFrEntryBase.H"

class FFrCreatorData;


class FFrFieldEntryBase : public FFrEntryBase
{
public:
  FFrFieldEntryBase() {}
  virtual ~FFrFieldEntryBase();

  void sortDataFieldsByUserID();

  bool resolve(const std::string& references, FFrCreatorData& cd, bool inlined = true);

  virtual const std::vector<FFrEntryBase*>* getDataFields() const { return &dataFields; }
  virtual bool hasDataFields() const { return !dataFields.empty(); }

  virtual bool isEmpty() const;
  virtual bool isVariableFloat() const;

  virtual void removeContainers(const std::set<FFrResultContainer*>& cont);

  virtual bool merge(FFrEntryBase* objToMergeFrom);
  virtual bool equal(const FFrEntryBase* obj) const;
  virtual bool less(const FFrEntryBase* obj) const;

  //! Prints out the positioning data of this field.
  virtual void printPosition(std::ostream& os) const;

  std::vector<FFrEntryBase*> dataFields;

protected:
  virtual int recursiveReadPosData(const double* vals, int nvals, int arrayPos) const;
  virtual int recursiveReadPosData(const float* vals, int nvals, int arrayPos) const;
  virtual int recursiveReadPosData(const int* vals, int nvals, int arrayPos) const;
};

#endif
