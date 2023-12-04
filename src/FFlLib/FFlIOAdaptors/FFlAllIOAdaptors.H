// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#ifndef FFL_ALL_IO_ADAPTORS_H
#define FFL_ALL_IO_ADAPTORS_H
#ifndef FFL_INIT_ONLY

#include "FFlLib/FFlIOAdaptors/FFlFedemReader.H"
#include "FFlLib/FFlIOAdaptors/FFlOldFLMReader.H"
#include "FFlLib/FFlIOAdaptors/FFlNastranReader.H"
#include "FFlLib/FFlIOAdaptors/FFlSesamReader.H"
#include "FFlLib/FFlIOAdaptors/FFlVdmReader.H"

#endif


namespace FFl
{
  //! This function initializes the link reader singleton objects.
  void initAllReaders();
  //! This function releases heap-allocated memory related to singleton objects.
  void releaseAllReaders();
}

#endif
