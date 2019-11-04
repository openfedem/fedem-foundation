// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#ifndef FFA_SWITCH_BOARD_H
#define FFA_SWITCH_BOARD_H

#if defined(win32) || defined(win64)
#pragma warning(disable:4503)
#endif

#include <map>
#include <set>
#include <list>
#include <cstddef>

#include "FFaLib/FFaPatterns/FFaInitialisation.H"

class FFaSlotBase;
class FFaSwitchBoardConnector;


struct FFaSlotBasePt
{
  FFaSlotBasePt(FFaSlotBase* slot) { slotPt = slot; isValid = true; protectedRefCount = 0; }

  FFaSlotBase* slotPt;
  bool isValid;
  int protectedRefCount;
};

typedef std::list<FFaSlotBasePt> FFaSlotList;
typedef std::map<unsigned int,FFaSlotList> TypeIDToSlotBasePtListMap;

typedef void(FFaSwitchBoardConnector::*anyMethodType)(...);
typedef void(*anyFunctionType)(...);


//
// Static switchboard that holds the connection information
//

class FFaSwitchBoard
{
public:
  // External interface
  static void connect(FFaSwitchBoardConnector* sender, int subject, FFaSlotBase* slot);
  static void disConnect(FFaSwitchBoardConnector* sender, int subject, FFaSlotBase* slot);

  // Used internally in SwitchBoard mechanism classes
  static TypeIDToSlotBasePtListMap *getSlots(FFaSwitchBoardConnector * aSender, int aSubject)
    {
      return &(ourConnections->operator[](aSender)[aSubject]);
    }

  static FFaSlotList::iterator nextValidSlot(FFaSlotList::iterator i, FFaSlotList* slots,
					     bool start,  bool &hasPassedEnd,
					     FFaSwitchBoardConnector * aSender,  int aSubject,  unsigned int typeID );
  // Used by FFaSlotBase only
  static void removeSlotReference(FFaSwitchBoardConnector* sender, int subject, FFaSlotBase* slot);

  // Used by FFaSwitchBoardConnector only
  static void removeAllSenderConnections(FFaSwitchBoardConnector* sender);

  static void init();
  static void removeInstance();

private:
  static bool isProtected(FFaSlotList::iterator i);
  static void cleanUpAfterSlot(FFaSwitchBoardConnector * aSender, int aSubject, unsigned int typeID);
  static void eraseSlotFromList(FFaSwitchBoardConnector * aSender, int aSubject, FFaSlotList &slots,
				FFaSlotList::iterator iteratorToSlotToDelete );

  //          map<Sender, map< Subject, SlotTypeId, List of slots  > >
  static std::map<FFaSwitchBoardConnector*, std::map<int,TypeIDToSlotBasePtListMap> >*  ourConnections;

  static std::list<FFaSlotList::iterator>* protectedIterators;
};

static FFaInitialisation<FFaSwitchBoard> FFaSwitchBoard_Init;


//
// Base class for every class that want to send or recieve
//

class FFaSwitchBoardConnector
{
public:
  FFaSwitchBoardConnector() { IAmDeletingMe = false; }
  virtual ~FFaSwitchBoardConnector();

  // Must only be used by FFaSlotN<*>
  void removeSlot(FFaSlotBase* slot)
  {
    if (!IAmDeletingMe)
      mySlots.erase(slot);
  }

  // Must only be used by FFaSlotN<*>
  void addSlot(FFaSlotBase* slot)
  {
    mySlots.insert(slot);
  }

private:
  std::set<FFaSlotBase*, std::less<FFaSlotBase*> > mySlots;
  bool IAmDeletingMe;
};


//
// The most basic slot class
//

class FFaSlotBase
{
  typedef std::map<int,int>                         IntMap;
  typedef std::map<FFaSwitchBoardConnector*,IntMap> SwitchBoardConnectorMap;

public:
  FFaSlotBase() { myTypeID = 0; IAmDeletingMe = false; }
  virtual ~FFaSlotBase();

  // Used internally in SwitchBoard mechanism classes
  virtual bool operator==(FFaSlotBase& otherSlot) = 0;
  unsigned int getTypeID() const { return myTypeID; }

  // These methods must ONLY be used from FFaSwitchBoard
  void addConnection(FFaSwitchBoardConnector* sender, int subject);
  void removeConnection(FFaSwitchBoardConnector* sender, int subject);

