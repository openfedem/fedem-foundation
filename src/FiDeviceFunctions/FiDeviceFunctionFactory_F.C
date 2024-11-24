// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

/*!
  \file FiDeviceFunctionFactory_F.C
  \brief Fortran wrapper for the FiDeviceFunctionFactory methods.
*/

#include "FiDeviceFunctions/FiDeviceFunctionFactory.H"
#include "FiDeviceFunctions/FiDeviceFunctionBase.H"
#include "FFaLib/FFaOS/FFaFortran.H"
#include <cstring>

//! \cond DO_NOT_DOCUMENT
#define FIDF FiDeviceFunctionFactory::instance()
//! \endcond

////////////////////////////////////////////////////////////////////////////////
//! \brief Opens a function device for reading.
//!
//! \arg name      - Name of the file to open. The file extension
//!                  is used to try to determine the file type.
//!                  If this does not succeed, ASCII is assumed.
//! \arg fileIndex - Unique index for the opened file, used in other calls.
//!                  A negative value indicates error.
//!
//! \note On input, the \a fileIndex argument contains the channel index.
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
//! \brief Opens a function device for writing.
//!
//! \arg name      - Name of the file to open
//! \arg fileType  - Type of the file to open. If UNKNOWN_FILE, the file
//!                  extension is used to try to determine the actual file type.
//!                  If this does not succeed, a simple text file is opened.
//! \arg fileIndex - Unique index for the opened file, used in other calls.
//!                  A negative value indicates error.
//!
//! \note Existing files are replaced.
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
//! \brief Closes the file associated with the \a fileIndex.
////////////////////////////////////////////////////////////////////////////////

SUBROUTINE(fidf_close,FIDF_CLOSE) (const int& fileIndex)
{
  FIDF->close(fileIndex);
}


////////////////////////////////////////////////////////////////////////////////
//! \brief Closes all open files.
////////////////////////////////////////////////////////////////////////////////

SUBROUTINE(fidf_closeall,FIDF_CLOSEALL) ()
{
  FiDeviceFunctionFactory::removeInstance();
}


////////////////////////////////////////////////////////////////////////////////
//! \brief Returns a value from the device.
//!
//! \arg fileIndex - Associated file index
//! \arg arg       - Argument to the function to evaluate
//! \arg err       - Error flag, on input it specifies the integration order
//! \arg channel   - Channel index
//! \return Corresponding value, i.e., f(arg)
//!
//! \note Only valid for read-only devices.
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
//! \brief Sets a value pair to the device.
//!
//! \arg fileIndex - Associated file index
//! \arg first     - First value (key) in the value pair
//! \arg second    - Second value in the value pair
//!
//! \note Only valid for write-only devices.
////////////////////////////////////////////////////////////////////////////////

SUBROUTINE(fidf_setvalue,FIDF_SETVALUE) (const int& fileIndex,
                                         const double& first,
                                         const double& second)
{
  FIDF->setValue(fileIndex,first,second);
}


////////////////////////////////////////////////////////////////////////////////
//! \brief Defines the sampling frequency for the device.
//!
//! \arg fileIndex - Associated file index
//! \arg freq      - Desired sampling frequency
//!
//! \note Only valid for write-only devices.
////////////////////////////////////////////////////////////////////////////////

SUBROUTINE(fidf_setfrequency,FIDF_SETFREQUENCY) (const int& fileIndex,
                                                 const double& freq)
{
  FIDF->setFrequency(fileIndex,freq);
}


////////////////////////////////////////////////////////////////////////////////
//! \brief Defines the sampling step size for the device.
//!
//! \arg fileIndex - Associated file index
//! \arg step      - Desired sampling step size
//!
//! \note Only valid for write-only devices.
////////////////////////////////////////////////////////////////////////////////

SUBROUTINE(fidf_setstep,FIDF_SETSTEP) (const int& fileIndex, const double& step)
{
  FIDF->setStep(fileIndex,step);
}


////////////////////////////////////////////////////////////////////////////////
//! \brief Returns X-axis info for the device.
//!
//! \arg fileIndex - Associated file index
//! \arg title     - Description of the axis (output)
//! \arg unit      - The axis unit (output)
////////////////////////////////////////////////////////////////////////////////

