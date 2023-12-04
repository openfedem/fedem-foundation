// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#include <sstream>

#include "FFrLib/FFrVariable.H"
#include "FFrLib/FFrResultContainer.H"
#include "FFrLib/FFrVariableReference.H"
#include "FFaLib/FFaString/FFaTokenizer.H"

#if FFR_DEBUG > 2
long int FFrVariable::count = 0;


FFrVariable::FFrVariable() : dataType(NONE), dataSize(0), repeats(1)
{
  std::cout <<"Creating variable #"<< (myCount = ++count) << std::endl;
}

FFrVariable::~FFrVariable()
{
  std::cout <<"Destroying variable #"<< myCount <<": "<< name << std::endl;
}
#endif


FFrStatus FFrVariable::create(FILE* varStream, FFrCreatorData& cd,
                              bool dataBlocks)
{
  FFaTokenizer tokens(varStream,'<','>',';');

  // Check for reference or real variable by examining the size of the tokens.
  // References should only appear in the data blocks section.
  if (tokens.size() == 1 && dataBlocks)
  {
    // Create a variable reference pointing to variable specified in the entry
    std::map<int,FFrVariable*>::const_iterator it = cd.variables.find(atoi(tokens.front().c_str()));
    if (it == cd.variables.end())
    {
      std::cerr <<" *** Undefined variable "<< tokens.front() << std::endl;
      return FAILED;
    }

    // Found a top level variable reference
    cd.topLevelEntries.push_back(new FFrVariableReference(it->second));
    return LABEL_SEARCH;
  }

  // This is not a variable reference. We have a real variable.
  FFrVariable* variablePtr = new FFrVariable();
  int id = variablePtr->fillObject(tokens);
  if (id < 0)
  {
    delete variablePtr;
    return FAILED;
  }
  else if (id == 0 && !dataBlocks)
  {
    // Inlined variables should only appear in the data blocks section
    std::cerr <<" *** Inlined variable description in the variable section:"
              <<"\n     ";
    for (const std::string& token : tokens)
      std::cerr <<" \""<< token <<"\"";
    std::cerr << std::endl;
    delete variablePtr;
    return FAILED;
  }

  // Check in extractor if the variable is already defined
  std::pair<VariableSetIt,bool> stat = cd.extractorVariables->insert(variablePtr);
  if (id == 0)
  {
#if FFR_DEBUG > 2
    std::cout <<"Inlined variable\n";
    variablePtr->dump(std::cout);
#endif
    return LABEL_SEARCH; // inlined variable
  }

  if (stat.second)
  {
    cd.variables[id] = variablePtr;
#if FFR_DEBUG > 2
    std::cout <<"ID             " << id <<"\n";
    variablePtr->dump(std::cout);
#endif
  }
  else // this variable is already in the extractor
  {
    delete variablePtr;
    cd.variables[id] = variablePtr = *stat.first;
  }

  // Not likely, but if a variable is defined in the data blocks section
  // it also has to be added as a top-level entry, unless it is inlined
  if (dataBlocks)
    cd.topLevelEntries.push_back(new FFrVariableReference(variablePtr));

  return LABEL_SEARCH;
}


int FFrVariable::fillObject(const std::vector<std::string>& tokens)
{
  size_t fieldCount = tokens.size();
  if (fieldCount < 6)
  {
    std::cerr <<" *** Fewer than 6 fields in variable description:"
              <<"\n     ";
    for (const std::string& token : tokens)
      std::cerr <<" \""<< token <<"\"";
    std::cerr << std::endl;
    return -1;
  }

  this->name = tokens[1];
  this->unit = tokens[2];
#ifdef linux64
  std::stringstream tmp(tokens[3]);
  tmp >> this->dataType;
#else
  std::stringstream(tokens[3]) >> this->dataType;
#endif
  std::stringstream(tokens[4]) >> this->dataSize;
  this->dataClass = tokens[5];

  if (fieldCount > 6)
  {
    repeats = 1;
    FFaTokenizer sizesTokens(tokens[6],'(',')');
    for (const std::string& token : sizesTokens)
    {
      int descrSize = atoi(token.c_str());
      this->dataBlockSizes.push_back(descrSize);
      repeats *= descrSize;
    }
  }

  if (fieldCount > 7)
  {
    FFaTokenizer descrTokens(tokens[7],'(',')');
    for (const std::string& token : descrTokens)
      this->dataBlockDescription.push_back(token);
  }

  return atoi(tokens.front().c_str());
}


void FFrVariable::dump(std::ostream& os) const
{
  os <<"Name           " << name
     <<"\nUnit           " << unit
     <<"\nData type      " << dataType
     <<"\nData type size " << dataSize
     <<"\nData class     " << dataClass;
  if (!dataBlockSizes.empty())
  {
    os <<"\nData block sizes: [ ";
    for (size_t bsize : dataBlockSizes)
      os << bsize <<' ';
    os <<']';
  }
  if (!dataBlockDescription.empty())
  {
    os <<"\n[ ";
    for (const std::string& bdescr : dataBlockDescription)
      os << bdescr <<' ';
    os <<']';
  }
  os << std::endl;
}


bool FFrVariable::equal(const FFrVariable* that) const
{
#define COMPARE_VARIABLE(VAR) if (this->VAR != that->VAR) return false
  COMPARE_VARIABLE(name);
  COMPARE_VARIABLE(unit);
  COMPARE_VARIABLE(dataType);
  COMPARE_VARIABLE(dataSize);
  COMPARE_VARIABLE(dataClass);
  COMPARE_VARIABLE(dataBlockSizes);
  COMPARE_VARIABLE(dataBlockDescription);
  return true;
#undef COMPARE_VARIABLE
}


bool FFrVariable::less(const FFrVariable* that) const
{
#define COMPARE_VARIABLE(VAR) if (this->VAR != that->VAR) return (this->VAR < that->VAR)
  COMPARE_VARIABLE(name);
  COMPARE_VARIABLE(unit);
  COMPARE_VARIABLE(dataType);
  COMPARE_VARIABLE(dataSize);
  COMPARE_VARIABLE(dataClass);
  COMPARE_VARIABLE(dataBlockSizes);
  COMPARE_VARIABLE(dataBlockDescription);
  return false;
#undef COMPARE_VARIABLE
}