  // Should be protected, used only by operator==
  virtual FFaSwitchBoardConnector* getObject() = 0;
  virtual anyMethodType            getMethod() = 0;
  virtual anyFunctionType          getFunction() = 0;
  virtual bool                     isStaticSlot() const = 0;

protected:
  unsigned int myTypeID;
  static int   uniqueTypeId;

private:
  //                      Sender --> (Subject,Count)
  SwitchBoardConnectorMap mySwitchBoardLookups;
  bool                    IAmDeletingMe;
};


//
// Defines for macro arguments
//

// One type :

#define FFASWB_TEMPLATE1 template < class P1Type >
#define FFASWB_ADDED_TEMPLATE_ARGS1 , class P1Type
#define FFASWB_TYPE_LIST1 < P1Type >
#define FFASWB_ADDED_TYPE_LIST1 , P1Type

#define FFASWB_ARGUMENT_LIST1 P1Type p1
#define FFASWB_ADDED_FUNCTION_ARGS1 , P1Type p1

#define FFASWB_PARAM_LIST1 p1

// Two types :

#define FFASWB_TEMPLATE2 template < class P1Type, class P2Type >
#define FFASWB_ADDED_TEMPLATE_ARGS2 , class P1Type, class P2Type
#define FFASWB_TYPE_LIST2 < P1Type, P2Type >
#define FFASWB_ADDED_TYPE_LIST2 , P1Type, P2Type

#define FFASWB_ARGUMENT_LIST2 P1Type p1, P2Type p2
#define FFASWB_ADDED_FUNCTION_ARGS2 , P1Type p1, P2Type p2

#define FFASWB_PARAM_LIST2 p1, p2

// Tree types :

#define FFASWB_TEMPLATE3 template < class P1Type, class P2Type, class P3Type >
#define FFASWB_ADDED_TEMPLATE_ARGS3 , class P1Type, class P2Type, class P3Type
#define FFASWB_TYPE_LIST3 < P1Type, P2Type, P3Type >
#define FFASWB_ADDED_TYPE_LIST3 , P1Type, P2Type, P3Type

#define FFASWB_ARGUMENT_LIST3 P1Type p1, P2Type p2, P3Type p3
#define FFASWB_ADDED_FUNCTION_ARGS3 , P1Type p1, P2Type p2, P3Type p3

#define FFASWB_PARAM_LIST3 p1, p2, p3

// Four types :

#define FFASWB_TEMPLATE4 template < class P1Type, class P2Type, class P3Type, class P4Type >
#define FFASWB_ADDED_TEMPLATE_ARGS4 , class P1Type, class P2Type, class P3Type, class P4Type
#define FFASWB_TYPE_LIST4 < P1Type, P2Type, P3Type, P4Type >
#define FFASWB_ADDED_TYPE_LIST4 , P1Type, P2Type, P3Type, P4Type

#define FFASWB_ARGUMENT_LIST4 P1Type p1, P2Type p2, P3Type p3, P4Type p4
#define FFASWB_ADDED_FUNCTION_ARGS4 , P1Type p1, P2Type p2, P3Type p3, P4Type p4

#define FFASWB_PARAM_LIST4 p1, p2, p3, p4


//
// Macro that defines the different slotBaseN templates.
// One macro call for each number of arguments we want to support.
//

#define FFASWB_MAKE_SLOTBASE_TEMPLATE(nr, templateDef, argumentList, typeList)	\
										\
templateDef									\
class FFaSlotBase##nr : public FFaSlotBase					\
{										\
public:										\
  FFaSlotBase##nr () { myTypeID = getClassTypeID(); }				\
  virtual ~FFaSlotBase##nr () {}						\
  virtual void invoke(argumentList) = 0;					\
  static unsigned int getClassTypeID()						\
  {										\
    static unsigned int ourClassTypeID = ++uniqueTypeId;			\
    return ourClassTypeID;							\
  }										\
};

FFASWB_MAKE_SLOTBASE_TEMPLATE(0, , , )
FFASWB_MAKE_SLOTBASE_TEMPLATE(1, FFASWB_TEMPLATE1, FFASWB_ARGUMENT_LIST1, FFASWB_TYPE_LIST1)
FFASWB_MAKE_SLOTBASE_TEMPLATE(2, FFASWB_TEMPLATE2, FFASWB_ARGUMENT_LIST2, FFASWB_TYPE_LIST2)
FFASWB_MAKE_SLOTBASE_TEMPLATE(3, FFASWB_TEMPLATE3, FFASWB_ARGUMENT_LIST3, FFASWB_TYPE_LIST3)
FFASWB_MAKE_SLOTBASE_TEMPLATE(4, FFASWB_TEMPLATE4, FFASWB_ARGUMENT_LIST4, FFASWB_TYPE_LIST4)


