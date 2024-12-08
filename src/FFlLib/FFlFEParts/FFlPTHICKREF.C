// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#include "FFlPTHICKREF.H"

#ifdef FF_NAMESPACE
namespace FF_NAMESPACE {
#endif


FFlPTHICKREF::FFlPTHICKREF(int id) : FFlAttributeBase(id)
{
  this->addField(factor);
  factor = 0.5;
}


FFlPTHICKREF::FFlPTHICKREF(const FFlPTHICKREF& obj) : FFlAttributeBase(obj)
{
  this->addField(factor);
  factor = obj.factor;
}


void FFlPTHICKREF::init()
{
  using TypeInfoSpec  = FFaSingelton<FFlTypeInfoSpec,FFlPTHICKREF>;
  using AttributeSpec = FFaSingelton<FFlFEAttributeSpec,FFlPTHICKREF>;

  TypeInfoSpec::instance()->setTypeName("PTHICKREF");
  TypeInfoSpec::instance()->setDescription("Strain coat heights");
  TypeInfoSpec::instance()->setCathegory(FFlTypeInfoSpec::STRC_PROP);

  AttributeSpec::instance()->addLegalAttribute("PTHICK",true);

  AttributeFactory::instance()->registerCreator
    (TypeInfoSpec::instance()->getTypeName(),
     FFaDynCB2S(FFlPTHICKREF::create,int,FFlAttributeBase*&));
}

#ifdef FF_NAMESPACE
} // namespace
#endif
