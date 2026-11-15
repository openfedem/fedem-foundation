// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

/*!
  \file FiRAOTable_F.C
  \brief Fortran wrapper for the FiRAOTable methods.
  \details This file contains the implementation of the following
  Fortran wrappers of the FiRAOTable module.

  - firaotableinterface::ficonvertwavedata
  - firaotableinterface::fiextractmotion
  - firaotableinterface::fireleasemotion

  No further documentation is provided here.
  The wrappers are documented in the fiRAOTableInterface.f90 file.
*/

#include "FiRAOTable.H"
#include "FiRAOTable.H"
#include "FFaLib/FFaOS/FFaFortran.H"
#include <cstring>


namespace
{
  std::vector<FiWave> myMotion; //!< Vessel motion container of the RAO
}


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
  if (dof < 0 || dof >= static_cast<int>(myMotion.size()))
  {
    --error;
    std::cerr <<" *** FiWxtractMotion: idof="<< dof
              <<" is out of range [0,"<< myMotion.size() <<">."<< std::endl;
    return;
  }

  memcpy(motionData,&myMotion[dof].front(),
         myMotion[dof].size()*3*sizeof(double));
}


SUBROUTINE(fireleasemotion,FIRELEASEMOTION) ()
{
  myMotion.clear();
}
