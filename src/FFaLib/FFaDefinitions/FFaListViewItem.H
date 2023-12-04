// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#ifndef FFA_LIST_VIEW_ITEM_H
#define FFA_LIST_VIEW_ITEM_H

#include "FFaLib/FFaDefinitions/FFaViewItem.H"
#include <map>


/*!
  \brief Base class for list view items.
  \details Every item that should live inside a list view must be an
  FFaListViewItem.
  The item can have an explicit list view position,
  but if it doesn't the list view will sort on item IDs,
  and if that doesn't exist either,
  the items will be positioned in the order of occurence.
*/

class FFaListViewItem : public FFaViewItem
{
public:
  //! \brief Default constructor.
  FFaListViewItem() { posExpInfo = NULL; }
  //! \brief The destructor deletes the position and expand status mapping.
  virtual ~FFaListViewItem() { delete posExpInfo; }

  //! \brief Returns the pixmap of this item.
  virtual const char** getListViewPixmap() const { return NULL; }

  //! \brief Sets the position of this item in the named list view.
  bool setPositionInListView(const char* lvName, int pos);
  //! \brief Returns the position of this item in the named list view.
  int  getPositionInListView(const char* lvName) const;

  //! \brief Sets the expand status of this item in the named list view.
  bool setExpandedInListView(const char* lvName, bool exp);
  //! \brief Returns the expand status of this item in the named list view.
  bool getExpandedInListView(const char* lvName) const;

  //! \brief Returns whether this item is positioned in the named list view.
  bool isSubjectToLVPositioning(const char* lvName) const
  { return this->getPositionInListView(lvName) > -1 || this->getItemID() > 0; }

private:
  //! Position in list view and whether the item is expanded or not
  typedef std::pair<int,bool>                 FFaPosExpPair;
  //! List view name to position and whether the item is expanded mapping
  typedef std::map<char const*,FFaPosExpPair> FFaPosExpMap;

  FFaPosExpMap* posExpInfo; //!< Position and expand status mapping
};

#endif