SUBROUTINE(fidf_getxaxis,FIDF_GETXAXIS) (const int& fileIndex, char* title,
#ifdef _NCHAR_AFTER_CHARARG
                                         const int ncharT, char* unit,
#else
                                         char* unit, const int ncharT,
#endif
                                         const int ncharU)
{
  FIDF->getAxisTitle(fileIndex,FiDeviceFunctionBase::X,title,ncharT);
  int nc = strlen(title);
  if (nc < ncharT) memset(title+nc,' ',ncharT-nc);

  FIDF->getAxisUnit(fileIndex,FiDeviceFunctionBase::X,unit,ncharU);
  nc = strlen(unit);
  if (nc < ncharU) memset(unit+nc,' ',ncharU-nc);
}

////////////////////////////////////////////////////////////////////////////////
//! \brief Returns Y-axis info for the device.
//!
//! \arg fileIndex - Associated file index
//! \arg title     - Description of the axis (output)
//! \arg unit      - The axis unit (output)
////////////////////////////////////////////////////////////////////////////////

SUBROUTINE(fidf_getyaxis,FIDF_GETYAXIS) (const int& fileIndex, char* title,
#ifdef _NCHAR_AFTER_CHARARG
                                         const int ncharT, char* unit,
#else
                                         char* unit, const int ncharT,
#endif
                                         const int ncharU)
{
  FIDF->getAxisTitle(fileIndex,FiDeviceFunctionBase::Y,title,ncharT);
  FIDF->getAxisUnit (fileIndex,FiDeviceFunctionBase::Y,unit ,ncharU);
  int nt = strlen(title), nu = strlen(unit);
  if (nt < ncharT) memset(title+nt,' ',ncharT-nt);
  if (nu < ncharU) memset(unit +nu,' ',ncharU-nu);
}


////////////////////////////////////////////////////////////////////////////////
//! \brief Sets X-axis info for the device.
//!
//! \arg fileIndex - Associated file index
//! \arg title     - Description of the axis
//! \arg unit      - The axis unit
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

////////////////////////////////////////////////////////////////////////////////
//! \brief Sets Y-axis info for the device.
//!
//! \arg fileIndex - Associated file index
//! \arg title     - Description of the axis
//! \arg unit      - The axis unit
////////////////////////////////////////////////////////////////////////////////

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
//! \brief Dumps data about current device functions.
////////////////////////////////////////////////////////////////////////////////

SUBROUTINE(fidf_dump,FIDF_DUMP) ()
{
  FIDF->dump();
}


////////////////////////////////////////////////////////////////////////////////
//! \brief Opens a file for reading external function values.
//!
//! \arg error - Error flag
//! \arg fname - File name
//! \arg label - Labels of file columns to use
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
//! \brief Updates the external function values from file.
//!
//! \arg nstep - Number of steps to read
////////////////////////////////////////////////////////////////////////////////

SUBROUTINE(fidf_extfunc_ff,FIDF_EXTFUNC_FF) (const int& nstep)
{
  FIDF->updateExtFuncFromFile(nstep);
}


////////////////////////////////////////////////////////////////////////////////
//! \brief Transfers external function values to/from state array.
//!
//! \arg data - The state array
//! \arg ndat - Length of the state array
//! \arg iop - 0: Return size in \a istat, 1: store values, 2: restore values
//! \arg istat - Running state array index, output negative on error
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
//! \brief Initializes external function values either from file or state array.
//!
//! \arg data - The state array
//! \arg ndat - Length of the state array
//! \arg istat - Negative value on error, otherwise equals \a ndat
////////////////////////////////////////////////////////////////////////////////

SUBROUTINE(fidf_initextfunc,FIDF_INITEXTFUNC) (double* data, const int& ndat,
                                               int& istat)
{
  istat = 0;
  if (!FIDF->updateExtFuncFromFile(1) && ndat > 0)
    FIDF->storeExtFuncValues(data,ndat,1,istat);
}
