// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

/*!
  \file FFrSuperObjectGroup.H
  \brief Class representing top level entries in a result file hierarchy.
*/

#ifndef FFR_SUPER_OBJECT_GROUP_H
#define FFR_SUPER_OBJECT_GROUP_H

#include "FFrLib/FFrFieldEntryBase.H"


/*!
  \brief Class representing top level entries in a result file hierarchy.
*/

class FFrSuperObjectGroup : public FFrFieldEntryBase
{
public:
  //! \brief The constructor inserts its \a name in the dictionary.
  FFrSuperObjectGroup(const std::string& name, std::set<std::string>& dict);
  //! \brief Empty destructor.
  virtual ~FFrSuperObjectGroup();

  //! \brief Returns the type name with additonal '(s)' formatting.
  virtual const std::string& getDescription() const;
  //! \brief Returns the type name without any modifications.
  virtual const std::string& getType() const { return *typeIt; }

  //! \brief Returns \e true since this entry is as Super Object Group.
  virtual bool isSOG() const { return true; }

private:
  //! Points to the type name of this entry, which is kept in a dictionary
  std::set<std::string>::const_iterator typeIt;

#if FFR_DEBUG > 2
  static long int count; //!< Internal counter, for debug usage
#endif
};

#endif
