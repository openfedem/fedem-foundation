/* SPDX-FileCopyrightText: 2023 SAP SE
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * This file is part of FEDEM - https://openfedem.org
 */
/*!
  \file FFaTag_C.h
  \brief C-version of the file-tag read/write functions.
*/

#ifndef FFA_TAG_C_H
#define FFA_TAG_C_H

#include "FFaLib/FFaOS/FFaIO.H"


extern int FFa_writeTag (FT_FILE fd, const char* tag, const int nchar,
                         const unsigned int chksum);

extern int FFa_readTag (FT_FILE fd, char* tag, const int nchar,
                        unsigned int* chksum);

extern int FFa_endian ();

#endif
