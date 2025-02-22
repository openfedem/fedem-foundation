// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#ifndef FFL_PSTRC_H
#define FFL_PSTRC_H

#include "FFlLib/FFlAttributeBase.H"
#include "FFlLib/FFlFEAttributeSpec.H"
#include "FFlLib/FFlTypeInfoSpec.H"
#include "FFlLib/FFlField.H"

#ifdef FF_NAMESPACE
namespace FF_NAMESPACE {
#endif


class FFlPSTRC : public FFlAttributeBase
{		
public:
  FFlPSTRC(int ID);
  FFlPSTRC(const FFlPSTRC& obj);
  virtual ~FFlPSTRC() {}

  static void init();

  FFL_FE_ATTRIBUTE_FACTORY_INIT(FFlPSTRC);
  FFL_FE_ATTRIBUTE_SPEC(FFlPSTRC);
  FFL_TYPE_INFO(FFlPSTRC);

  FFlField<std::string> name;
};

#ifdef FF_NAMESPACE
} // namespace
#endif
#endif
