// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#include <functional>
#include <algorithm>
#include <cctype>

#include "FFaLib/FFaDefinitions/FFaViewItem.H"


/*!
  Returns true if i1's description precedes i2's. Case insensitive.
  If strings are equal, sort on id.
*/

bool FFaViewItem::compareDescr(FFaViewItem* i1, FFaViewItem* i2)
{
  // If any of the pointers are NULL, return as if the NULL-pointer is first

  if (!(i1 && i2))
    return !i1;

  // Binary compare descriptions
  // If descriptions are equal, compare by ID, unless both are zero

  std::string s1(i1->getItemDescr());
  std::string s2(i2->getItemDescr());

  if (s1 == s2)
  {
    if (i1->getItemID() == 0 && i2->getItemID() == 0)
      return true;

    return i1->getItemID() < i2->getItemID();
  }

  return stringCompare(s1, s2);
}


/*!
  Returns true if i1's id precedes i2's.
*/

bool FFaViewItem::compareID(FFaViewItem* i1, FFaViewItem* i2)
{
  // If any of the pointers are NULL, return as if the NULL-pointer is first

  if (!(i1 && i2))
    return !i1;

  int n1 = i1->getItemID();
  int n2 = i2->getItemID();

  // Put items with a zero id at the end of the list
  if (n1 == 0) n1 = 2147483647; // maxint
  if (n2 == 0) n2 = 2147483647; // maxint

  if (n1 == n2)
    return stringCompare(i1->getItemDescr(), i2->getItemDescr());

  return n1 < n2;
}


/*!
  Three-way lexicographical comparison of i1 and i2's descriptions.
*/

int FFaViewItem::compareDescr3w(FFaViewItem* i1, FFaViewItem* i2)
{
  // If any of the pointers are NULL, return as if the NULL-pointer is first

  if (!(i1 && i2))
  {
    if (i1 == i2)
      return 0;
    else if (!i1)
      return -1;
    else
      return 1;
  }

  return stringCompare3w(i1->getItemDescr(), i2->getItemDescr());
}


/*!
  Three way comparison of i1 and i2's ids.
*/

int FFaViewItem::compareID3w(FFaViewItem* i1, FFaViewItem* i2)
{
  // If any of the pointers are NULL, return as if the NULL-pointer is first

  if (!(i1 && i2))
  {
    if (i1 == i2)
      return 0;
    else if (!i1)
      return -1;
    else
      return 1;
  }

  int id1 = i1->getItemID();
  int id2 = i2->getItemID();

  if (id1 < id2)
    return -1;
  else if (id1 > id2)
    return 1;
  else
    return 0;
}


/*!
  \brief Does all the work for the lexicographical string comparison.
*/

static int stringCompare3wImpl(const std::string& s1, const std::string& s2)
{
  // Lambda function doing case-insensitive 3-way comparison of two characters.
  std::function<int(char,char)> charCompare = [](char c1, char c2) -> int
  {
    int lc1 = tolower(static_cast<unsigned char>(c1));
    int lc2 = tolower(static_cast<unsigned char>(c2));
    return lc1 < lc2 ? -1 : (lc1 > lc2 ? 1 : 0);
  };

  std::pair<std::string::const_iterator,std::string::const_iterator> p;
  p = std::mismatch(s1.begin(), s1.end(), s2.begin(), std::not_fn(charCompare));
  if (p.first == s1.end())
    return p.second == s2.end() ? 0 : -1;

  return charCompare(*p.first,*p.second);
}


bool FFaViewItem::stringCompare(const std::string& s1, const std::string& s2)
{
  // Lambda function doing case-insensitive comparison of two characters.
  std::function<bool(char,char)> charCompare = [](char c1, char c2) -> bool
  {
    int lc1 = tolower(static_cast<unsigned char>(c1));
    int lc2 = tolower(static_cast<unsigned char>(c2));
    return lc1 < lc2;
  };

  return std::lexicographical_compare(s1.begin(), s1.end(),
                                      s2.begin(), s2.end(),
                                      charCompare);
}


int FFaViewItem::stringCompare3w(const std::string& s1, const std::string& s2)
{
  if (s1.size() <= s2.size())
    return stringCompare3wImpl(s1, s2);
  else
    return -stringCompare3wImpl(s2, s1);
}
