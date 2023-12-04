// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#include "FFrLib/FFrItemGroup.H"
#include "FFrLib/FFrResultContainer.H"
#include "FFaLib/FFaString/FFaTokenizer.H"

#if FFR_DEBUG > 2
long int FFrItemGroup::count = 0;
#endif


FFrItemGroup::FFrItemGroup(bool inlined) : isInlined(inlined)
{
  static std::set<std::string> undefined;
  if (undefined.empty())
    undefined.insert("(undefined)");

  myId = 0;
  myNameIt = undefined.begin();
  // Set the global flag for inlined item groups to avoid it is garbage-collected
  // while traversing the hierarchy
  if (isInlined) this->FFrEntryBase::setGlobal();
#if FFR_DEBUG > 2
  std::cout <<"Creating item group #"<< (myCount = ++count);
  if (isInlined) std::cout <<" (inlined)";
  std::cout << std::endl;
#endif
}

FFrItemGroup::FFrItemGroup(const FFrItemGroup& obj) : FFrFieldEntryBase(obj), isInlined(false)
{
  myId = obj.myId;
  myNameIt = obj.myNameIt;
#if FFR_DEBUG > 2
  std::cout <<"Creating item group #"<< (myCount = ++count)
            <<" Copy of #"<< obj.myCount <<" "<< obj.getDescription() << std::endl;
#endif
  if (obj.isInlined)
    std::cerr <<" *** Logic error: An inline item group should not be copied."<< std::endl;
}

FFrItemGroup::~FFrItemGroup()
{
#if FFR_DEBUG > 2
  std::cout <<"Destroying item group #"<< myCount
            <<" "<< this->getDescription() << std::endl;
#endif
}


const std::string& FFrItemGroup::getType() const
{
  if (myId < 0) return *myNameIt;

  static std::string intStr;
  intStr = std::to_string(myId);
  return intStr;
}


FFrStatus FFrItemGroup::create(FILE* varStream, FFrCreatorData& cd,
			       bool dataBlocks)
{
  FFaTokenizer tokens(varStream,'[',']',';');

  // Check for reference or real item group by examining the size of the tokens.
  // References should only appear in the data blocks section.
  if (tokens.size() == 1 && dataBlocks)
  {
    // Add a pointer to the referred item group
    std::map<int,FFrItemGroup*>::const_iterator it = cd.itemGroups.find(atoi(tokens.front().c_str()));
    if (it == cd.itemGroups.end())
    {
      std::cerr <<" *** Undefined item group "<< tokens.front() << std::endl;
      return FAILED;
    }

    // Found a top level item group
    cd.topLevelEntries.push_back(it->second);
    return LABEL_SEARCH;
  }

  // We have an item group definition
  FFrItemGroup* itgPtr = new FFrItemGroup(dataBlocks);
  int id = itgPtr->fillObject(tokens,cd);
  if (id < 0)
  {
    delete itgPtr;
    return FAILED;
  }
  else if (id == 0 && !dataBlocks)
  {
    // Item groups without ID should only appear in the data blocks section
    std::cerr <<" *** Item group with no ID found in the variable section:"
              <<"\n     ";
    for (const std::string& token : tokens)
      std::cerr <<" \""<< token <<"\"";
    std::cerr << std::endl;
    delete itgPtr;
    return FAILED;
  }

  std::pair<ItemGroupSetIt,bool> status = cd.extractorIGs->insert(itgPtr);
  if (id == 0) return LABEL_SEARCH; // inlined item group

  if (status.second)
  {
    cd.itemGroups[id] = itgPtr;
    itgPtr->setGlobal();
  }
  else // this item group is already in the extractor
  {
    delete itgPtr;
    cd.itemGroups[id] = itgPtr = *status.first;
  }

  // Not likely, but if an item group is defined in the data blocks section
  // it also has to be added as a top-level entry, unless it is inlined
  if (dataBlocks)
    cd.topLevelEntries.push_back(itgPtr);

  return LABEL_SEARCH;
}


/*!
  Puts the data into this item group.
  Returns -1 on error, and 0 if no id could be read [id;name/num;references].
  That would mean an inlined item group which can occur both in the VARIABLES
  and in the DATABLOCKS section. It also resolves the references.
*/

int FFrItemGroup::fillObject(const std::vector<std::string>& tokens,
			     FFrCreatorData& cd)
{
  if (tokens.size() < 3)
  {
    std::cerr <<" *** Fewer than 3 fields in item group description:"
              <<"\n     ";
    for (const std::string& token : tokens)
      std::cerr <<" \""<< token <<"\"";
    std::cerr << std::endl;
    return -1;
  }

  // Check the name to see if it is a number
  const char* idStr = tokens[1].c_str();
  char* endPos;
  int id = strtol(idStr, &endPos, 10);
  if (size_t(endPos-idStr) == tokens[1].size() && id >= 0) // this an integer...
    myId = id;
  else
  {
    myId = -1;
    myNameIt = cd.dict->insert(tokens[1]).first;
  }
#if FFR_DEBUG > 2
  std::cout <<"Resolving item group #"<< myCount <<": "<< this->getType() << std::endl;
#endif

  // Resolve the references of this item group
  if (!this->resolve(tokens[2],cd,isInlined))
    return -2;

  // Everything is OK, return the ID
  return atoi(tokens.front().c_str());
}


