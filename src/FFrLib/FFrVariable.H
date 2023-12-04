// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#ifndef FFR_VARIABLE_H
#define FFR_VARIABLE_H

#include "FFrLib/FFrEnums.H"
#include "FFaLib/FFaString/FFaEnum.H"
#include <string>
#include <vector>
#include <set>
#include <cstdio>

class FFrCreatorData;


class FFrVariable
{
public:
  enum VDType { NONE, CHAR, UCHAR, INT, UINT, FLOAT };

private:
  FFaEnumMapping(VDType) {
    FFaEnumEntry(NONE,  "NONE");
    FFaEnumEntry(CHAR,  "CHAR");
    FFaEnumEntry(UCHAR, "UCHAR");
    FFaEnumEntry(INT,   "INT");
    FFaEnumEntry(UINT,  "UINT");
    FFaEnumEntry(FLOAT, "FLOAT");
    FFaEnumEntryEnd;
  };

public:
#if FFR_DEBUG > 2
  FFrVariable();
  ~FFrVariable();
#else
  FFrVariable() : dataType(NONE), dataSize(0), repeats(1) {}
#endif

  static FFrStatus create(FILE* varStream, FFrCreatorData& cd, bool dataBlocks);

  int fillObject(const std::vector<std::string>& tokens);

  void dump(std::ostream& os = std::cout) const;

  size_t getTotalDataSize() const { return dataSize*repeats; }
  size_t getRepeats() const { return repeats; }

  bool equal(const FFrVariable* that) const;
  bool less(const FFrVariable* that) const;

  struct Less
  {
    bool operator()(const FFrVariable* a, const FFrVariable* b) const { return a->less(b); }
  };

#if FFR_DEBUG > 2
private:
  static long int count;
  long int myCount;
#endif

public:
  std::string name;
  std::string unit;       // fix this to iterator in a library of units
  VDTypeEnum  dataType;
  std::string dataClass;  // fix this to iterator in a library of classes
  size_t      dataSize;
  size_t      repeats;
  std::vector<size_t>	   dataBlockSizes;
  std::vector<std::string> dataBlockDescription;
};

typedef std::set<FFrVariable*,FFrVariable::Less> VariableSet;
typedef VariableSet::const_iterator              VariableSetIt;

#endif
