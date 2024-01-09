// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#include "FFrLib/FFrVariableReference.H"
#include "FFrLib/FFrResultContainer.H"
#include "FFrLib/FFrReadOp.H"
#include <float.h>
#include <math.h>

#if FFR_DEBUG > 2
long int FFrVariableReference::count = 0;


FFrVariableReference::FFrVariableReference(FFrVariable* v) : variableDescr(v)
{
  std::cout <<"Creating variable reference #"<< (myCount = ++count);
  if (variableDescr) std::cout <<": "<< variableDescr->name;
  std::cout << std::endl;
}

FFrVariableReference::FFrVariableReference(const FFrVariableReference& varRef)
  : variableDescr(varRef.variableDescr)
{
  std::cout <<"Copying variable reference #"<< (myCount = ++count)
            <<" <== #" << varRef.myCount;
  if (variableDescr) std::cout <<": "<< variableDescr->name;
  std::cout << std::endl;
}

FFrVariableReference::~FFrVariableReference()
{
  std::cout <<"Destroying variable reference #"<< myCount;
  if (variableDescr) std::cout <<": "<< variableDescr->name;
  std::cout << std::endl;
}
#endif


const std::string& FFrVariableReference::getDescription() const
{
  return variableDescr->name;
}


const std::string& FFrVariableReference::getType() const
{
  return variableDescr->dataClass;
}


int FFrVariableReference::traverse(FFrResultContainer* resultCont,
				   FFrEntryBase* owner,
				   FFrEntryBase*& objToBeMod,
				   int binPos)
{
  // Copy this variable reference
  objToBeMod = new FFrVariableReference(*this);
  objToBeMod->setOwner(owner);
  FFrVariableReference* vRef = static_cast<FFrVariableReference*>(objToBeMod);

  vRef->containers.push_back(std::make_pair(resultCont,binPos));
  binPos += vRef->variableDescr->getTotalDataSize();

  return binPos;
}


bool FFrVariableReference::merge(FFrEntryBase* obj)
{
  if (!this->compare(obj))
    return false;

  // Assume (without checking) the object we are merging from has only one file reference
  FFrVariableReference* that = static_cast<FFrVariableReference*>(obj);
  this->containers.push_back(that->containers.front());
  return true;
}


bool FFrVariableReference::equal(const FFrEntryBase* obj) const
{
  const FFrVariableReference* that = dynamic_cast<const FFrVariableReference*>(obj);
  return that ? this->variableDescr->equal(that->variableDescr) : false;
}


bool FFrVariableReference::less(const FFrEntryBase* obj) const
{
  const FFrVariableReference* that = dynamic_cast<const FFrVariableReference*>(obj);
  return that ? this->variableDescr->less(that->variableDescr) : false;
}


void FFrVariableReference::removeContainers(const std::set<FFrResultContainer*>& cont)
{
  std::vector<FFrResultContainerRef> tmp;
  for (FFrResultContainerRef& contref : containers)
    if (cont.find(contref.first) == cont.end())
      tmp.push_back(contref);

  containers.swap(tmp);
}


int FFrVariableReference::getNearestContainer() const
{
  double closestDist = DBL_MAX;
  int closestContainer = -1;
  int matchingContainer = -1;
  unsigned int latestDate = 0;
  for (size_t i = 0; i < containers.size(); i++)
  {
    // check if this container has data for the wanted key:
    double dist = containers[i].first->getDistanceFromPosKey();
    if (dist < FLT_EPSILON && dist > -FLT_EPSILON)
    {
      // if several containers have data for this key, pick the most recent one
      if (matchingContainer < 0 || containers[i].first->getDate() > latestDate)
      {
        latestDate = containers[i].first->getDate();
        matchingContainer = i;
      }
    }
    else if (matchingContainer < 0)
    {
      // no match yet, find the closest container
      if (containers[i].first->getDistanceToNextKey(dist) && dist < closestDist)
      {
        closestDist = dist;
        closestContainer = i;
      }
    }
  }

  return matchingContainer >= 0 ? matchingContainer : closestContainer;
}


