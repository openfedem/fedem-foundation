// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#ifndef FFL_PWAVGM_H
#define FFL_PWAVGM_H

#include "FFlLib/FFlAttributeBase.H"
#include "FFlLib/FFlFEAttributeSpec.H"
#include "FFlLib/FFlTypeInfoSpec.H"
#include "FFlLib/FFlField.H"

#include "FFaLib/FFaPatterns/FFaSingelton.H"

#include <array>


class FFlPWAVGM : public FFlAttributeBase
{
public:
  FFlPWAVGM(int ID);
  FFlPWAVGM(const FFlPWAVGM& obj);
  virtual ~FFlPWAVGM() {}

  virtual bool isIdentic(const FFlAttributeBase* otherAttrib) const;

  static void init();

  FFlAttributeBase* removeWeights(const std::vector<int>& nodes, size_t nNod);

  FFL_FE_ATTRIBUTE_FACTORY_INIT(FFlPWAVGM);
  FFL_FE_ATTRIBUTE_SPEC(FFlPWAVGM);
  FFL_TYPE_INFO(FFlPWAVGM);

  typedef std::vector<double> DoubleVec;

  FFlField<int>               refC; // component numbers at the reference point
  std::array<FFlField<int>,6> indC; // indices into the weightMatrix
  FFlField<DoubleVec> weightMatrix; // master DOF weighting factors
};


typedef FFaSingelton<FFlTypeInfoSpec,FFlPWAVGM>    FFlPWAVGMTypeInfoSpec;
typedef FFaSingelton<FFlFEAttributeSpec,FFlPWAVGM> FFlPWAVGMAttributeSpec;

#endif
