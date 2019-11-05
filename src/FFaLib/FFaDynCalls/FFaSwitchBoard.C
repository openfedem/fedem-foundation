// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#include "FFaLib/FFaDynCalls/FFaSwitchBoard.H"
#include <iostream>


int FFaSlotBase::uniqueTypeId = 0;

// Static member initialization FFaSwitchBoard :
typedef std::map<FFaSwitchBoardConnector*,  std::map<int,TypeIDToSlotBasePtListMap> > SwitchBoardConnectionT;

SwitchBoardConnectionT* FFaSwitchBoard::ourConnections = 0;


static FFaSlotList::iterator eraseSlot(FFaSwitchBoardConnector* sender, int subject,
                                       FFaSlotList& slots, FFaSlotList::iterator slit)
{
  if (!slit->isValid)
    return ++slit;

  slit->slotPt->removeConnection(sender,subject);

  if (slit->protectedRefCount < 1)
    return slots.erase(slit);

  slit->isValid = false;
  return ++slit;
}


void FFaSwitchBoard::init()
{
  FFaSwitchBoard::ourConnections = new SwitchBoardConnectionT();
}


void FFaSwitchBoard::removeInstance()
{
  delete FFaSwitchBoard::ourConnections;
}


// Initialisation of FFaSlotBase0::ourClassTypeID :
//FFASWB_MAKE_SLOTBASE_ID_INIT_TEMPLATE(0,,)

//
// The macro to make the different overloaded emit template functions :
//


void FFaSwitchBoard::connect(FFaSwitchBoardConnector * aSender, int aSubject, FFaSlotBase *aSlot)
{
  FFaSlotBasePt slotHolder(aSlot);
  (((ourConnections->operator[](aSender))[aSubject])[aSlot->getTypeID()]).push_front(slotHolder);
  aSlot->addConnection(aSender,aSubject);
}


void FFaSwitchBoard::disConnect(FFaSwitchBoardConnector* sender, int subject, FFaSlotBase* slot)
{
  // Increment to avoid accidental removal during looping if it is a working slot
  slot->addConnection(sender,subject);

  FFaSlotList& ffaSlots = FFaSwitchBoard::getSlots(sender,subject,slot->getTypeID());
  for (FFaSlotList::iterator it = ffaSlots.begin(); it != ffaSlots.end();)
    if (*slot == *(it->slotPt))
      it = eraseSlot(sender,subject,ffaSlots,it);
    else
      ++it;

  if (ffaSlots.empty())
    FFaSwitchBoard::cleanUpAfterSlot(sender,subject);

  // Decrement to make it disappear if no longer used
  slot->removeConnection(sender,subject);
}


void FFaSwitchBoard::removeAllSenderConnections(FFaSwitchBoardConnector* sender)
{
  SwitchBoardConnectionT::iterator sbcIter = ourConnections->find(sender);
  if (sbcIter == ourConnections->end())
    return; // Nothing to do

  std::map<int,TypeIDToSlotBasePtListMap>::iterator subjIt, subjToDelete;
  for (subjIt = sbcIter->second.begin(); subjIt != sbcIter->second.end();)
  {
    TypeIDToSlotBasePtListMap::iterator typeIt, typeToDelete;
    for (typeIt = subjIt->second.begin(); typeIt != subjIt->second.end();)
    {
      for (FFaSlotList::iterator it = typeIt->second.begin(); it != typeIt->second.end();)
        it = eraseSlot(sender,subjIt->first,typeIt->second,it);

      typeToDelete = typeIt++;
      if (typeToDelete->second.empty())
        subjIt->second.erase(typeToDelete);
    }

    subjToDelete = subjIt++;
    if (subjToDelete->second.empty())
      sbcIter->second.erase(subjToDelete);
  }

  if (sbcIter->second.empty())
    ourConnections->erase(sender);
}


