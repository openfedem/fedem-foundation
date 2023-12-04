// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#ifndef FFA_BYTE_ARRAY_H
#define FFA_BYTE_ARRAY_H

#include <vector>
#include <iostream>


using FFaByteArray = std::vector<char>;

std::ostream& operator<< (std::ostream& s, const FFaByteArray& array);
std::istream& operator>> (std::istream& s, FFaByteArray& array);

#endif
