// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

/*!
  \file FiDeviceFunctionFactory_F.C
  \brief Fortran wrapper for the FiDeviceFunctionFactory methods.
  \details This file contains the implementation of the following
  Fortran wrappers of the FiDeviceFunctionFactory module.

  - fidevicefunctioninterface::fidf_open
  - fidevicefunctioninterface::fidf_openwrite
  - fidevicefunctioninterface::fidf_close
  - fidevicefunctioninterface::fidf_closeall
  - fidevicefunctioninterface::fidf_setvalue
  - fidevicefunctioninterface::fidf_setfrequency
  - fidevicefunctioninterface::fidf_setstep
  - fidevicefunctioninterface::fidf_getxaxis
  - fidevicefunctioninterface::fidf_getyaxis
  - fidevicefunctioninterface::fidf_setxaxis
  - fidevicefunctioninterface::fidf_setyaxis
  - fidevicefunctioninterface::fidf_dump
  - fidevicefunctioninterface::fidf_extfunc
  - fidevicefunctioninterface::fidf_extfunc_ff
  - fidevicefunctioninterface::fidf_storeextfunc
  - fidevicefunctioninterface::fidf_initextfunc

  No further documentation is provided here.
  The wrappers are documented in the fiDeviceFunctionInterface.f90 file.
*/

#include "FiDeviceFunctions/FiDeviceFunctionFactory.H"
#include "FiDeviceFunctions/FiDeviceFunctionBase.H"
#include "FFaLib/FFaOS/FFaFortran.H"
#include <cstring>

//! \cond DO_NOT_DOCUMENT
#define FIDF FiDeviceFunctionFactory::instance()
//! \endcond


////////////////////////////////////////////////////////////////////////////////

SUBROUTINE(fidf_open,FIDF_OPEN) (const char* name,
#ifdef _NCHAR_AFTER_CHARARG
                                 const int nchar, int& fileIndex, int& error
#else
                                 int& fileIndex, int& error, const int nchar
#endif
){
  fileIndex = FIDF->open(std::string(name,nchar),
                         UNKNOWN_FILE,IO_READ,fileIndex);
  error = fileIndex < 0 ? fileIndex : 0;
}


////////////////////////////////////////////////////////////////////////////////

SUBROUTINE(fidf_openwrite,FIDF_OPENWRITE) (const char* name,
#ifdef _NCHAR_AFTER_CHARARG
                                           const int nchar, const int& fileType,
                                           int& fileIndex, int& error
#else
                                           const int& fileType, int& fileIndex,
                                           int& error, const int nchar
#endif
){
  // Set output endian formatting for the device depending on platform
#if defined(win32) || defined(win64)
  bool littleEndian = true;
#else
  bool littleEndian = false;
#endif

  fileIndex = FIDF->open(std::string(name,nchar),
                         (FiDevFormat)fileType,IO_WRITE,littleEndian);
  error = fileIndex < 0 ? fileIndex : 0;
}


////////////////////////////////////////////////////////////////////////////////

SUBROUTINE(fidf_close,FIDF_CLOSE) (const int& fileIndex)
{
  FIDF->close(fileIndex);
}


////////////////////////////////////////////////////////////////////////////////

SUBROUTINE(fidf_closeall,FIDF_CLOSEALL) ()
{
  FiDeviceFunctionFactory::removeInstance();
}


////////////////////////////////////////////////////////////////////////////////

DOUBLE_FUNCTION(fidf_getvalue,FIDF_GETVALUE) (const int& fileIndex,
                                              const double& arg, int& err,
                                              const int& channel,
                                              const int& zeroAdjust,
                                              const double& vertShift,
                                              const double& scale)
{
  return FIDF->getValue(fileIndex,arg,err,channel,zeroAdjust,vertShift,scale);
}


////////////////////////////////////////////////////////////////////////////////

SUBROUTINE(fidf_setvalue,FIDF_SETVALUE) (const int& fileIndex,
                                         const double& first,
                                         const double& second)
{
  FIDF->setValue(fileIndex,first,second);
}


////////////////////////////////////////////////////////////////////////////////

SUBROUTINE(fidf_setfrequency,FIDF_SETFREQUENCY) (const int& fileIndex,
                                                 const double& freq)
{
  FIDF->setFrequency(fileIndex,freq);
}


////////////////////////////////////////////////////////////////////////////////

SUBROUTINE(fidf_setstep,FIDF_SETSTEP) (const int& fileIndex, const double& step)
{
  FIDF->setStep(fileIndex,step);
}


////////////////////////////////////////////////////////////////////////////////

