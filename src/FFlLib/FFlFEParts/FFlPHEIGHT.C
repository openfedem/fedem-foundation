// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#include "FFlLib/FFlFEParts/FFlPHEIGHT.H"
#include "FFaLib/FFaAlgebra/FFaUnitCalculator.H"

#ifdef FF_NAMESPACE
namespace FF_NAMESPACE {
#endif


FFlPHEIGHT::FFlPHEIGHT(int id) : FFlAttributeBase(id)
{
  this->addField(height);
  height = 0.1;
}


FFlPHEIGHT::FFlPHEIGHT(const FFlPHEIGHT& obj)
  : FFlAttributeBase(obj)
{
  this->addField(height);
  height = obj.height;
}


void FFlPHEIGHT::convertUnits(const FFaUnitCalculator* convCal)
{
  convCal->convert(height.data(),"LENGTH");
}


void FFlPHEIGHT::init()
{
  using TypeInfoSpec = FFaSingelton<FFlTypeInfoSpec,FFlPHEIGHT>;

  TypeInfoSpec::instance()->setTypeName("PHEIGHT");
  TypeInfoSpec::instance()->setDescription("Strain coat heights");
  TypeInfoSpec::instance()->setCathegory(FFlTypeInfoSpec::GEOMETRY_PROP);

  AttributeFactory::instance()->registerCreator
    (TypeInfoSpec::instance()->getTypeName(),
     FFaDynCB2S(FFlPHEIGHT::create,int,FFlAttributeBase*&));
}

#ifdef FF_NAMESPACE
} // namespace
#endif