int FFrVariableReference::recursiveReadPosData(const double* vals, int nvals, int arrayPos) const
{
  // find the closest position (Switching faster than if)
  // (Ugly look to preserve line nrs. during optimization)
  switch (containers.size()){
  case 0: return arrayPos;
  case 1: return arrayPos + containers.front().first->readPositionedTimestepData(&vals[arrayPos], nvals - arrayPos,
										 containers.front().second, variableDescr->dataSize,
										 variableDescr->getRepeats());
  default:{
    register int closestContainer = this->getNearestContainer();
    if (closestContainer < 0) return arrayPos;
    return arrayPos + containers[closestContainer].first->readPositionedTimestepData(&vals[arrayPos], nvals - arrayPos,
										     containers[closestContainer].second,
										     variableDescr->dataSize, variableDescr->getRepeats());
  }
  }
}


int FFrVariableReference::recursiveReadPosData(const float* vals, int nvals, int arrayPos) const
{
  // find the closest position (Switching faster than if)
  // (Ugly look to preserve line nrs. during optimization)
  switch (containers.size()){
  case 0: return arrayPos;
  case 1: return arrayPos + containers.front().first->readPositionedTimestepData(&vals[arrayPos], nvals - arrayPos,
										 containers.front().second, variableDescr->dataSize,
										 variableDescr->getRepeats());
  default:{
    register int closestContainer = this->getNearestContainer();
    if (closestContainer < 0) return arrayPos;
    return arrayPos + containers[closestContainer].first->readPositionedTimestepData(&vals[arrayPos], nvals - arrayPos,
										     containers[closestContainer].second,
										     variableDescr->dataSize, variableDescr->getRepeats());
  }
  }
}


int FFrVariableReference::recursiveReadPosData(const int* vals, int nvals, int arrayPos) const
{
  // find the closest position (Switching faster than if)
  // (Ugly look to preserve line nrs. during optimization)
  switch (containers.size()){
  case 0: return arrayPos;
  case 1: return arrayPos + containers.front().first->readPositionedTimestepData(&vals[arrayPos], nvals - arrayPos,
										 containers.front().second, variableDescr->dataSize,
										 variableDescr->getRepeats());
  default:{
    register int closestContainer = this->getNearestContainer();
    if (closestContainer < 0) return arrayPos;
    return arrayPos + containers[closestContainer].first->readPositionedTimestepData(&vals[arrayPos], nvals - arrayPos,
										     containers[closestContainer].second,
										     variableDescr->dataSize, variableDescr->getRepeats());
  }
  }
}


bool FFrVariableReference::isVariableFloat() const
{
  return (variableDescr->dataType == FFrVariable::FLOAT && variableDescr->dataSize == 32);
}


FFaOperationBase* FFrVariableReference::getReadOperation()
{
  return OperationFactory::instance()->create(ReadOpCreatorType(variableDescr->dataClass, variableDescr->dataSize), this);
}


unsigned int FFrVariableReference::getTimeStamp() const
{
  switch (containers.size())
  {
    case 0:
      return 0;
    case 1:
      return containers.front().first->getDate();
    default:
    {
      // Not trivially found. Check the closest container
      unsigned int lastDate = containers.front().first->getDate();
      for (size_t i = 1; i < containers.size(); i++)
	if (containers[i].first->getDate() > lastDate)
	  lastDate = containers[i].first->getDate();
      return lastDate;
    }
  }
}