/*!
  Comparator functor. Returns true if first argument is less than the second.
*/

bool FFrItemGroup::Less::operator()(const FFrItemGroup* first, const FFrItemGroup* sec) const
{
  if (first->myId < 0 && sec->myId < 0)
  {
    // Both item groups are string-identified
    if (first->myNameIt != sec->myNameIt)
      return *first->myNameIt < *sec->myNameIt;
  }

  // Integer-identified item groups are defined to be "less" than string-identified ones
  else if (first->myId < 0)
    return false;
  else if (sec->myId < 0)
    return true;

  // Both item groups are integer-identified
  else if (first->myId != sec->myId)
    return first->myId < sec->myId;

  // Only if the id is equal, we compare the data fields recursively to
  // determine which is the lesser item. We don't compare the arrays themselves as before,
  // as that will only compare the pointers which may be somewhat random.
  return first->FFrFieldEntryBase::less(sec);
}


int FFrItemGroup::traverse(FFrResultContainer* resultCont,
			   FFrEntryBase* owner,
			   FFrEntryBase*& objToBeMod,
			   int binPos)
{
  if (!isInlined)
  {
    // Copy this item group
    objToBeMod = new FFrItemGroup(*this);
    objToBeMod->setOwner(owner);
  }
  FFrItemGroup* igrp = static_cast<FFrItemGroup*>(objToBeMod);

  // Note: Can not use range-based loop here, since the container will expand
  std::vector<FFrEntryBase*>::iterator it;
  for (it = igrp->dataFields.begin(); it != igrp->dataFields.end(); ++it)
  {
    if (!(*it)->isGlobal()) resultCont->collectGarbage(*it);
    binPos = (*it)->traverse(resultCont, igrp, *it, binPos);
  }

  return binPos;
}


void FFrItemGroup::setGlobal()
{
  this->FFrEntryBase::setGlobal();
  for (FFrEntryBase* field : dataFields)
    field->setGlobal();
}


bool FFrItemGroup::compare(const FFrEntryBase* obj) const
{
  const FFrItemGroup* varObj = dynamic_cast<const FFrItemGroup*>(obj);
  if (!varObj) return false;

  if (this->myId < 0 && varObj->myId < 0)
    return (*this->myNameIt == *varObj->myNameIt);
  else
    return (this->myId == varObj->myId);
}


bool FFrItemGroup::equal(const FFrEntryBase* obj) const
{
  return this->compare(obj) ? this->FFrFieldEntryBase::equal(obj) : false;
}


bool FFrItemGroup::less(const FFrEntryBase* obj) const
{
  const FFrItemGroup* that = dynamic_cast<const FFrItemGroup*>(obj);
  if (!that) return false;

  Less lessThan;
  return lessThan(this,that);
}


#ifdef FFR_NEWALLOC
#define BLOCK_SIZE 512

size_t FFrItemGroup::newed = 0;
FFrItemGroup* FFrItemGroup::headOfFreeList = NULL;
std::vector<FFrItemGroup*> FFrItemGroup::memBlocks;


void* FFrItemGroup::operator new(size_t size)
{
  if (size != sizeof(FFrItemGroup)) {
    std::cerr <<"FFrItemGroup::operator new: wrong size "<< size << std::endl;
    return ::operator new(size);
  }

  FFrItemGroup* p = headOfFreeList;

  if (p)
    headOfFreeList = p->next;
  else {
    p = static_cast<FFrItemGroup*>(::operator new(BLOCK_SIZE*sizeof(FFrItemGroup)));
    newed += BLOCK_SIZE*sizeof(FFrItemGroup);

    memBlocks.push_back(p);

    for (size_t i = 1; i < BLOCK_SIZE-1; i++)
      p[i].next = &p[i+1];
    p[BLOCK_SIZE-1].next = NULL;

    headOfFreeList = &p[1];
  }

  return p;
}


void FFrItemGroup::operator delete(void* deadObject, size_t size)
{
  if (!deadObject) return;

  if (size != sizeof(FFrItemGroup)) {
    std::cerr <<"FFrItemGroup::operator delete: Wrong size "<< size << std::endl;
    ::operator delete(deadObject);
    return;
  }

  FFrItemGroup* carcass = static_cast<FFrItemGroup*>(deadObject);

  carcass->next = headOfFreeList;
  headOfFreeList = carcass;
}


void FFrItemGroup::releaseMemBlocks()
{
  for (FFrItemGroup* itemGroup : memBlocks)
    if (itemGroup) ::operator delete(itemGroup);

  headOfFreeList = NULL;
  newed = 0;

  std::vector<FFrItemGroup*> empty;
  memBlocks.swap(empty);
}

#endif
