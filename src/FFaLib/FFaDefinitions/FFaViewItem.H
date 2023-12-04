// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#ifndef FFA_VIEW_ITEM_H
#define FFA_VIEW_ITEM_H

#include <string>
#include <vector>


/*!
  \brief Abstract interface class for list view items.
  \details It also contains some static helpers for sorting view items.
  The description sorting is case insensitive.
*/

class FFaViewItem
{
public:
  //! \brief Default constructor.
  FFaViewItem() = default;
  //! \brief Disable default copy constructor.
  FFaViewItem(const FFaViewItem&) = delete;
  //! \brief Empty destructor.
  virtual ~FFaViewItem() {}
  //! \brief Disable default assignment operator.
  FFaViewItem& operator=(const FFaViewItem&) = delete;
  //! \brief Returns the name of this item.
  virtual char const* getItemName() const = 0;
  //! \brief Returns the description of this item.
  virtual std::string getItemDescr() const = 0;
  //! \brief Returns the user ID of this item.
  virtual int         getItemID() const = 0;
  //! \brief Returns the base ID of this item.
  virtual int         getItemBaseID() const = 0;
  //! \brief Returns the sub-assembly ID path of this item.
  virtual void        getItemAssemblyPath(std::vector<int>&) const {}

  //! \brief Binary comparison of description strings (implementation of less).
  static bool compareDescr(FFaViewItem* i1, FFaViewItem* i2);
  //! \brief Binary comparison of user IDs (implementation of less).
  static bool compareID(FFaViewItem* i1, FFaViewItem* i2);

  //! \brief Three-way lexicographical comparison of description strings.
  static int compareDescr3w(FFaViewItem* i1, FFaViewItem* i2);
  //! \brief Three-way lexicographical comparison of user IDs.
  static int compareID3w(FFaViewItem* i1, FFaViewItem* i2);

private:
  //! \brief Binary comparison of two strings.
  static bool stringCompare(const std::string& s1, const std::string& s2);
  //! \brief Three-way lexicographical comparison of two strings.
  static int stringCompare3w(const std::string& s1, const std::string& s2);
};

#endif
