// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

/*!
  \file FFaViewItemUtils.C
  \brief Sorting utilities for FFaViewItem objects.
*/

#include <functional>
#include <algorithm>
#include <cctype>

#include "FFaLib/FFaDefinitions/FFaViewItem.H"


namespace
{
  //! \brief Binary comparison of two strings.
  bool stringCompare(const std::string& s1, const std::string& s2)
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

  //! \brief Does the main work for the lexicographical string comparison.
  int compare3way(const std::string& s1, const std::string& s2)
  {
    // Lambda function doing case-insensitive 3-way comparison of 2 characters.
    std::function<int(char,char)> charCmp = [](char c1, char c2) -> int
    {
      int lc1 = tolower(static_cast<unsigned char>(c1));
      int lc2 = tolower(static_cast<unsigned char>(c2));
      return lc1 < lc2 ? -1 : (lc1 > lc2 ? 1 : 0);
    };

    std::pair<std::string::const_iterator,std::string::const_iterator> p;
    p = std::mismatch(s1.begin(), s1.end(), s2.begin(), std::not_fn(charCmp));
    if (p.first == s1.end())
      return p.second == s2.end() ? 0 : -1;

    return charCmp(*p.first,*p.second);
  }

  //! \brief Three-way lexicographical comparison of two strings.
  int stringCompare3w(const std::string& s1, const std::string& s2)
  {
    return s1.size() <= s2.size() ? compare3way(s1,s2) : -compare3way(s2,s1);
  }
}


/*!
  Returns \e true if the description \a *i1 precedes that of *i2.
  Case insensitive comparison.
  If the strings are equal, the ID of the two objects are compared instead.
  If either \a i1 or \a i2 is NULL, return as if the NULL-pointer is first.
*/

bool FFaViewItem::compareDescr(FFaViewItem* i1, FFaViewItem* i2)
{
  if (!(i1 && i2))
    return !i1;

  // Binary compare descriptions.
  // If descriptions are equal, compare by ID, unless both are zero.

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
  Returns \e true if the ID of \a i1 precedes that of \a i2.
  If either \a i1 or \a i2 is NULL, return as if the NULL-pointer is first.
*/

bool FFaViewItem::compareID(FFaViewItem* i1, FFaViewItem* i2)
{
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
  If either \a i1 or \a i2 is NULL, return as if the NULL-pointer is first.
*/

int FFaViewItem::compareDescr3w(FFaViewItem* i1, FFaViewItem* i2)
{
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
  If either \a i1 or \a i2 is NULL, return as if the NULL-pointer is first.
*/

int FFaViewItem::compareID3w(FFaViewItem* i1, FFaViewItem* i2)
{
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
