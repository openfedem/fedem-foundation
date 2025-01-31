// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#include "FFlLib/FFlFEParts/FFlVDetail.H"

#ifdef FF_NAMESPACE
namespace FF_NAMESPACE {
#endif


FFlVDetail::FFlVDetail(int id) : FFlVisualBase(id)
{
  detail = FFlVDetail::ON;
  this->addField(detail);
}


FFlVDetail::FFlVDetail(const FFlVDetail& obj) : FFlVisualBase(obj)
{
  this->addField(detail);
  detail = obj.detail;
}


void FFlVDetail::init()
{
  using TypeInfoSpec = FFaSingelton<FFlTypeInfoSpec,FFlVDetail>;

  TypeInfoSpec::instance()->setTypeName("VDETAIL");
  TypeInfoSpec::instance()->setCathegory(FFlTypeInfoSpec::VISUAL_PROP);

  VisualFactory::instance()->registerCreator(TypeInfoSpec::instance()->getTypeName(),
                                             FFaDynCB2S(FFlVDetail::create,int,FFlVisualBase*&));
}

#ifdef FF_NAMESPACE
} // namespace
#endif
