// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#ifndef FFA_INITIALISATION_H
#define FFA_INITIALISATION_H


/*!
  Nifty counter implementation to ensure initialisation of classes on start-up.
*/

template<class T> class FFaInitialisation
{
public:
  FFaInitialisation() { if (++count == 1) T::init(); }
  ~FFaInitialisation() { --count; } // No generic uninitialization, maybe later

private:
  static int count; 
};

template<class T> int FFaInitialisation<T>::count = 0;

#endif
