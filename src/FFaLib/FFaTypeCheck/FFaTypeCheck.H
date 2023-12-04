// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#ifndef FFA_TYPE_CHECK_H
#define FFA_TYPE_CHECK_H


/*!
  \brief Type checking control class.
*/

class FFaTypeCheck
{
public:
  static int getNewTypeID(const char* typeName);
  static int getTypeIDFromName(const char* typeName);
  static const char* getTypeNameFromID(int typeID);

  enum { NO_TYPE_ID = -1 };

private:
  static int counter;
};


///////////////////////////////////////////////////////////
//
// Macros to simplify type checking in classes :
//
// Place this in top of the class definition, e.g.:
//
//    class someClass
//    {
//      Fmd_HEADER_INIT();

#define Fmd_HEADER_INIT()			\
public:						\
  virtual bool        isOfType(int tp) const;	\
  virtual int         getTypeID() const;	\
  virtual const char* getTypeIDName() const;	\
						\
  static  const char* getClassTypeIDName();	\
  static  int         getClassTypeID();		\
  static  int         getParentClassTypeID();	\
						\
protected:					\
  static bool staticIsOfType(int tp);		\
						\
  static int         typeID;			\
  static const char* typeIDName

// Use this in top classes, instead of Fmd_HEADER_INIT()

#define Fmd_BASE_HEADER_INIT()			\
protected:					\
  int         itsTypeID;			\
  const char* itsTypeIDName;			\
Fmd_HEADER_INIT()

// Place this in the start of your constructor implementation, e.g.:
//
// Fmd_CONSTRUCTOR_INIT(someClassName);

#define Fmd_CONSTRUCTOR_INIT(clType)		\
 itsTypeID     = clType::getClassTypeID();	\
 itsTypeIDName = clType::getClassTypeIDName()

// Place this somewhere in global scope, preferably at the top of the
// implementation file of the class, e.g.:
//
// Fmd_SOURCE_INIT(clName, clType, prClType);

#define Fmd_SOURCE_INIT(clName, clType, prClType)		\
								\
int         clType::typeID = FFaTypeCheck::getNewTypeID(FM__QUOTE(clName)); \
const char* clType::typeIDName = FM__QUOTE(clName);		\
								\
int clType::getTypeID() const					\
{								\
  return itsTypeID;						\
}								\
								\
const char* clType::getTypeIDName() const			\
{								\
  return itsTypeIDName;						\
}								\
								\
int clType::getClassTypeID()					\
{								\
  return typeID;						\
}								\
								\
const char* clType::getClassTypeIDName()			\
{								\
  return typeIDName;						\
}								\
								\
int clType::getParentClassTypeID()				\
{								\
  return prClType::getClassTypeID();				\
}								\
								\
bool clType::isOfType(int tp) const				\
{								\
  if (itsTypeID == tp)						\
    return true;						\
  else								\
    return prClType::staticIsOfType(tp); 			\
}								\
								\
bool clType::staticIsOfType(int tp)				\
{								\
  if (typeID == tp)						\
    return true; 						\
  else if (typeID == prClType::getClassTypeID())		\
    return false;						\
  else								\
    return prClType::staticIsOfType(tp);			\
}


////////////////////////////////////////////////////////////
//
// Some FmDB specific macros that uses the type checking.
//
// These are used when making new Mechanism Database objects,
// instead of the plain type checking macros :

#define Fmd_DB_HEADER_INIT()							\
										\
Fmd_HEADER_INIT();								\
public:										\
  static bool parentParse(const char* keyWord, std::istream& activeStatement, void* obj); \
  bool cloneInherited(FmBase* obj, int depth)

#define Fmd_DB_SOURCE_INIT(clName, clType, prClType)				\
										\
bool clType::cloneInherited(FmBase* obj, int depth)				\
{										\
  bool status = true;								\
  if (clType::typeID != prClType::getClassTypeID())				\
    status = this->prClType::cloneInherited(obj,depth);				\
										\
  return this->clType::cloneLocal(obj,depth) && status;				\
}										\
										\
bool clType::parentParse(const char* keyWord, std::istream& activeStatement, void* obj) \
{ 										\
  return prClType::localParse(keyWord,activeStatement,static_cast<prClType*>(obj)); \
}										\
										\
Fmd_SOURCE_INIT(clName, clType, prClType)


#define FM__QUOTE(str) #str

#endif
