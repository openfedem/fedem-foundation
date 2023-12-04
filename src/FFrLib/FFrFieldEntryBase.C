// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#include "FFrLib/FFrFieldEntryBase.H"
#include "FFrLib/FFrItemGroup.H"
#include "FFrLib/FFrVariable.H"
#include "FFrLib/FFrVariableReference.H"
#include "FFrLib/FFrResultContainer.H"
#include "FFaLib/FFaString/FFaTokenizer.H"
#include <algorithm>


FFrFieldEntryBase::~FFrFieldEntryBase()
{
  for (FFrEntryBase* field : dataFields)
    if (field && field->getOwner() == this)
      delete field;
}


bool FFrFieldEntryBase::isEmpty() const
{
  for (FFrEntryBase* field : dataFields)
    if (!field->isEmpty())
      return false;

  return true;
}


bool FFrFieldEntryBase::isVariableFloat() const
{
  // Assuming here that all fields are of the same type
  if (!dataFields.empty())
    return dataFields.front()->isVariableFloat();

  return false;
}


/*!
  Resolves the references string (the last string in object- and item groups)
  to variable references or item group references.
*/

bool FFrFieldEntryBase::resolve(const std::string& references,
				FFrCreatorData& cd, bool inlined)
{
  bool retvar = true;
  for (std::string::const_iterator it = references.begin(); it != references.end();)
    if (*it == '<')
    {
      FFaTokenizer tokens('<','>',';');
      it = tokens.createTokens(it,references.end());
      if (tokens.size() == 1) // we have a reference
      {
	// Create a reference pointing to variable specified in the entry
	int vID = atoi(tokens.front().c_str());
	std::map<int,FFrVariable*>::const_iterator vit = cd.variables.find(vID);
	if (vit == cd.variables.end())
	{
	  std::cerr <<"FFrFieldEntryBase::resolve: Variable reference "<< vID
		    <<" was not found."<< std::endl;
	  retvar = false;
	}
	else
	{
	  dataFields.push_back(new FFrVariableReference(vit->second));
	  dataFields.back()->setOwner(this);
	}
      }
      else if (tokens.size() > 5)
      {
	// We have an inlined variable definition.
	// Create a new variable and insert it into the local variable vector
	FFrVariable* var = new FFrVariable();
	var->fillObject(tokens);
	std::pair<VariableSetIt,bool> stat = cd.extractorVariables->insert(var);
	if (!stat.second)
	{
	  // This variable is already defined, use existing instance
	  delete var;
	  var = *stat.first;
	}

	// Create a reference to the newly created variable
	// put this reference into the dataField vector of the object.
	dataFields.push_back(new FFrVariableReference(var));
	dataFields.back()->setOwner(this);
      }
    }
    else if (*it == '[')
    {
      FFaTokenizer tokens('[',']',';');
      it = tokens.createTokens(it,references.end());
      if (tokens.size() == 1) // we have a reference
      {
	// Adds a pointer to the referred item group
	int iID = atoi(tokens.front().c_str());
	std::map<int,FFrItemGroup*>::const_iterator iit = cd.itemGroups.find(iID);
	if (iit == cd.itemGroups.end() || !iit->second)
	{
	  std::cerr <<"FFrFieldEntryBase::resolve: Item group reference "<< iID
		    <<" was not found."<< std::endl;
	  retvar = false;
	}
	else
	  dataFields.push_back(iit->second);
      }
      else if (tokens.size() == 3)
      {
	// We have an inline item group. It is tokenized and resolved as usual.
	// This item group is not available to anyone else than this object.
	FFrItemGroup* igrp = new FFrItemGroup(inlined);
	if (igrp->fillObject(tokens,cd) >= 0)
	{
	  dataFields.push_back(igrp);
	  igrp->setOwner(this);
	}
	else
	  delete igrp;
      }
    }
    else
      ++it;

  return retvar;
}


void FFrFieldEntryBase::removeContainers(const std::set<FFrResultContainer*>& cont)
{
  // Check all children, remove references if empty
  std::vector<FFrEntryBase*> tmp;
  tmp.reserve(dataFields.size());
  for (FFrEntryBase* field : dataFields)
  {
    field->removeContainers(cont);
    if (field->isEmpty() && field->getOwner() == this)
      delete field;
    else
      tmp.push_back(field);
  }

  if (tmp.size() < dataFields.size())
    dataFields.swap(tmp);
}


