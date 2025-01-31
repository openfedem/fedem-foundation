// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#ifndef FFL_VAPPEARANCE_H
#define FFL_VAPPEARANCE_H

#include "FFlLib/FFlVisualBase.H"
#include "FFlLib/FFlTypeInfoSpec.H"
#include "FFlLib/FFlField.H"
#include "FFaLib/FFaAlgebra/FFaVec3.H"

#ifdef FF_NAMESPACE
namespace FF_NAMESPACE {
#endif


class FFlVAppearance : public FFlVisualBase
{		
public:
  FFlVAppearance(int ID);
  FFlVAppearance(const FFlVAppearance& obj);
  virtual ~FFlVAppearance() {}

  static void init();

  FFL_VISUAL_FACTORY_INIT(FFlVAppearance);
  FFL_TYPE_INFO(FFlVAppearance);

public:
  FFlField<FaVec3> color;
  FFlField<double>  shininess;
  FFlField<double>  transparency;
  FFlField<unsigned short> linePattern;
  FFlField<FaVec3> specularColor;
  FFlField<FaVec3> ambientColor;

  // A temporary index telling the running index
  // of this appearance in the appearance vector in the link handler.
  // Used by the groupPart generation.
  int runningIdx;
};

#ifdef FF_NAMESPACE
} // namespace
#endif
#endif
