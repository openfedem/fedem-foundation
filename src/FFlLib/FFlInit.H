// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#ifndef FFL_INIT_H
#define FFL_INIT_H
#define FFL_INIT_ONLY

#include "FFlLib/FFlFEParts/FFlAllFEParts.H"
#include "FFlLib/FFlIOAdaptors/FFlAllIOAdaptors.H"


/*!
  \brief Auxiliary class for initializing and the release of FFlLib singeltons.
  \details This class in mainly used by unit test programs, where we want to
  ensure there are no heap-memory leaks. Simply define an object of this class
  in the beginning of the test program to initialize the singeltons. They will
  then be released when the FFlInit object goes out of scope by its destructor.
*/

class FFlInit
{
public:
  //! \brief The constructor initializes the singelton objects.
  FFlInit()
  {
    FFl::initAllReaders();
    FFl::initAllElements();
  }

  //! \brief The destructor releases the heap-allocated singelton objects.
  ~FFlInit()
  {
    FFl::releaseAllReaders();
    FFl::releaseAllElements();
  }
};

#undef FFL_INIT_ONLY
#undef FFL_ALL_FE_PARTS_H
#undef FFL_ALL_IO_ADAPTORS_H
#endif
