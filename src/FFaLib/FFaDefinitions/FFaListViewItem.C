// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#include "FFaLib/FFaDefinitions/FFaListViewItem.H"


bool FFaListViewItem::setPositionInListView(const char* lvName, int pos)
{
  if (!lvName) return false;

  FFaPosExpMap::iterator i;
  if (!this->posExpInfo)
    this->posExpInfo = new FFaPosExpMap();
  else if ((i = this->posExpInfo->find(lvName)) != this->posExpInfo->end())
  {
    i->second.first = pos;
    return true;
  }

  (*this->posExpInfo)[lvName] = FFaPosExpPair(pos,false);
  return false;
}


int FFaListViewItem::getPositionInListView(const char* lvName) const
{
  if (!this->posExpInfo) return false;

  FFaPosExpMap::const_iterator i = this->posExpInfo->find(lvName);
  if (i != this->posExpInfo->end())
    return i->second.first;
  else
    return -1;
}


bool FFaListViewItem::setExpandedInListView(const char* lvName, bool exp)
{
  if (!lvName) return false;

  FFaPosExpMap::iterator i;
  if (!this->posExpInfo)
    this->posExpInfo = new FFaPosExpMap();
  else if ((i = this->posExpInfo->find(lvName)) != this->posExpInfo->end())
  {
    i->second.second = exp;
    return true;
  }

  (*this->posExpInfo)[lvName] = FFaPosExpPair(-1,exp);
  return false;
}


bool FFaListViewItem::getExpandedInListView(const char* lvName) const
{
  if (!this->posExpInfo) return false;

  FFaPosExpMap::const_iterator i = this->posExpInfo->find(lvName);
  if (i != this->posExpInfo->end())
    return i->second.second;
  else
    return false;
}
