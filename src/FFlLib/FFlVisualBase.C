// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#include "FFlLib/FFlVisualBase.H"
#include "FFlLib/FFlFieldBase.H"
#include "FFlLib/FFlTypeInfoSpec.H"
#include "FFaLib/FFaAlgebra/FFaCheckSum.H"


const std::string& FFlVisualBase::getTypeName() const
{
  return this->getTypeInfoSpec()->getTypeName();
}


void FFlVisualBase::calculateChecksum(FFaCheckSum* cs, int csMask) const
{
  this->checksum(cs,csMask);
  for (FFlFieldBase* field : myFields)
    field->calculateChecksum(cs);
}
