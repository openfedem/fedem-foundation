// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#include "FFrLib/FFrObjectGroup.H"
#include "FFrLib/FFrResultContainer.H"
#include "FFaLib/FFaString/FFaTokenizer.H"

#if FFR_DEBUG > 2
long int FFrObjectGroup::count = 0;
#endif


FFrObjectGroup::FFrObjectGroup()
{
  static std::set<std::string> undefined;
  if (undefined.empty())
    undefined.insert("(undefined)");

  id = baseId = 0;
  typeIt = undefined.begin();

#if FFR_DEBUG > 2
  std::cout <<"Creating object group #"<< (myCount = ++count) << std::endl;
#endif
}

FFrObjectGroup::~FFrObjectGroup()
{
#if FFR_DEBUG > 2
  std::cout <<"Destroying object group #"<< myCount <<" "<< this->getType()
	    <<" "<< id <<" "<< this->getDescription() << std::endl;
#endif
}


FFrStatus FFrObjectGroup::create(FILE* varStream, FFrCreatorData& cd,
                                 bool dataBlocks)
{
  FFaTokenizer tokens(varStream,'{','}',';');
  if (!dataBlocks)
  {
    std::cerr <<" *** Detected an object group in the variable section\n    ";
    for (const std::string& field : tokens) std::cerr <<" \""<< field <<"\"";
    std::cerr <<" (ignored)."<< std::endl;
    return FAILED;
  }

  FFrObjectGroup* obj = new FFrObjectGroup();
  if (obj->fillObject(tokens,cd) < 0)
  {
    delete obj;
    return FAILED;
  }

  cd.topLevelEntries.push_back(obj);
  return LABEL_SEARCH;
}


/*!
  Parses one variable entry.
*/

int FFrObjectGroup::fillObject(const std::vector<std::string>& tokens,
			       FFrCreatorData& cd)
{
  if (tokens.size() < 5)
  {
    std::cerr <<" *** Fewer than 5 fields in object group description:"
              <<"\n     ";
    for (const std::string& token : tokens)
      std::cerr <<" \""<< token <<"\"";
    std::cerr << std::endl;
    return -1;
  }

  this->typeIt      = cd.dict->insert(tokens[0]).first;
  this->baseId      = atoi(tokens[1].c_str());
  this->id          = atoi(tokens[2].c_str());
  this->description = tokens[3];
#if FFR_DEBUG > 2
  std::cout <<"Resolving object group #"<< myCount <<": "<< *typeIt
            <<" {"<< baseId <<"} ["<< id <<"] "<< description << std::endl;
#endif

  // Resolve the references and return zero if everything OK
  return this->resolve(tokens[4],cd) ? 0 : -2;
}


int FFrObjectGroup::traverse(FFrResultContainer* resultCont, FFrEntryBase*,
			     FFrEntryBase*& objToBeMod, int binPos)
{
  FFrObjectGroup* ogrp = static_cast<FFrObjectGroup*>(objToBeMod);

  // Note: Can not use range-based loop here, since the container will expand
  std::vector<FFrEntryBase*>::iterator it;
  for (it = ogrp->dataFields.begin(); it != ogrp->dataFields.end(); ++it)
  {
    if (!(*it)->isGlobal()) resultCont->collectGarbage(*it);
    binPos = (*it)->traverse(resultCont, this, *it, binPos);
  }

  return binPos;
}