void FFrFieldEntryBase::sortDataFieldsByUserID()
{
  std::sort(dataFields.begin(), dataFields.end(),
            [](const FFrEntryBase* first, const FFrEntryBase* sec)
            {
              if (first && sec)
                return first->getUserID() < sec->getUserID();
              else
                return !first;
            });
}


bool FFrFieldEntryBase::merge(FFrEntryBase* objToMergeFrom)
{
  FFrFieldEntryBase* field = dynamic_cast<FFrFieldEntryBase*>(objToMergeFrom);
  if (!field) return false;

#if FFR_DEBUG > 2
  std::cout <<"Merging "<< field->getType() <<" #"<< field->myCount <<" "<< field->getDescription()
            <<" into "<< this->getType() <<" #"<< myCount <<" "<< this->getDescription() << std::endl;
#endif

  // Optimized merge if I am item group with item group datafields
  // with numeric identifier, i.e., nodes, elements, elementnodes
  // datafields MUST be sorted by userID (=IG::myID)
  bool optMergeMyFields = (this->isIG() && !this->dataFields.empty() &&
			   this->dataFields.front()->isIG() &&
			   this->dataFields.front()->hasUserID());

  std::vector<FFrEntryBase*> added;
  std::vector<FFrEntryBase*>::iterator thisFieldIt = this->dataFields.begin();
  std::vector<FFrEntryBase*>::iterator mergeFieldsIt;

  // Run over all fields in the object which we are merging from
  for (mergeFieldsIt = field->dataFields.begin();
       mergeFieldsIt != field->dataFields.end(); ++mergeFieldsIt)
   {
    if (!optMergeMyFields)
      thisFieldIt = this->dataFields.begin();

    // Check if this field is in our container
    bool found = false;
    for (; thisFieldIt != this->dataFields.end(); ++thisFieldIt)
      if ((found = (*thisFieldIt)->compare(*mergeFieldsIt)))
        break;
      else if (optMergeMyFields && (*thisFieldIt)->getUserID() > (*mergeFieldsIt)->getUserID())
        break;

    if (found) {
      if ((*thisFieldIt)->merge(*mergeFieldsIt)) {
	delete *mergeFieldsIt;
	*mergeFieldsIt = NULL;
      }
    }
    else {
      // Not found - adding to the dataFields
      if (added.empty())
	added.reserve(field->dataFields.size());
      added.push_back(*mergeFieldsIt);
      (*mergeFieldsIt)->setOwner(this);
    }
  }

  if (!added.empty())
  {
    this->dataFields.insert(this->dataFields.end(),added.begin(),added.end());
    if (optMergeMyFields) this->sortDataFieldsByUserID();
  }

  return true;
}


bool FFrFieldEntryBase::equal(const FFrEntryBase* obj) const
{
  const FFrFieldEntryBase* that = dynamic_cast<const FFrFieldEntryBase*>(obj);
  if (!that) return false;

  if (this->dataFields.size() != that->dataFields.size())
    return false;

  for (size_t i = 0; i < this->dataFields.size(); i++)
    if (!this->dataFields[i]->equal(that->dataFields[i]))
      return false;

  return true;
}


bool FFrFieldEntryBase::less(const FFrEntryBase* obj) const
{
  const FFrFieldEntryBase* that = dynamic_cast<const FFrFieldEntryBase*>(obj);
  if (!that) return false;

  for (size_t i = 0; i < this->dataFields.size(); i++)
    if (i == that->dataFields.size())
      return true; // this has fewer fields than that
    else if (!this->dataFields[i]->equal(that->dataFields[i]))
      return this->dataFields[i]->less(that->dataFields[i]);

  return false;
}


int FFrFieldEntryBase::recursiveReadPosData(const double* vals, int nvals, int arrayPos) const
{
  int valsRead = arrayPos;
  for (FFrEntryBase* field : dataFields)
    valsRead = field->recursiveReadPosData(vals, nvals, valsRead);

  return valsRead;
}

int FFrFieldEntryBase::recursiveReadPosData(const float* vals, int nvals, int arrayPos) const
{
  int valsRead = arrayPos;
  for (FFrEntryBase* field : dataFields)
    valsRead = field->recursiveReadPosData(vals, nvals, valsRead);

  return valsRead;
}

int FFrFieldEntryBase::recursiveReadPosData(const int* vals, int nvals, int arrayPos) const
{
  int valsRead = arrayPos;
  for (FFrEntryBase* field : dataFields)
    valsRead = field->recursiveReadPosData(vals, nvals, valsRead);

  return valsRead;
}


void FFrFieldEntryBase::printPosition(std::ostream& os) const
{
  if (!dataFields.empty())
    dataFields.front()->printPosition(os);
}
