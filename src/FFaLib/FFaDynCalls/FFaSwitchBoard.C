// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#include "FFaLib/FFaDynCalls/FFaSwitchBoard.H"
#ifdef FFA_DEBUG
#include <iostream>
#endif


int FFaSlotBase::uniqueTypeId = 0;

FFaSwitchBoard::SwitchBoardConnection* FFaSwitchBoard::ourConnections = NULL;


static FFaSlotList::iterator eraseSlot(FFaSwitchBoardConnector* sender, int subject,
                                       FFaSlotList& slots, FFaSlotList::iterator slit)
{
  if (slit->refCount < 0)
    return ++slit;

  slit->slotPt->removeConnection(sender,subject);

  if (slit->refCount < 1)
    return slots.erase(slit);

  slit->refCount = -1;
  return ++slit;
}


void FFaSwitchBoard::init()
{
  FFaSwitchBoard::ourConnections = new SwitchBoardConnection();
}


void FFaSwitchBoard::removeInstance()
{
  delete FFaSwitchBoard::ourConnections;
}


void FFaSwitchBoard::connect(FFaSwitchBoardConnector* sender, int subject, FFaSlotBase* slot)
{
  ourConnections->operator[](sender)[subject][slot->getTypeID()].push_front({slot,0});
  slot->addConnection(sender,subject);
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
  SwitchBoardConnection::iterator sbcIter = ourConnections->find(sender);
  if (sbcIter == ourConnections->end())
    return; // Nothing to do

  SlotMap::iterator subjIt, subjToDelete;
  for (subjIt = sbcIter->second.begin(); subjIt != sbcIter->second.end();)
  {
    SlotContainer::iterator typeIt, typeToDelete;
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
  SwitchBoardConnection::iterator it1 = ourConnections->find(sender);
  if (it1 != ourConnections->end())
  {
    SlotMap::iterator it2 = it1->second.find(subject);
    if (it2 != it1->second.end())
    {
      SlotContainer::iterator it3 = it2->second.find(typeID);
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
#ifdef FFA_DEBUG
  size_t islot = 0, nslot = slots.size();
#endif
  if (it == slots.end())
  {
#ifdef FFA_DEBUG
    std::cout <<"FFaSwitchBoard: sender=\""<< sender->getLabel()
              <<"\" signal="<< subject <<" nslot="<< nslot << std::endl;
#endif
    it = slots.begin();
  }
  else
  {
#ifdef FFA_DEBUG
    for (FFaSlotList::iterator j = slots.begin(); j != it && j != slots.end(); ++j)
      ++islot;
#endif
    if (it->refCount < 0)
    {
#ifdef FFA_DEBUG
      std::cout <<"\tErasing invalid slot "<< islot << std::endl;
#endif
      it = slots.erase(it);
    }
    else
    {
      it->refCount--;
      ++it;
#ifdef FFA_DEBUG
      ++islot;
#endif
    }
  }

  for (; it != slots.end(); ++it)
    if (it->refCount >= 0)
    {
      it->refCount++;
#ifdef FFA_DEBUG
      std::cout <<"\tUsing valid slot "<< islot << std::endl;
#endif
      return it;
    }
#ifdef FFA_DEBUG
    else
      ++islot;
#endif

  if (slots.empty())
    FFaSwitchBoard::cleanUpAfterSlot(sender,subject);

#ifdef FFA_DEBUG
  if (nslot > 0)
    std::cout <<"\tNo valid slot "<< islot << std::endl;
#endif
  return it;
}


void FFaSwitchBoard::cleanUpAfterSlot(FFaSwitchBoardConnector* sender, int subject)
{
  SwitchBoardConnection::iterator sbcIter = ourConnections->find(sender);
  if (sbcIter == ourConnections->end())
    return; // Nothing to do

  SlotMap::iterator subjIt = sbcIter->second.find(subject);
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