//
// Macro that defines the different slotN templates
//

#define FFASWB_MAKE_SLOT_TEMPLATE(nr, addedTemplateArgs, argumentList, parameterList, typeList)	\
												\
template<class T addedTemplateArgs>								\
class FFaSlot##nr : public FFaSlotBase##nr typeList						\
{												\
  typedef void (T::*methodType)(argumentList);							\
												\
public:												\
  FFaSlot##nr (T* obj, methodType method)							\
  {												\
    myObject = obj;										\
    myMethodPt = method;									\
    myObject->addSlot(this);									\
  }												\
												\
  virtual ~FFaSlot##nr ()									\
  {												\
    myObject->removeSlot(this);									\
  }												\
												\
  virtual void invoke(argumentList)								\
  {												\
    (this->myObject->*this->myMethodPt)(parameterList);						\
  }												\
												\
  virtual bool operator==(FFaSlotBase& otherSlot)						\
  {												\
    return ((FFaSwitchBoardConnector*)this->myObject == otherSlot.getObject() &&		\
            (anyMethodType)this->myMethodPt          == otherSlot.getMethod());			\
  }												\
												\
  virtual FFaSwitchBoardConnector* getObject() { return myObject; }				\
  virtual anyMethodType            getMethod() { return (anyMethodType)myMethodPt; }		\
  virtual anyFunctionType          getFunction() { return (anyFunctionType)NULL; }		\
  virtual bool                     isStaticSlot() const { return false; }			\
												\
private:											\
  T*         myObject;										\
  methodType myMethodPt;									\
};

FFASWB_MAKE_SLOT_TEMPLATE(0, , , , )
FFASWB_MAKE_SLOT_TEMPLATE(1, FFASWB_ADDED_TEMPLATE_ARGS1, FFASWB_ARGUMENT_LIST1, FFASWB_PARAM_LIST1, FFASWB_TYPE_LIST1)
FFASWB_MAKE_SLOT_TEMPLATE(2, FFASWB_ADDED_TEMPLATE_ARGS2, FFASWB_ARGUMENT_LIST2, FFASWB_PARAM_LIST2, FFASWB_TYPE_LIST2)
FFASWB_MAKE_SLOT_TEMPLATE(3, FFASWB_ADDED_TEMPLATE_ARGS3, FFASWB_ARGUMENT_LIST3, FFASWB_PARAM_LIST3, FFASWB_TYPE_LIST3)
FFASWB_MAKE_SLOT_TEMPLATE(4, FFASWB_ADDED_TEMPLATE_ARGS4, FFASWB_ARGUMENT_LIST4, FFASWB_PARAM_LIST4, FFASWB_TYPE_LIST4)


#define FFASWB_MAKE_STATIC_SLOT_TEMPLATE(nr, templateDef, argumentList, parameterList, typeList)	\
													\
templateDef												\
class FFaStaticSlot##nr : public FFaSlotBase##nr typeList						\
{													\
  typedef void (*functionType)(argumentList);								\
													\
public:													\
  FFaStaticSlot##nr (functionType func) { myFunctionPt = func; }					\
  virtual ~FFaStaticSlot##nr () {}									\
													\
  virtual void invoke(argumentList) { (this->myFunctionPt)(parameterList); }				\
													\
  virtual bool operator==(FFaSlotBase& otherSlot)							\
  {													\
    if (otherSlot.isStaticSlot())									\
      return (anyFunctionType)myFunctionPt == otherSlot.getFunction();					\
    else												\
      return false;											\
  }													\
													\
  virtual FFaSwitchBoardConnector* getObject() { return NULL; }						\
  virtual anyMethodType            getMethod() { return (anyMethodType)NULL; }				\
  virtual anyFunctionType          getFunction() { return (anyFunctionType)myFunctionPt; }		\
  virtual bool                     isStaticSlot() const { return true; }				\
													\
private:												\
  functionType myFunctionPt;										\
};

