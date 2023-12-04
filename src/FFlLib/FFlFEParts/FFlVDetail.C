// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#include "FFlLib/FFlFEParts/FFlVDetail.H"
#include "FFaLib/FFaAlgebra/FFaUnitCalculator.H"


FFlVDetail::FFlVDetail(int id) : FFlVisualBase(id)
{
  detail = FFlVDetail::ON;
  this->addField(detail);
}


FFlVDetail::FFlVDetail(const FFlVDetail& obj)
  : FFlVisualBase(obj)
{
  this->addField(detail);
  detail = obj.detail;
}


FFlVDetail::~FFlVDetail()
{
}

void FFlVDetail::convertUnits(const FFaUnitCalculator*)
{
}


void FFlVDetail::init()
{
  FFlVDetailTypeInfoSpec::instance()->setTypeName("VDETAIL");
  FFlVDetailTypeInfoSpec::instance()->setCathegory(FFlTypeInfoSpec::VISUAL_PROP);

  VisualFactory::instance()->registerCreator(FFlVDetailTypeInfoSpec::instance()->getTypeName(),
					     FFaDynCB2S(FFlVDetail::create,int,FFlVisualBase*&));

}
