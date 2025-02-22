// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#include "FFlLib/FFlIOAdaptors/FFlAllIOAdaptors.H"
#include "FFlLib/FFlIOAdaptors/FFlReaders.H"

#ifdef FF_NAMESPACE
using namespace FF_NAMESPACE;
#endif

//! Set to \e true when initialized, to avoid initializing more than once.
static bool initialized = false;


void FFl::initAllReaders()
{
  if (initialized) return;

  FFlFedemReader::init();
  FFlOldFLMReader::init();
  FFlNastranReader::init();
  FFlSesamReader::init();
#ifdef FT_HAS_VKI
  FFlAbaqusReader::init();
  FFlAnsysReader::init();
#endif

  initialized = true;
}


void FFl::releaseAllReaders()
{
  FFlReaders::removeInstance();

  initialized = false;
}