FFASWB_MAKE_STATIC_SLOT_TEMPLATE(0, , , , )
FFASWB_MAKE_STATIC_SLOT_TEMPLATE(1, FFASWB_TEMPLATE1, FFASWB_ARGUMENT_LIST1, FFASWB_PARAM_LIST1, FFASWB_TYPE_LIST1)
FFASWB_MAKE_STATIC_SLOT_TEMPLATE(2, FFASWB_TEMPLATE2, FFASWB_ARGUMENT_LIST2, FFASWB_PARAM_LIST2, FFASWB_TYPE_LIST2)
FFASWB_MAKE_STATIC_SLOT_TEMPLATE(3, FFASWB_TEMPLATE3, FFASWB_ARGUMENT_LIST3, FFASWB_PARAM_LIST3, FFASWB_TYPE_LIST3)
FFASWB_MAKE_STATIC_SLOT_TEMPLATE(4, FFASWB_TEMPLATE4, FFASWB_ARGUMENT_LIST4, FFASWB_PARAM_LIST4, FFASWB_TYPE_LIST4)


/*!
  Function to call all the slots connected to a signal.
  Kind of "emit" from the Qt world.
  Loops trough all the slots connected to this signal and invokes them.
  This is made complicated by the fact that the slots can disappear due
  to the functions actually called.
*/

#define FFASWB_MAKE_EMIT_FUNCTION_TEMPLATE(nr, templateDef, addedArgumentList, parameterList, typeList)	\
													\
