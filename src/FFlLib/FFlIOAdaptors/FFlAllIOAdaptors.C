// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#include "FFlAllIOAdaptors.H"
#include "FFlReaders.H"

//! Set to \e true when initialized, to avoid initializing more than once.
static bool initialized = false;


void FFl::initAllReaders()
{
  if (initialized) return;

  FFlFedemReader::init();
  FFlOldFLMReader::init();
  FFlNastranReader::init();
  FFlSesamReader::init();
  FFlAbaqusReader::init();
  FFlAnsysReader::init();

  initialized = true;
}


void FFl::releaseAllReaders()
{
  FFlReaders::removeInstance ();

  initialized = false;
}
