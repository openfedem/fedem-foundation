// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

/*!
  \file FFrVariable.H
  \brief Class representing a physical variable in the result file hierarchy.
*/

#ifndef FFR_VARIABLE_H
#define FFR_VARIABLE_H

#include "FFrLib/FFrEnums.H"
#include "FFaLib/FFaString/FFaEnum.H"
#include <string>
#include <vector>
#include <set>
#include <cstdio>

class FFrCreatorData;


/*!
  \brief Class representing a physical variable in the result file hierarchy.
*/

class FFrVariable
{
public:
  //! \brief Enum defining available the variable types.
  enum VDType {
    NONE,  //!< None - undefined
    CHAR,  //!< Signed character
    UCHAR, //!< Unsigned character
    INT,   //!< Signed integer
    UINT,  //!< Unsigned integer
    FLOAT  //!< Real, either single or double precision (float or double)
  };

private:
  //! \cond DO_NOT_DOCUMENT
  FFaEnumMapping(VDType) {
    FFaEnumEntry(NONE,  "NONE");
    FFaEnumEntry(CHAR,  "CHAR");
    FFaEnumEntry(UCHAR, "UCHAR");
    FFaEnumEntry(INT,   "INT");
    FFaEnumEntry(UINT,  "UINT");
    FFaEnumEntry(FLOAT, "FLOAT");
    FFaEnumEntryEnd;
  };
  //! \endcond

public:
#if FFR_DEBUG > 2
  //! \brief Default constructor with some debug print.
  FFrVariable();
  //! \brief Destructor with some debug print.
  ~FFrVariable();
#else
  //! \brief Default constructor initializing the members.
  FFrVariable() : dataType(NONE), dataSize(0), repeats(1) {}
#endif

  //! \brief Static method for parsing a variable definition from file.
  static FFrStatus create(FILE* varStream, FFrCreatorData& cd, bool dataBlocks);

  //! \brief Parses the variable definition from a list of strings.
  int fillObject(const std::vector<std::string>& tokens);

  //! \brief Prints the variable defintion to the sepcified output stream.
  void dump(std::ostream& os = std::cout) const;

  //! \brief Returns the total number of bits in the variable.
  size_t getTotalDataSize() const { return dataSize*repeats; }
  //! \brief Returns the total number of words in the variable.
  size_t getRepeats() const { return repeats; }

  //! \brief Return \e true if \a *this equals \a *that.
  bool equal(const FFrVariable* that) const;
  //! \brief Return \e true if \a *this is less than \a *that.
  bool less(const FFrVariable* that) const;

  //! \brief Functor for comparison of two variable definitions.
  struct Less
  {
    //! \brief Returns \e true if \a *a is considered less than \a *b.
    bool operator()(const FFrVariable* a, const FFrVariable* b) const
    {
      return a->less(b);
    }
  };

#if FFR_DEBUG > 2
private:
  static long int count; //!< Global counter for all FFrVariable instances
  long int myCount;      //!< Running variable ID for this instancew
#endif

public:
  std::string name;      //!< Name or desription of the variable
  std::string unit;      //!< Unit of the variable (only for FLOAT variables)
  VDTypeEnum  dataType;  //!< Basic data type of the variable
  std::string dataClass; //!< Class name (structured type) of the variable
  size_t      dataSize;  //!< Size (in bits) of each word of the variable
  size_t      repeats;   //!< Total number of words in the variable

  //! Number of data words in each direction for multi-dimensional variables
  std::vector<size_t>	   dataBlockSizes;
  //! Component names for multi-dimensional variables
  std::vector<std::string> dataBlockDescription;
};


//! Data type alias for a sorted set of variables
using VariableSet   = std::set<FFrVariable*,FFrVariable::Less>;
//! Iterator over variable container
using VariableSetIt = VariableSet::const_iterator;

#endif