templateDef												\
void FFaSwitchBoardCall(FFaSwitchBoardConnector* sender, int subject addedArgumentList)			\
{													\
  bool hasPassedEnd = false;										\
  unsigned int typeID = FFaSlotBase##nr typeList::getClassTypeID();					\
  FFaSlotList& ffaSlots = (*(FFaSwitchBoard::getSlots(sender,subject)))[typeID];			\
  FFaSlotList::iterator i = FFaSwitchBoard::nextValidSlot(ffaSlots.begin(),&ffaSlots,true,hasPassedEnd,sender,subject,typeID); \
  while (!hasPassedEnd) {										\
    ((FFaSlotBase##nr typeList*)(i->slotPt))->invoke(parameterList);					\
    i = FFaSwitchBoard::nextValidSlot(i,&ffaSlots,false,hasPassedEnd,sender,subject,typeID);		\
  }													\
}

FFASWB_MAKE_EMIT_FUNCTION_TEMPLATE(0, inline, , , )
FFASWB_MAKE_EMIT_FUNCTION_TEMPLATE(1, FFASWB_TEMPLATE1, FFASWB_ADDED_FUNCTION_ARGS1, FFASWB_PARAM_LIST1, FFASWB_TYPE_LIST1)
FFASWB_MAKE_EMIT_FUNCTION_TEMPLATE(2, FFASWB_TEMPLATE2, FFASWB_ADDED_FUNCTION_ARGS2, FFASWB_PARAM_LIST2, FFASWB_TYPE_LIST2)
FFASWB_MAKE_EMIT_FUNCTION_TEMPLATE(3, FFASWB_TEMPLATE3, FFASWB_ADDED_FUNCTION_ARGS3, FFASWB_PARAM_LIST3, FFASWB_TYPE_LIST3)
FFASWB_MAKE_EMIT_FUNCTION_TEMPLATE(4, FFASWB_TEMPLATE4, FFASWB_ADDED_FUNCTION_ARGS4, FFASWB_PARAM_LIST4, FFASWB_TYPE_LIST4)


#define FFASWB_MAKE_SIMPLE_CONNECT_FUNCTION_TEMPLATE(nr, addedTemplateArgs, argumentList, addedTypeList)	\
														\
template<class RecieverType addedTemplateArgs>									\
void FFaSwitchBoardConnect(FFaSwitchBoardConnector* sender, int subject, RecieverType* receiver,		\
                           void (RecieverType::*method)(argumentList))						\
{														\
  FFaSwitchBoard::connect(sender, subject, new FFaSlot##nr <RecieverType addedTypeList>(receiver,method));	\
}

FFASWB_MAKE_SIMPLE_CONNECT_FUNCTION_TEMPLATE(0, , , )
FFASWB_MAKE_SIMPLE_CONNECT_FUNCTION_TEMPLATE(1, FFASWB_ADDED_TEMPLATE_ARGS1, FFASWB_ARGUMENT_LIST1, FFASWB_ADDED_TYPE_LIST1)
FFASWB_MAKE_SIMPLE_CONNECT_FUNCTION_TEMPLATE(2, FFASWB_ADDED_TEMPLATE_ARGS2, FFASWB_ARGUMENT_LIST2, FFASWB_ADDED_TYPE_LIST2)
FFASWB_MAKE_SIMPLE_CONNECT_FUNCTION_TEMPLATE(3, FFASWB_ADDED_TEMPLATE_ARGS3, FFASWB_ARGUMENT_LIST3, FFASWB_ADDED_TYPE_LIST3)
FFASWB_MAKE_SIMPLE_CONNECT_FUNCTION_TEMPLATE(4, FFASWB_ADDED_TEMPLATE_ARGS4, FFASWB_ARGUMENT_LIST4, FFASWB_ADDED_TYPE_LIST4)


#define FFASWB_MAKE_SIMPLE_DISCONNECT_FUNCTION_TEMPLATE(nr, addedTemplateArgs, argumentList, addedTypeList)	\
														\
template<class RecieverType addedTemplateArgs>									\
void FFaSwitchBoardDisConnect(FFaSwitchBoardConnector* sender, int subject, RecieverType* receiver,		\
                              void (RecieverType::*method)(argumentList))					\
{														\
  FFaSwitchBoard::disConnect(sender, subject, new FFaSlot##nr <RecieverType addedTypeList>(receiver,method));	\
}

FFASWB_MAKE_SIMPLE_DISCONNECT_FUNCTION_TEMPLATE(0, , , )
FFASWB_MAKE_SIMPLE_DISCONNECT_FUNCTION_TEMPLATE(1, FFASWB_ADDED_TEMPLATE_ARGS1, FFASWB_ARGUMENT_LIST1, FFASWB_ADDED_TYPE_LIST1)
FFASWB_MAKE_SIMPLE_DISCONNECT_FUNCTION_TEMPLATE(2, FFASWB_ADDED_TEMPLATE_ARGS2, FFASWB_ARGUMENT_LIST2, FFASWB_ADDED_TYPE_LIST2)
FFASWB_MAKE_SIMPLE_DISCONNECT_FUNCTION_TEMPLATE(3, FFASWB_ADDED_TEMPLATE_ARGS3, FFASWB_ARGUMENT_LIST3, FFASWB_ADDED_TYPE_LIST3)
FFASWB_MAKE_SIMPLE_DISCONNECT_FUNCTION_TEMPLATE(4, FFASWB_ADDED_TEMPLATE_ARGS4, FFASWB_ARGUMENT_LIST4, FFASWB_ADDED_TYPE_LIST4)


//////////////////////////
// Slot macro interface //
//////////////////////////

#define FFaSlot0S(ownerType,        method)                                 new FFaStaticSlot0 (&ownerType::method)
#define FFaSlot1S(ownerType,        method, P1Type)                         new FFaStaticSlot1<P1Type> (&ownerType::method)
#define FFaSlot2S(ownerType,        method, P1Type, P2Type)                 new FFaStaticSlot2<P1Type, P2Type> (&ownerType::method)
#define FFaSlot3S(ownerType,        method, P1Type, P2Type, P3Type)         new FFaStaticSlot3<P1Type, P2Type, P3Type> (&ownerType::method)
#define FFaSlot4S(ownerType,        method, P1Type, P2Type, P3Type, P4Type) new FFaStaticSlot4<P1Type, P2Type, P3Type, P4Type> (&ownerType::method)

#define FFaSlot0M(ownerType, owner, method)                                 new FFaSlot0<ownerType> (owner,&ownerType::method)
#define FFaSlot1M(ownerType, owner, method, P1Type)                         new FFaSlot1<ownerType, P1Type> (owner,&ownerType::method)
#define FFaSlot2M(ownerType, owner, method, P1Type, P2Type)                 new FFaSlot2<ownerType, P1Type, P2Type> (owner,&ownerType::method)
#define FFaSlot3M(ownerType, owner, method, P1Type, P2Type, P3Type)         new FFaSlot3<ownerType, P1Type, P2Type, P3Type> (owner,&ownerType::method)
#define FFaSlot4M(ownerType, owner, method, P1Type, P2Type, P3Type, P4Type) new FFaSlot4<ownerType, P1Type, P2Type, P3Type, P4Type> (owner,&ownerType::method)

#endif
