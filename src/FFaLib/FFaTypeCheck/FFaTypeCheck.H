// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

/*!
  \file FFaTypeCheck.H
  \brief Type checking control.
  \details This file contains a set of macros to be used in class definitions,
  to enable simple type checking in class hierarcies.
*/

#ifndef FFA_TYPE_CHECK_H
#define FFA_TYPE_CHECK_H


namespace FFaTypeCheck //! Type checking control
{
  //! \brief Returns a unique type ID number for \a typeName.
  int getNewTypeID(const char* typeName);
  //! \brief Returns the type ID number for \a typeName, if it exists.
  int getTypeIDFromName(const char* typeName);
  //! \brief Returns the type name associated with the specified \a typeID.
  const char* getTypeNameFromID(int typeID);

  constexpr int NO_TYPE_ID = -1; //!< Value for undefined type ID number
}


//! \brief Macro enabling type checking for a class.
//! \details Place this in top of the class definition, e.g.:
//! \code
//!   class someClass
//!   {
//!     Fmd_HEADER_INIT();
//! \endcode
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

//! \brief Macro enabling type checking for a top-level class.
//! \details Place this in top of the class definition, e.g.:
//! \code
//!   class myTopClass
//!   {
//!     Fmd_BASE_HEADER_INIT();
//! \endcode
#define Fmd_BASE_HEADER_INIT()			\
protected:					\
  int         itsTypeID;			\
  const char* itsTypeIDName;			\
Fmd_HEADER_INIT()

//! \brief Macro with type checking initializations.
//! \details Place this in the start of your constructor implementation, e.g.:
//! \code
//!   someClassName::someClassName()
//!   {
//!     Fmd_CONSTRUCTOR_INIT(someClassName);
//! \endcode
#define Fmd_CONSTRUCTOR_INIT(clType)		\
 itsTypeID     = clType::getClassTypeID();	\
 itsTypeIDName = clType::getClassTypeIDName()

//! \brief Macro with type checking implementations.
//! \details Place this somewhere in global scope, preferably at the top of the
//! implementation file of the class, e.g.:
//! \code
//!   Fmd_SOURCE_INIT(classTypeIDName, someClassName, parentClassName);
//! \endcode
#define Fmd_SOURCE_INIT(clName, clType, prClType)		\
								\
int         clType::typeID = FFaTypeCheck::getNewTypeID(#clName);\
const char* clType::typeIDName = #clName;			\
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
    return prClType::staticIsOfType(tp);			\
}								\
								\
bool clType::staticIsOfType(int tp)				\
{								\
  if (typeID == tp)						\
    return true;						\
  else if (typeID == prClType::getClassTypeID())		\
    return false;						\
  else								\
    return prClType::staticIsOfType(tp);			\
}

#endif
