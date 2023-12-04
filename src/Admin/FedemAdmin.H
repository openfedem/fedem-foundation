// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#ifndef _FEDEM_ADMIN_H
#define _FEDEM_ADMIN_H

namespace FedemAdmin
{
  const char* getVersion();
  const char* getBuildDate();
  const char* getBuildYear();
  const char* getCopyrightString();

  int  getDaysSinceBuilt();
  int  getExpireAfter();
  bool is64bit();
}

#endif
