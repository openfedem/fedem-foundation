// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

/*!
  \file FFaInitialisation.H
  \brief Template for static class initialisation on start-up.
*/

#ifndef FFA_INITIALISATION_H
#define FFA_INITIALISATION_H


/*!
  \brief Template for static class initialisation on start-up.
  \details Nifty counter implementation to ensure proper initialisation
  of classes on start-up. The template argument \a T must be a class with
  a static method init() defined.
*/

template<class T> class FFaInitialisation
{
public:
  //! \brief The constructor invokes the T::init() method in the first call.
  FFaInitialisation() { if (++count == 1) T::init(); }
  //! \brief The destructor decrements the counter.
  ~FFaInitialisation() { --count; } // No generic uninitialization, maybe later

private:
  static int count; //!< Keeps track of the number of invokations for class \a T
};

template<class T> int FFaInitialisation<T>::count = 0;

#endif
