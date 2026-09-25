// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#include "FFrLib/FFrEntryBase.H"
#include "FFrLib/FFrVariableReference.H"
#include "FFaLib/FFaDefinitions/FFaResultDescription.H"
#include "FFaLib/FFaAlgebra/FFaMat34.H"


#if FFR_DEBUG > 2
void FFrEntryBase::printID(std::ostream& os, bool EOL) const
{
  if (this->isOG())
    os <<"Object group #";
  else if (this->isSOG())
    os <<"Superobject group #";
  else if (this->isIG())
    os <<"Item group #";
  else if (this->isVarRef())
    os <<"Variable reference #";
  os << myCount <<": ";
  if (this->isOG())
    os << this->getType() <<" \""<< this->getDescription() <<"\"";
  else
    os << this->getDescription();
  if (EOL) os << std::endl;
}


void FFrEntryBase::setOwner(FFrEntryBase* owner)
{
  if (myOwner == owner)
    return;

  myOwner = owner;

  this->printID(std::cout,false);
  std::cout <<" is owned by ";
  if (owner)
    owner->printID(std::cout);
  else
    std::cout <<"(NULL)"<< std::endl;
}
#endif


FFaResultDescription FFrEntryBase::getEntryDescription() const
{
  FFaResultDescription descr;

  if (this->isVarRef())
    descr.varRefType = this->getType();

  for (const FFrEntryBase* entry = this; entry; entry = entry->getOwner())
    if (entry->isSOG())
    {
      descr.OGType = entry->getType();
      descr.baseId = -1;
    }
    else if (entry->isOG())
    {
      descr.OGType = entry->getType();
      descr.baseId = entry->hasBaseID() ? entry->getBaseID() : 0;
      descr.userId = entry->hasUserID() ? entry->getUserID() : 0;
      break;
    }
    else
      descr.varDescrPath.insert(descr.varDescrPath.begin(),
                                entry->getDescription());

  return descr;
}


template<class T> bool FFrEntryBase::readVar(T& value)
{
  FFrVariableReference* vRef = dynamic_cast<FFrVariableReference*>(this);
  if (!vRef)
    std::cerr <<" *** FFrEntryBase::readVar(): Logic error,"
              <<" expected a variable reference, found \""
              << this->getDescription() <<"\""<< std::endl;

  else if (vRef->hasDataForCurrentKey(true)) // use the positioned time step
    return vRef->readVariable(value);

#ifdef FFR_DEBUG
  else
    std::cerr <<" *** FFrEntryBase::readVar(): \""<< this->getDescription()
              <<"\" has no data at the requested time, distance = "
              << vRef->getDistanceFromResultPoint() << std::endl;
#endif

  return false;
}

using Vec3Vec = std::vector<FaVec3>;

template bool FFrEntryBase::readVar<double>(double&);
template bool FFrEntryBase::readVar<FaVec3>(FaVec3&);
template bool FFrEntryBase::readVar<FaMat34>(FaMat34&);
template bool FFrEntryBase::readVar<Vec3Vec>(Vec3Vec&);
