// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#include "FFlLib/FFlFEParts/FFlVAppearance.H"
#include "FFaLib/FFaAlgebra/FFaUnitCalculator.H"


FFlVAppearance::FFlVAppearance(int id) : FFlVisualBase(id)
{
  this->addField(color);
  this->addField(shininess);
  this->addField(transparency);
  this->addField(linePattern);
  this->addField(specularColor);
  this->addField(ambientColor);

  // defalut values;
  color = FaVec3(0.5, 0.5, 0.36);
  shininess = 0.8;
  transparency = 0.0;
  linePattern = 0xffff;
  specularColor = FaVec3(1, 1, 1);
  ambientColor  = FaVec3(0, 0, 0);
  runningIdx = -1;
}


FFlVAppearance::FFlVAppearance(const FFlVAppearance& obj) : FFlVisualBase(obj)
{
  this->addField(color);
  this->addField(shininess);
  this->addField(transparency);
  this->addField(linePattern);
  this->addField(specularColor);
  this->addField(ambientColor);

  color = obj.color;
  shininess = obj.shininess;
  transparency = obj.transparency;
  linePattern = obj.linePattern;
  specularColor = obj.specularColor;
  ambientColor = obj.ambientColor;
  runningIdx = -1;
}


FFlVAppearance::~FFlVAppearance()
{
}


void FFlVAppearance::convertUnits(const FFaUnitCalculator*)
{
}


void FFlVAppearance::init()
{
  FFlVAppearanceTypeInfoSpec::instance()->setTypeName("VAPPEARANCE");
  FFlVAppearanceTypeInfoSpec::instance()->setCathegory(FFlTypeInfoSpec::VISUAL_PROP);

  VisualFactory::instance()->registerCreator(FFlVAppearanceTypeInfoSpec::instance()->getTypeName(),
					     FFaDynCB2S(FFlVAppearance::create,int,FFlVisualBase*&));

}