double FFrVariableReference::getDistanceFromResultPoint(const bool usePositionedKey) const
{
  switch (containers.size())
  {
    case 0:
      return DBL_MAX;
    case 1:
      return containers.front().first->getDistanceFromPosKey(usePositionedKey);
    default:
    {
      // Not trivially found. Check the closest container
      register double closestDist = containers.front().first->getDistanceFromPosKey(usePositionedKey);
      for (size_t i = 1; i < containers.size(); i++)
      {
	register double dist = containers[i].first->getDistanceFromPosKey(usePositionedKey);
	if (fabs(dist) < fabs(closestDist)) closestDist = dist;
      }
      return closestDist;
    }
  }
}


bool FFrVariableReference::hasDataForCurrentKey(const bool usePositionedKey) const
{
  register double dist = this->getDistanceFromResultPoint(usePositionedKey);
  return (dist < FLT_EPSILON && dist > -FLT_EPSILON);
}


void FFrVariableReference::getValidKeys(std::set<double>& validValues) const
{
  for (const FFrResultContainerRef& cref : containers)
  {
    std::set<double>::iterator lastInserted = validValues.begin();
    for (const std::pair<const double,int>& time : cref.first->getPhysicalTime())
      lastInserted = validValues.insert(lastInserted,time.first);
  }
}


void FFrVariableReference::printPosition(std::ostream& os) const
{
#if FFR_DEBUG > 2
  os <<"\nVariable reference #"<< myCount <<": "<< variableDescr->name;
#else
  os <<"\nName: "<< variableDescr->name;
#endif
  for (const FFrResultContainerRef& cref : containers)
    os <<"\nContainer: "<< cref.first->getFileName()
       <<"\nHeader size: "<< cref.first->getHeaderSize()
       <<"\nTimestep size: "<< cref.first->getStepSize()
       <<"\nPosition: "<< (cref.second >> 3);
  os <<"\nSize: "<< (variableDescr->getTotalDataSize() >> 3)
     <<"\nType: "<< variableDescr->dataClass <<"("<< variableDescr->dataType
     <<")\nUnit: "<< variableDescr->unit << std::endl;
}


#ifdef FFR_NEWALLOC
#define BLOCK_SIZE 512

size_t FFrVariableReference::newed = 0;
FFrVariableReference* FFrVariableReference::headOfFreeList = NULL;
std::vector<FFrVariableReference*> FFrVariableReference::memBlocks;


void* FFrVariableReference::operator new(size_t size)
{
  if (size != sizeof(FFrVariableReference)) {
    std::cerr <<"FFrVariableReference::operator new: Wrong size %d"<< size << std::endl;
    return ::operator new(size);
  }

  FFrVariableReference* p = headOfFreeList;

  if (p)
    headOfFreeList = p->next;
  else {
    FFrVariableReference* newBlock = static_cast<FFrVariableReference*>(::operator new(BLOCK_SIZE*sizeof(FFrVariableReference)));

    newed += BLOCK_SIZE*sizeof(FFrVariableReference);

    memBlocks.push_back(newBlock);

    for (size_t i = 1; i < BLOCK_SIZE-1; i++)
      newBlock[i].next = &newBlock[i+1];
    newBlock[BLOCK_SIZE-1].next = NULL;

    p = newBlock;
    headOfFreeList = &newBlock[1];
  }
  return p;
}


void FFrVariableReference::operator delete(void* deadObject, size_t size)
{
  if (!deadObject) return;

  if (size != sizeof(FFrVariableReference)) {
    std::cerr <<"FFrVariableReference::operator delete: Wrong size "<< size << std::endl;
    ::operator delete(deadObject);
    return;
  }

  FFrVariableReference* carcass = static_cast<FFrVariableReference*>(deadObject);

  carcass->next = headOfFreeList;
  headOfFreeList = carcass;
}


void FFrVariableReference::releaseMemBlocks()
{
  for (FFrVariableReference* varRef : memBlocks)
    if (varRef) ::operator delete(varRef);

  headOfFreeList = NULL;
  newed = 0;

  std::vector<FFrVariableReference*> empty;
  memBlocks.swap(empty);
}

#endif
