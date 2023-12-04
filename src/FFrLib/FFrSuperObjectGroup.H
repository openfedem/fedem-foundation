// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#ifndef FFR_SUPER_OBJECT_GROUP_H
#define FFR_SUPER_OBJECT_GROUP_H

#include "FFrLib/FFrFieldEntryBase.H"


class FFrSuperObjectGroup : public FFrFieldEntryBase
{
public:
  FFrSuperObjectGroup(const std::string& name, std::set<std::string>& dict);
  virtual ~FFrSuperObjectGroup();

  //! Returns the type name with additonal '(s)' formatting.
  virtual const std::string& getDescription() const;
  //! Returns the type name without any modifications.
  virtual const std::string& getType() const { return *typeIt; }

  virtual bool isSOG() const { return true; }

private:
  std::set<std::string>::const_iterator typeIt;

#if FFR_DEBUG > 2
  static long int count;
#endif
};

#endif
