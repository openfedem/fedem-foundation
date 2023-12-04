// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#ifndef FFP_DFT_PARAMS_H
#define FFP_DFT_PARAMS_H


struct DFTparams
{
  enum { MAGNITUDE, PHASE } resultType = MAGNITUDE;
  bool entiredomain = false;
  bool resample = false;
  bool zeroAdjustX = false;
  bool zeroAdjustY = false;
  bool removeComp = false;
  double startDomain = 0.0;
  double endDomain = 0.0;
  double resampleRate = 0.0;
  double offsetX = 0.0;
  double scaleX = 0.0;
  double offsetY = 0.0;
  double scaleY = 0.0;
};

#endif
