// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#include "FFrLib/FFrSuperObjectGroup.H"

#if FFR_DEBUG > 2
long int FFrSuperObjectGroup::count = 0;
#endif


FFrSuperObjectGroup::FFrSuperObjectGroup(const std::string& name,
                                         std::set<std::string>& dict)
{
  typeIt = dict.insert(name).first;
#if FFR_DEBUG > 2
  std::cout <<"Creating superobject group #"<< (myCount = ++count)
            <<": "<< this->getDescription() << std::endl;
#endif
}


FFrSuperObjectGroup::~FFrSuperObjectGroup()
{
#if FFR_DEBUG > 2
  std::cout <<"Destroying superobject group #"<< myCount
            <<": "<< this->getDescription() << std::endl;
#endif
}


const std::string& FFrSuperObjectGroup::getDescription() const
{
  static std::string descr;
  descr = *typeIt + "(s)";
  return descr;
}
