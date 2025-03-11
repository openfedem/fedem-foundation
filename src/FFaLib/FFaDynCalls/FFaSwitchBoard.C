// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#include "FFaLib/FFaDynCalls/FFaSwitchBoard.H"
#include <iostream>


int FFaSlotBase::uniqueTypeId = 0;


namespace
{
  using SlotContainer         = std::map<unsigned int,FFaSlotList>;
  using SlotMap               = std::map<int,SlotContainer>;
  using SwitchBoardConnection = std::map<FFaSwitchBoardConnector*,SlotMap>;

  //! Switchboard container (Sender, Subject, SlotTypeId) --> List of slots
  static SwitchBoardConnection* ourConnections = NULL;


  FFaSlotList::iterator eraseSlot(FFaSwitchBoardConnector* sender, int subject,
                                  FFaSlotList& slots,
                                  FFaSlotList::iterator slit)
  {
#ifdef FFA_DEBUG
    std::cout <<"FFaSwitchBoard::eraseSlot("<< sender->getLabel()
              <<"): "<< subject <<" size="<< slots.size()
              <<" refCount="<< slit->refCount << std::endl;
#endif
    if (slit->refCount < 0 || !slit->slotPt)
      return ++slit;

    if (slit->slotPt->removeConnection(sender,subject))
      slit->slotPt = NULL; // FFaSlot object was deleted

    if (slit->refCount < 1)
      return slots.erase(slit);

    slit->refCount = -1;
    return ++slit;
  }


  void cleanUpAfterSlot(FFaSwitchBoardConnector* sender, int subject)
  {
    if (!ourConnections) return;

    SwitchBoardConnection::iterator sbcIter = ourConnections->find(sender);
    if (sbcIter == ourConnections->end()) return; // Nothing to do

    SlotMap::iterator subjIt = sbcIter->second.find(subject);
    if (subjIt != sbcIter->second.end())
      sbcIter->second.erase(subjIt);

    if (sbcIter->second.empty())
      ourConnections->erase(sbcIter);
  }
}


void FFaSwitchBoard::removeInstance()
{
  delete ourConnections;
  ourConnections = NULL;
}


void FFaSwitchBoard::connect(FFaSwitchBoardConnector* sender, int subject,
                             FFaSlotBase* slot)
{
  if (!ourConnections)
    ourConnections = new SwitchBoardConnection();

  SlotMap& connections = ourConnections->operator[](sender);
  connections[subject][slot->getTypeID()].push_front({slot,0});
  slot->addConnection(sender,subject);
}


void FFaSwitchBoard::disConnect(FFaSwitchBoardConnector* sender, int subject,
                                FFaSlotBase* slot)
{
  // Increment to avoid accidental removal during looping
  // if this is a working slot
  slot->addConnection(sender,subject);

  FFaSlotList& ffaSlots = getSlots(sender,subject,slot->getTypeID());
  FFaSlotList::iterator it = ffaSlots.begin();
  while (it != ffaSlots.end())
    if (*slot == *(it->slotPt))
      it = eraseSlot(sender,subject,ffaSlots,it);
    else
      ++it;

  if (ffaSlots.empty())
    cleanUpAfterSlot(sender,subject);

  // Decrement to make it disappear if no longer used
  slot->removeConnection(sender,subject);
}


void FFaSwitchBoard::removeAllSenderConnections(FFaSwitchBoardConnector* sender)
{
  if (!ourConnections) return;

  SwitchBoardConnection::iterator sbcIter = ourConnections->find(sender);
  if (sbcIter == ourConnections->end()) return; // Nothing to do

#ifdef FFA_DEBUG
  std::cout <<"FFaSwitchBoard::removeAllSenderConnections("
            << sender->getLabel() <<"): "<< ourConnections->size() << std::endl;
#endif

  SlotMap::iterator subjIt = sbcIter->second.begin();
  while (subjIt != sbcIter->second.end())
  {
    SlotContainer::iterator typeIt = subjIt->second.begin();
    while (typeIt != subjIt->second.end())
    {
      FFaSlotList::iterator it = typeIt->second.begin();
      while (it != typeIt->second.end())
        it = eraseSlot(sender,subjIt->first,typeIt->second,it);

      SlotContainer::iterator typeToDelete = typeIt++;
      if (typeToDelete->second.empty())
        subjIt->second.erase(typeToDelete);
    }

    SlotMap::iterator subjToDelete = subjIt++;
    if (subjToDelete->second.empty())
      sbcIter->second.erase(subjToDelete);
  }

  if (sbcIter->second.empty())
    ourConnections->erase(sender);
}


