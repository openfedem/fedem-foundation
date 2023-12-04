// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#include "FiRAOTable.H"
#include "FFaLib/FFaOS/FFaFortran.H"

static std::vector<FiWave> myMotion;


SUBROUTINE(ficonvertwavedata,FICONVERTWAVEDATA) (const char* name,
#ifdef _NCHAR_AFTER_CHARARG
                                                 const int nchar,
#endif
                                                 const int& dir,
                                                 const int& nRw,
                                                 const int& nComp,
                                                 const double* waveData,
#ifdef _NCHAR_AFTER_CHARARG
                                                 int& error
#else
                                                 int& error, const int nchar
#endif
){
  if (!FiRAOTable::applyRAO(std::string(name,nchar),
                            dir,nRw,nComp,waveData,myMotion)) error -= 2;
}


SUBROUTINE(fiextractmotion,FIEXTRACTMOTION) (const int& dof,
                                             double* motionData, int& error)
{
  if (!FiRAOTable::extractMotion(myMotion,dof,motionData)) error--;
}


SUBROUTINE(fireleasemotion,FIRELEASEMOTION) ()
{
  myMotion.clear();
}
