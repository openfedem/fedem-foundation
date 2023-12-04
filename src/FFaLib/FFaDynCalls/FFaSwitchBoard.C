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
typedef std::list<std::list<FFaSlotBasePt>::iterator >                                SwitchBoartIteratorT;

SwitchBoardConnectionT* FFaSwitchBoard::ourConnections = 0;
SwitchBoartIteratorT*   FFaSwitchBoard::protectedIterators = 0;


void FFaSwitchBoard::init()
{
  FFaSwitchBoard::ourConnections     = new SwitchBoardConnectionT();
  FFaSwitchBoard::protectedIterators = new SwitchBoartIteratorT();
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


void  FFaSwitchBoard::disConnect(FFaSwitchBoardConnector * aSender, int aSubject, FFaSlotBase *aSlot)
{
  std::list< FFaSlotBasePt>::iterator i;
  std::list< FFaSlotBasePt>::iterator toDelete; 
  std::list< FFaSlotBasePt>* ffaSlots;
  unsigned int typeID;


  typeID = aSlot->getTypeID();
  aSlot->addConnection(aSender,aSubject); // increment to avoid accidental removal during looping if aSlot is a working slot
  ffaSlots = &(*FFaSwitchBoard::getSlots(aSender, aSubject))[typeID];

  i = ffaSlots->begin();                                                                                    
  while(i != ffaSlots->end())                                                                               
   {                                                                                                    
     if (  (*aSlot) == (*((*i).slotPt))  )                
       {                                                                                                
	 toDelete = i;                                                                                  
	 i++;
	 FFaSwitchBoard::eraseSlotFromList(aSender, aSubject, (*ffaSlots), toDelete); 
       }                                                                                                
     else                                                                                               
       { i++; }                                                                                                
   } 

  FFaSwitchBoard::cleanUpAfterSlot(aSender,aSubject,typeID);
  aSlot->removeConnection(aSender,aSubject); // decrement to make it disappear if no longer used.
}


void
FFaSwitchBoard::removeAllSenderConnections(FFaSwitchBoardConnector* aSender)
{
  std::map<int , TypeIDToSlotBasePtListMap>::iterator subjIt;
  std::map<int , TypeIDToSlotBasePtListMap>::iterator subjToDelete;

  TypeIDToSlotBasePtListMap::iterator typeIt;
  TypeIDToSlotBasePtListMap::iterator typeToDelete;

  std::list<FFaSlotBasePt>::iterator slotIt;
  std::list<FFaSlotBasePt>::iterator slotToDelete;

  subjIt = ourConnections->operator[](aSender).begin();
  while (subjIt != ourConnections->operator[](aSender).end())
    {
      typeIt = (*subjIt).second.begin();
      while (  typeIt != (*subjIt).second.end())
	{
	  slotIt = (*typeIt).second.begin();
	  while ( slotIt != (*typeIt).second.end())
	    {
	      slotToDelete = slotIt;
	      slotIt++;
	      FFaSwitchBoard::eraseSlotFromList(aSender, (*subjIt).first, ((*typeIt).second), slotToDelete); 
	    }

	  typeToDelete = typeIt;
	  typeIt ++; 
	  if ( (*typeToDelete).second.empty() )
	    {
	      (*subjIt).second.erase(typeToDelete);
	    }
	}
      subjToDelete = subjIt;
      subjIt ++;
      if ( (*subjToDelete).second.empty() )
	{
	  ourConnections->operator[](aSender).erase(subjToDelete);
	}
     }

  if(ourConnections->operator[](aSender).empty()) 
    {
      ourConnections->erase(aSender);
    }
}


void FFaSwitchBoard::removeSlotReference(FFaSwitchBoardConnector * aSender, int aSubject, FFaSlotBase *aSlot)
{
  std::list<FFaSlotBasePt>::iterator slotIt;
  std::list<FFaSlotBasePt>::iterator toDelete;  
  std::list<FFaSlotBasePt> *ffaSlots;

  ffaSlots = &((*(FFaSwitchBoard::getSlots(aSender, aSubject)))[aSlot->getTypeID()]);

  slotIt = ffaSlots->begin();
  while ( slotIt != ffaSlots->end())
    {
      if ((*slotIt).slotPt == aSlot)
	{
	  toDelete = slotIt;
	  slotIt++;

	  FFaSwitchBoard::eraseSlotFromList(aSender,aSubject,(*ffaSlots),toDelete); 
	}
      else
	{ slotIt++; }
    }
     
  FFaSwitchBoard::cleanUpAfterSlot(aSender,aSubject,aSlot->getTypeID());
}

/*!
  Function that finds the next slot that is available and valid for invoking.
  Used when a signal is emitted using the FFsSwitchBoardCall function to
  iterate over the slots that can be deleted as we go.
*/

std::list<FFaSlotBasePt>::iterator
FFaSwitchBoard::nextValidSlot( std::list<FFaSlotBasePt>::iterator i, std::list<
    FFaSlotBasePt> *ffaSlots, bool start, bool &hasPassedEnd,
    FFaSwitchBoardConnector * aSender, int aSubject, unsigned int typeID )
{
  std::list<FFaSlotBasePt>::iterator result;
  std::list<FFaSlotBasePt>::iterator toDelete;

  if ( start )
  {
    i = ffaSlots->begin();
  }
  else
  {
    ( *i ).protectedRefCount--; // VS 2008 port FFaSwitchBoard::protectedIterators->pop_front();
    if  ( (*i).protectedRefCount < 0 )
    {
      std::cout << "Error: FFaSwitchboardcall::protectedRefcount negative"
            << std::endl;
    }
    if( !(*i).isValid )
    {
      toDelete = i;
      ++i;
      ffaSlots->erase(toDelete);
    }
    else
    {
      ++i;
    }
  }

  while (i != ffaSlots->end() && !(*i).isValid)
  {
    i++;
  }

  if (i != ffaSlots->end() && (*i).isValid)
  {
  (*i).protectedRefCount++;// VS 2008 port FFaSwitchBoard::protectedIterators->push_front(i);
    }
  else
  {
    hasPassedEnd = true;
    FFaSwitchBoard::cleanUpAfterSlot(aSender,aSubject,typeID);
  }

  return i;
}


void FFaSwitchBoard::eraseSlotFromList(FFaSwitchBoardConnector * aSender, 
				       int aSubject, 
				       std::list<FFaSlotBasePt> &ffaSlots, 
				       std::list<FFaSlotBasePt>::iterator iteratorToSlotToDelete )
{
  if((*iteratorToSlotToDelete).isValid) 
    {
      ((*iteratorToSlotToDelete).slotPt)->removeConnection(aSender,aSubject);

      if(!FFaSwitchBoard::isProtected(iteratorToSlotToDelete))
	ffaSlots.erase(iteratorToSlotToDelete);
      else
	(*iteratorToSlotToDelete).isValid = false;
    }
} 

 
bool FFaSwitchBoard::isProtected( std::list<FFaSlotBasePt>::iterator i)
{
  return ((*i).protectedRefCount > 0);
  /* VS 2008 port
  std::list< std::list<FFaSlotBasePt>::iterator >::iterator result;
  
  result = find(protectedIterators->begin(), protectedIterators->end(), i);
  
  if (result == protectedIterators->end())
    return false;
  else
    return true;
    */
}


void FFaSwitchBoard::cleanUpAfterSlot(FFaSwitchBoardConnector * aSender, int aSubject, unsigned int typeID)
{
  if ( (((ourConnections->operator[](aSender))[aSubject])[typeID]).empty() )
    {
      ((ourConnections->operator[](aSender))[aSubject]).erase(typeID);

      if ( ((ourConnections->operator[](aSender))[aSubject]).empty() )
	{
	  (ourConnections->operator[](aSender)).erase(aSubject);

	  if( (ourConnections->operator[](aSender)).empty())
	    {
	      ourConnections->erase(aSender);
	    }
	}
    }	
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
    for (const std::pair<int,int>& ij : swb.second)
      FFaSwitchBoard::removeSlotReference(swb.first,ij.first,this);
}
