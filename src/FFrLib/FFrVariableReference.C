// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#include "FFrLib/FFrVariableReference.H"
#include "FFrLib/FFrResultContainer.H"
#include "FFrLib/FFrReadOp.H"
#include "FFaLib/FFaAlgebra/FFaMat34.H"
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

  vRef->containers.emplace_back(resultCont,binPos);
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
  switch (containers.size()) {
  case 0: break;
  case 1: arrayPos += containers.front().first->readPositionedTimestepData(&vals[arrayPos], nvals - arrayPos,
                                                                           containers.front().second,
                                                                           variableDescr->dataSize,
                                                                           variableDescr->getRepeats()); break;
  default:
    if (int idx = this->getNearestContainer(); idx >= 0)
      arrayPos += containers[idx].first->readPositionedTimestepData(&vals[arrayPos], nvals - arrayPos,
                                                                    containers[idx].second,
                                                                    variableDescr->dataSize,
                                                                    variableDescr->getRepeats());
  }
  return arrayPos;
}


int FFrVariableReference::recursiveReadPosData(const float* vals, int nvals, int arrayPos) const
{
  switch (containers.size()) {
  case 0: break;
  case 1: arrayPos += containers.front().first->readPositionedTimestepData(&vals[arrayPos], nvals - arrayPos,
                                                                           containers.front().second,
                                                                           variableDescr->dataSize,
                                                                           variableDescr->getRepeats()); break;
  default:
    if (int idx = this->getNearestContainer(); idx >= 0)
      arrayPos += containers[idx].first->readPositionedTimestepData(&vals[arrayPos], nvals - arrayPos,
                                                                    containers[idx].second,
                                                                    variableDescr->dataSize,
                                                                    variableDescr->getRepeats());
  }
  return arrayPos;
}


int FFrVariableReference::recursiveReadPosData(const int* vals, int nvals, int arrayPos) const
{
  switch (containers.size()) {
  case 0: break;
  case 1: arrayPos += containers.front().first->readPositionedTimestepData(&vals[arrayPos], nvals - arrayPos,
                                                                           containers.front().second,
                                                                           variableDescr->dataSize,
                                                                           variableDescr->getRepeats()); break;
  default:
    if (int idx = this->getNearestContainer(); idx >= 0)
      arrayPos += containers[idx].first->readPositionedTimestepData(&vals[arrayPos], nvals - arrayPos,
                                                                    containers[idx].second,
                                                                    variableDescr->dataSize,
                                                                    variableDescr->getRepeats());
  }
  return arrayPos;
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
  }

  // Not trivially found. Check the closest container
  unsigned int lastDate = containers.front().first->getDate();
  for (size_t i = 1; i < containers.size(); i++)
    if (containers[i].first->getDate() > lastDate)
      lastDate = containers[i].first->getDate();

  return lastDate;
}


double FFrVariableReference::getDistanceFromResultPoint(const bool usePositionedKey) const
{
  switch (containers.size())
  {
    case 0:
      return DBL_MAX;
    case 1:
      return containers.front().first->getDistanceFromPosKey(usePositionedKey);
  }

  // Not trivially found. Check the closest container
  double closestDist = containers.front().first->getDistanceFromPosKey(usePositionedKey);
  for (size_t i = 1; i < containers.size(); i++)
    if (double dist = containers[i].first->getDistanceFromPosKey(usePositionedKey);
        fabs(dist) < fabs(closestDist))
      closestDist = dist;

  return closestDist;
}


bool FFrVariableReference::hasDataForCurrentKey(const bool usePositionedKey) const
{
  double dist = this->getDistanceFromResultPoint(usePositionedKey);
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


template<class T> bool FFrVariableReference::readVariable(T& value)
{
  bool ok = false;
  if (FFaOperationBase* readOp = this->getReadOperation(); readOp)
  {
    if (FFaOperation<T>* op = dynamic_cast<FFaOperation<T>*>(readOp); op)
      ok = op->evaluate(value);

    readOp->unref();
  }

  if (!ok)
    std::cerr <<" *** FFrVariableReference: Invalid read operation for \""
              << this->getDescription() <<"\"."<< std::endl;
  return ok;
}

template bool FFrVariableReference::readVariable<FaVec3>(FaVec3&);
template bool FFrVariableReference::readVariable<FaMat34>(FaMat34&);

//! Specialization for double variables - also accounting for float data.
template<> bool FFrVariableReference::readVariable<double>(double& value)
{
  using DblOper = FFaOperation<double>;
  using FltOper = FFaOperation<float>;

  bool ok = false;
  if (FFaOperationBase* readOp = this->getReadOperation(); readOp)
  {
    if (DblOper* op = dynamic_cast<DblOper*>(readOp); op)
      ok = op->evaluate(value);
    else if (FltOper* op = dynamic_cast<FltOper*>(readOp); op)
      if (float floatVal; (ok = op->evaluate(floatVal)))
        value = static_cast<double>(floatVal);

    readOp->unref();
  }

  if (!ok)
    std::cerr <<" *** FFrVariableReference: Invalid read operation for \""
              << this->getDescription() <<"\"."<< std::endl;
  return ok;
}


using Vec3Vec = std::vector<FaVec3>;

//! Specialization for array for FaVec3 objects - reading from a 1D array.
template<> bool FFrVariableReference::readVariable<Vec3Vec>(Vec3Vec& value)
{
  using DoubleVec  = std::vector<double>;
  using DblVecOper = FFaOperation<DoubleVec>;

  bool ok = false;
  if (FFaOperationBase* readOp = this->getReadOperation(); readOp)
  {
    if (DblVecOper* op = dynamic_cast<DblVecOper*>(readOp); op)
      if (DoubleVec data; (ok = op->evaluate(data)))
      {
        value.resize(data.size()/3);
        DoubleVec::const_iterator dit = data.begin();
        for (FaVec3& X : value)
          for (int i = 0; i < 3; i++, ++dit)
            X[i] = *dit;
      }

    readOp->unref();
  }

  if (!ok)
    std::cerr <<" *** FFrVariableReference: Invalid read operation for \""
              << this->getDescription() <<"\"."<< std::endl;
  return ok;
}
