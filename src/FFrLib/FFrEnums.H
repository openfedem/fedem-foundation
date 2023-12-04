// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#ifndef FFR_ENUMS_H
#define FFR_ENUMS_H

enum FFrStatus
{
  FAILED = -1,
  DONE = 0,
  LABEL_SEARCH,
  LABEL_IGNORE,
  LABEL_READ,
  LABEL_VALID,
  LABEL_ERROR,
  FOUND_HEADING,
  FOUND_VARIABLES,
  FOUND_DATABLOCKS,
  FOUND_DATA
};

#endif
