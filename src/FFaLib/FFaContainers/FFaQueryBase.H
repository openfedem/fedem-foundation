// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#ifndef FFA_QUERY_BASE_H
#define FFA_QUERY_BASE_H


/*!
  \brief Base class for all DB objects and fields.
*/

class FFaQueryBase
{
protected:
  FFaQueryBase() : next(0) {}

public:
  virtual ~FFaQueryBase() {}

  FFaQueryBase* next;
};

#endif