namespace
{
  //! \brief Pad character string with traling spaces when passing to Fortran.
  void padWhiteSpace(char* str, const int nchar)
  {
    if (int nc = strlen(str); nc < nchar)
      memset(str+nc,' ',nchar-nc);
  }
}

SUBROUTINE(fidf_getxaxis,FIDF_GETXAXIS) (const int& fileIndex, char* title,
#ifdef _NCHAR_AFTER_CHARARG
                                         const int ncharT, char* unit,
#else
                                         char* unit, const int ncharT,
#endif
                                         const int ncharU)
{
  FIDF->getAxisTitle(fileIndex,FiDeviceFunctionBase::X,title,ncharT);
  padWhiteSpace(title,ncharT);

  FIDF->getAxisUnit(fileIndex,FiDeviceFunctionBase::X,unit,ncharU);
  padWhiteSpace(unit,ncharU);
}

SUBROUTINE(fidf_getyaxis,FIDF_GETYAXIS) (const int& fileIndex, char* title,
#ifdef _NCHAR_AFTER_CHARARG
                                         const int ncharT, char* unit,
#else
                                         char* unit, const int ncharT,
#endif
                                         const int ncharU)
{
  FIDF->getAxisTitle(fileIndex,FiDeviceFunctionBase::Y,title,ncharT);
  padWhiteSpace(title,ncharT);

  FIDF->getAxisUnit (fileIndex,FiDeviceFunctionBase::Y,unit ,ncharU);
  padWhiteSpace(unit,ncharU);
}


////////////////////////////////////////////////////////////////////////////////

SUBROUTINE(fidf_setxaxis,FIDF_SETXAXIS) (const int& fileIndex,
                                         const char* title,
#ifdef _NCHAR_AFTER_CHARARG
                                         const int ncharT, const char* unit,
#else
                                         const char* unit, const int ncharT,
#endif
                                         const int ncharU)
{
  std::string t(title,ncharT), u(unit,ncharU);
  FIDF->setAxisTitle(fileIndex,FiDeviceFunctionBase::X,t.c_str());
  FIDF->setAxisUnit (fileIndex,FiDeviceFunctionBase::X,u.c_str());
}

SUBROUTINE(fidf_setyaxis,FIDF_SETYAXIS) (const int& fileIndex,
                                         const char* title,
#ifdef _NCHAR_AFTER_CHARARG
                                         const int ncharT, const char* unit,
#else
                                         const char* unit, const int ncharT,
#endif
                                         const int ncharU)
{
  std::string t(title,ncharT), u(unit,ncharU);
  FIDF->setAxisTitle(fileIndex,FiDeviceFunctionBase::Y,t.c_str());
  FIDF->setAxisUnit (fileIndex,FiDeviceFunctionBase::Y,u.c_str());
}


////////////////////////////////////////////////////////////////////////////////

SUBROUTINE(fidf_dump,FIDF_DUMP) ()
{
  FIDF->dump();
}


////////////////////////////////////////////////////////////////////////////////

SUBROUTINE(fidf_extfunc,FIDF_EXTFUNC) (int& error, char* fname,
#ifdef _NCHAR_AFTER_CHARARG
                                       const int ncharF, char* label,
#else
                                       char* label, const int ncharF,
#endif
                                       const int ncharL)
{
  std::string fileName(fname,ncharF), channels(label,ncharL);
  error = FIDF->initExtFuncFromFile(fileName,channels) ? 0 : -1;
}


////////////////////////////////////////////////////////////////////////////////

SUBROUTINE(fidf_extfunc_ff,FIDF_EXTFUNC_FF) (const int& nstep)
{
  FIDF->updateExtFuncFromFile(nstep,false);
}


////////////////////////////////////////////////////////////////////////////////

SUBROUTINE(fidf_storeextfunc,FIDF_STOREEXTFUNC) (double* data, const int& ndat,
                                                 const int& iop, int& istat)
{
  int offset = istat-1;
  if (iop == 0)
    offset = FiDeviceFunctionFactory::myNumExtFun;
  else
    FIDF->storeExtFuncValues(data,ndat,iop,offset);
  istat = iop == 0 || offset < 0 ? offset : offset+1;
}


////////////////////////////////////////////////////////////////////////////////

SUBROUTINE(fidf_initextfunc,FIDF_INITEXTFUNC) (double* data, const int& ndat,
                                               int& istat)
{
  istat = 0;
  if (ndat > 0)
    FIDF->storeExtFuncValues(data,ndat,1,istat);
  else
    FIDF->updateExtFuncFromFile(1);
}
