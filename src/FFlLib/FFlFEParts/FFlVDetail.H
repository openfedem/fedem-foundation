// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#ifndef FFL_VDETAIL_H
#define FFL_VDETAIL_H

#include "FFlLib/FFlVisualBase.H"
#include "FFlLib/FFlTypeInfoSpec.H"
#include "FFlLib/FFlField.H"

#ifdef FF_NAMESPACE
namespace FF_NAMESPACE {
#endif


class FFlVDetail : public FFlVisualBase
{		
public:
  FFlVDetail(int ID);
  FFlVDetail(const FFlVDetail& obj);
  virtual ~FFlVDetail() {}

  static void init();

  FFL_VISUAL_FACTORY_INIT(FFlVDetail);
  FFL_TYPE_INFO(FFlVDetail);

  enum { OFF = 0, ON = 1 };

public:
  FFlField<int> detail;
};

#ifdef FF_NAMESPACE
} // namespace
#endif
#endif