void FFaSwitchBoard::removeAllOwnerConnections(FFaSwitchBoardConnector* owner)
{
  if (!ourConnections) return;

#ifdef FFA_DEBUG
  std::cout <<"FFaSwitchBoard::removeAllOwnerConnections("
            << owner->getLabel() <<"): "<< ourConnections->size() << std::endl;
#endif

  SwitchBoardConnection::iterator sbcIter = ourConnections->begin();
  while (sbcIter != ourConnections->end())
  {
    SlotMap::iterator subjIt = sbcIter->second.begin();
    while (subjIt != sbcIter->second.end())
    {
      SlotContainer::iterator tit = subjIt->second.begin();
      while (tit != subjIt->second.end())
      {
        FFaSlotList::iterator it = tit->second.begin();
        while (it != tit->second.end())
          if (!it->slotPt)
          {
            std::cerr <<" *** FFaSwitchBoard::removeAllOwnerConnections("
                      << owner->getLabel() <<"): Ignoring already deleted slot"
                      << std::endl;
            ++it;
          }
          else if (it->slotPt->getObject() == owner)
            it = eraseSlot(sbcIter->first,subjIt->first,tit->second,it);
          else
            ++it;

        SlotContainer::iterator typeToDelete = tit++;
        if (typeToDelete->second.empty())
          subjIt->second.erase(typeToDelete);
      }

      SlotMap::iterator subjToDelete = subjIt++;
      if (subjToDelete->second.empty())
        sbcIter->second.erase(subjToDelete);
    }

    SwitchBoardConnection::iterator sbcToDelete = sbcIter++;
    if (sbcToDelete->second.empty())
      ourConnections->erase(sbcToDelete);
  }
}


void FFaSwitchBoard::removeSlotReference(FFaSwitchBoardConnector* sender,
                                         int subject, FFaSlotBase* slot)
{
  FFaSlotList& ffaSlots = getSlots(sender,subject,slot->getTypeID());
  FFaSlotList::iterator it = ffaSlots.begin();
  while (it != ffaSlots.end())
    if (it->slotPt == slot)
      it = eraseSlot(sender,subject,ffaSlots,it);
    else
      ++it;

  if (ffaSlots.empty())
    cleanUpAfterSlot(sender,subject);
}


FFaSlotList& FFaSwitchBoard::getSlots(FFaSwitchBoardConnector* sender,
                                      int subject, unsigned int typeID)
{
  if (ourConnections)
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
    if (it->refCount < 0 || !it->slotPt)
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
    if (it->refCount >= 0 && it->slotPt)
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
    cleanUpAfterSlot(sender,subject);

#ifdef FFA_DEBUG
  if (nslot > 0)
    std::cout <<"\tNo valid slot "<< islot << std::endl;
#endif
  return it;
}


FFaSwitchBoardConnector::FFaSwitchBoardConnector(const char* s) : label(s)
{
  IAmDeletingMe = false;
#ifdef FFA_DEBUG
  if (label)
    std::cout <<"FFaSwitchBoardConnector("<< label <<")"<< std::endl;
#endif
}


FFaSwitchBoardConnector::~FFaSwitchBoardConnector()
{
#ifdef FFA_DEBUG
  std::cout <<"~FFaSwitchBoardConnector(";
  if (label) std::cout << label;
  std::cout <<"): "<< mySlots.size() << std::endl;
#endif
  IAmDeletingMe = true;
  for (FFaSlotBase* slot : mySlots) delete slot;
  FFaSwitchBoard::removeAllSenderConnections(this);
}


FFaSlotBase::~FFaSlotBase()
{
#ifdef FFA_DEBUG
  std::cout <<"~FFaSlotBase("<< myTypeID <<"): "
            << mySwitchBoardLookups.size() << std::endl;
#endif
  IAmDeletingMe = true;
  for (const SwitchBoardConnectorMap::value_type& swb : mySwitchBoardLookups)
    for (const std::pair<const int,int>& ij : swb.second)
      FFaSwitchBoard::removeSlotReference(swb.first,ij.first,this);
}


bool FFaSlotBase::addConnection(FFaSwitchBoardConnector* sender, int subject)
{
  if (IAmDeletingMe) return false;

  IntMap& lookUp = mySwitchBoardLookups[sender];
  IntMap::iterator it = lookUp.find(subject);
  if (it == lookUp.end())
  {
    lookUp[subject] = 1;
    return true;
  }

  it->second++;
  return false;
}


bool FFaSlotBase::removeConnection(FFaSwitchBoardConnector* sender, int subject)
{
  if (IAmDeletingMe) return false;

#ifdef FFA_DEBUG
  std::cout <<"FFaSlotBase::removeConnection("<< sender->getLabel() <<"): "
            << subject <<" size="<< mySwitchBoardLookups.size() << std::endl;
#endif

  SwitchBoardConnectorMap::iterator cit = mySwitchBoardLookups.find(sender);
  if (cit != mySwitchBoardLookups.end())
  {
    IntMap::iterator it = cit->second.find(subject);
    if (it != cit->second.end() && --(it->second) < 1)
      cit->second.erase(it);

    if (cit->second.empty())
      mySwitchBoardLookups.erase(cit);
  }

  if (!mySwitchBoardLookups.empty()) return false;

#ifdef FFA_DEBUG
  std::cout <<"\tdeleted."<< std::endl;
#endif
  delete this;
  return true;
}