void FFaSwitchBoard::removeSlotReference(FFaSwitchBoardConnector* sender, int subject, FFaSlotBase* slot)
{
  FFaSlotList& ffaSlots = FFaSwitchBoard::getSlots(sender,subject,slot->getTypeID());
  for (FFaSlotList::iterator it = ffaSlots.begin(); it != ffaSlots.end();)
    if (it->slotPt == slot)
      it = eraseSlot(sender,subject,ffaSlots,it);
    else
      ++it;

  if (ffaSlots.empty())
    FFaSwitchBoard::cleanUpAfterSlot(sender,subject);
}


FFaSlotList& FFaSwitchBoard::getSlots(FFaSwitchBoardConnector* sender,
                                      int subject, unsigned int typeID)
{
  SwitchBoardConnectionT::iterator it1 = ourConnections->find(sender);
  if (it1 != ourConnections->end())
  {
    std::map<int,TypeIDToSlotBasePtListMap>::iterator it2 = it1->second.find(subject);
    if (it2 != it1->second.end())
    {
      TypeIDToSlotBasePtListMap::iterator it3 = it2->second.find(typeID);
      if (it3 != it2->second.end())
        return it3->second;
    }
  }

  static FFaSlotList empty;
  return empty;
}


/*!
  Function that finds the next slot that is available and valid for invoking.
  Used when a signal is emitted using the FFsSwitchBoardCall function to
  iterate over the slots that can be deleted as we go.
*/

FFaSlotList::iterator
FFaSwitchBoard::nextValidSlot(FFaSlotList::iterator it, FFaSlotList& slots,
                              FFaSwitchBoardConnector* sender, int subject)
{
  if (it == slots.end())
    it = slots.begin();

  else
  {
    if (--(it->protectedRefCount) < 0)
      std::cerr <<"Error: FFaSwitchBoard: Negative protectedRefcount"<< std::endl;

    if (!it->isValid)
      it = slots.erase(it);
    else
      ++it;
  }

  while (it != slots.end())
    if (it->isValid)
    {
      it->protectedRefCount++;
      return it;
    }
    else
      ++it;

  if (slots.empty())
    FFaSwitchBoard::cleanUpAfterSlot(sender,subject);

  return it;
}


void FFaSwitchBoard::cleanUpAfterSlot(FFaSwitchBoardConnector* sender, int subject)
{
  SwitchBoardConnectionT::iterator sbcIter = ourConnections->find(sender);
  if (sbcIter == ourConnections->end())
    return; // Nothing to do

  std::map<int,TypeIDToSlotBasePtListMap>::iterator subjIt = sbcIter->second.find(subject);
  if (subjIt != sbcIter->second.end())
    sbcIter->second.erase(subjIt);

  if (sbcIter->second.empty())
    ourConnections->erase(sbcIter);
}


FFaSwitchBoardConnector::~FFaSwitchBoardConnector()
{
  IAmDeletingMe = true;
  for (FFaSlotBase* slot : mySlots) delete slot;
  FFaSwitchBoard::removeAllSenderConnections(this);
}


FFaSlotBase::~FFaSlotBase()
{
  IAmDeletingMe = true;
  for (const SwitchBoardConnectorMap::value_type& swb : mySwitchBoardLookups)
    for (const std::pair<const int,int>& ij : swb.second)
      FFaSwitchBoard::removeSlotReference(swb.first,ij.first,this);
}


void FFaSlotBase::addConnection(FFaSwitchBoardConnector* sender, int subject)
{
  if (IAmDeletingMe) return;

  IntMap& lookUp = mySwitchBoardLookups[sender];
  IntMap::iterator it = lookUp.find(subject);
  if (it == lookUp.end())
    lookUp[subject] = 1;
  else
    it->second++;
}


void FFaSlotBase::removeConnection(FFaSwitchBoardConnector* sender, int subject)
{
  if (IAmDeletingMe) return;

  SwitchBoardConnectorMap::iterator cit = mySwitchBoardLookups.find(sender);
  if (cit != mySwitchBoardLookups.end())
  {
    IntMap::iterator it = cit->second.find(subject);
    if (it != cit->second.end() && --(it->second) < 1)
      cit->second.erase(it);

    if (cit->second.empty())
      mySwitchBoardLookups.erase(cit);
  }

  if (mySwitchBoardLookups.empty())
    delete this;
}
