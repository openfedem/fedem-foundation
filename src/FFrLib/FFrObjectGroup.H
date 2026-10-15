// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

/*!
  \file FFrObjectGroup.H
  \brief Class representing mechanism objects in a result file hierarchy.
*/

#ifndef FFR_OBJECT_GROUP_H
#define FFR_OBJECT_GROUP_H

#include "FFrLib/FFrFieldEntryBase.H"
#include "FFrLib/FFrEnums.H"
#include <cstdio>

class FFrCreatorData;


/*!
  \brief Class representing mechanism objects in a result file hierarchy.
  \details An object group is a result database entry that corresponds to
  a mechanism object (Triad, Part, Spring, etc.) in a Fedem model.
  The result quantities associated with the object are then represented by
  item group and variable reference entries as data fields,
  held by the parent class FFrFieldEntryBase.
*/

class FFrObjectGroup : public FFrFieldEntryBase
{
public:
  //! \brief The default constructor initializes its data members.
  FFrObjectGroup();
  //! \brief Empty destructor.
  virtual ~FFrObjectGroup();

  //! \brief Static method for parsing an object group definition from file.
  static FFrStatus create(FILE* varStream, FFrCreatorData& cd, bool dataBlocks);

  //! \brief Parses the object group definition from a list of strings.
  int fillObject(const std::vector<std::string>& tokens, FFrCreatorData& cd);

  virtual int traverse(FFrResultContainer* resultContainer,
		       FFrEntryBase* owner,
		       FFrEntryBase*& objToBeModified,
		       int binPos);

  //! \brief Returns the \ref description of this object group.
  virtual const std::string& getDescription() const { return description; }
  virtual bool hasDescription() const { return !description.empty(); }

  //! \brief Returns the type name of the object represented by this entry.
  virtual const std::string& getType() const { return *typeIt; }

  //! \brief Returns the \ref baseId for this entry.
  virtual int getBaseID() const  { return baseId; }
  virtual bool hasBaseID() const { return baseId != 0; }

  //! \brief Returns the \ref userId for this entry.
  virtual int getUserID() const  { return userId; }
  virtual bool hasUserID() const { return userId != 0; }

  //! \brief Returns \e true since this entry is an Object Group.
  virtual bool isOG() const { return true; }

private:
  int baseId; //!< The base ID of the object represented by this object group
  int userId; //!< The user ID of the object represented by this object group

  //! Points to the type name of this entry, which is kept in a dictionary
  std::set<std::string>::const_iterator typeIt;
  //! User-defined description of the object represented by this object group
  std::string description;

#if FFR_DEBUG > 2
  static long int count; //!< Internal counter, for debug usage
#endif
};

#endif
