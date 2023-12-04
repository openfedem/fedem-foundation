// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#include "FFaFunctionManager.H"
#include "FFaFunctionProperties.H"
#include "FFaLib/FFaAlgebra/FFaVec3.H"
#include "FFaLib/FFaOS/FFaFortran.H"
#include <cstdlib>

#ifdef _NO_FORTRAN
#include <iostream>
#else

DOUBLE_FUNCTION (getfunctionvalue,GETFUNCTIONVALUE) (const int& baseID,
						     const int* intVars,
						     const double* realVars,
						     const int& intNR,
						     const int& realNC,
						     const double& x,
						     int& ierr);

DOUBLE_FUNCTION (getfunctionderiv,GETFUNCTIONDERIV) (const int& baseID,
						     const int* intVars,
						     const double* realVars,
						     const int& intNR,
						     const int& realNC,
						     const double& x,
						     int& ierr);

INTEGER_FUNCTION (getfunctiontypeid,GETFUNCTIONTYPEID) (const char* funcName,
                                                        const int nChar);

SUBROUTINE(initwavefuncfromfile,INITWAVEFUNCFROMFILE) (const char* fileName,
#ifdef _NCHAR_AFTER_CHARARG
						       const int nChar,
#endif
						       const int& nWave,
						       const int& rSeed,
						       double* realVars,
						       int& ierr
#ifndef _NCHAR_AFTER_CHARARG
						     , const int nChar
#endif
						       );

SUBROUTINE(initwavefuncspectrum,INITWAVEFUNCSPECTRUM) (const int& iop,
						       const int& nWave,
						       const int& nDir,
						       const int& sprExp,
						       const int& rSeed,
						       double* realVars,
						       int& ierr);

SUBROUTINE(initnonlinwavefunc,INITNONLINWAVEFUNC) (const int& iop,
						   const double& g,
						   const double& d,
						   double* realVars,
						   int& ierr);

SUBROUTINE(initembeddedwave,INITEMBEDDEDWAVE) (const int& iop,
					       const int& nWave,
					       const int& rSeed,
					       const double& g,
					       const double& d,
					       int* intVars,
					       double* realVars,
					       int& ierr);

DOUBLE_FUNCTION(waveprofile,WAVEPROFILE) (const int& iop, const int& ldi,
					  const int& ldr, const int& nWave,
					  const int& nDir, const int* intVars,
					  const double* realVars,
					  const double& g, const double& d,
					  const double* x, const double& t);

SUBROUTINE(evalwave,EVALWAVE) (const int& iop, const int& ldi,
			       const int& ldr, const int& nWave,
			       const int& nDir, const int* intVars,
			       const double* realVars,
			       const double& g, const double& d,
			       const double* x, const double& t,
			       double& eta, double* v, double* a);

#endif


double FFaFunctionManager::getValue (int baseID,
				     const std::vector<int>& intVars,
				     const std::vector<double>& realVars,
				     const double x, int& ierr)
{
  if (realVars.empty() || intVars.empty())
  {
    ierr = -3;
    return 0.0;
  }

  double value = 0.0;
  ierr = FFaFunctionProperties::getValue(baseID,intVars,realVars,x,value);
  if (ierr <= 0) return value;

#ifdef _NO_FORTRAN
  std::cerr <<" *** FFaFunctionManager::getValue(dummy) "<< baseID
            <<" "<< intVars.size() <<" "<< realVars.size() <<" "<< x
            << std::endl;
  ierr = -99;
  return 0.0;
#else
  return F90_NAME(getfunctionvalue,GETFUNCTIONVALUE) (baseID,
                                                      &intVars.front(),
                                                      &realVars.front(),
                                                      intVars.size(),
                                                      realVars.size(),
                                                      x, ierr);
#endif
}


double FFaFunctionManager::getValue (int baseID, int fType, int extrap,
				     const std::vector<double>& realVars,
				     const double x, int& ierr)
{
  if (realVars.empty())
  {
    ierr = -3;
    return 0.0;
  }

#ifdef _NO_FORTRAN
  std::cerr <<" *** FFaFunctionManager::getValue(dummy) "<< baseID
            <<" "<< fType <<" "<< extrap <<" "<< realVars.size() <<" "<< x
            << std::endl;
  ierr = -99;
  return 0.0;
#else
  // Set intVars[2]=2 for sinusoidals, to indicate that the function parameters
  // are permuted for wave function evaluation, see FmfSinusoidal::initGetValue
  int nComp = fType == 4 ? realVars.size()/3 : (fType == 1 ? 2 : -1);
  int intVars[3] = { fType, extrap, nComp };
  return F90_NAME(getfunctionvalue,GETFUNCTIONVALUE) (baseID, intVars,
                                                      &realVars.front(),
                                                      3, realVars.size(),
                                                      x, ierr);
#endif
}


double FFaFunctionManager::getDerivative (int baseID, int fType, int extrap,
					  const std::vector<double>& realVars,
					  const double x, int& ierr)
{
  if (realVars.empty())
  {
    ierr = -3;
    return 0.0;
  }

#ifdef _NO_FORTRAN
  std::cerr <<" *** FFaFunctionManager::getDerivative(dummy) "<< baseID
            <<" "<< fType <<" "<< extrap <<" "<< realVars.size() <<" "<< x
            << std::endl;
  ierr = -99;
  return 0.0;
#else
  // Set intVars[2]=2 for sinusoidals, to indicate that the function parameters
  // are permuted for wave function evaluation, see FmfSinusoidal::initGetValue
  int nComp = fType == 4 ? realVars.size()/3 : (fType == 1 ? 2 : -1);
  int intVars[3] = { fType, extrap, nComp };
  return F90_NAME(getfunctionderiv,GETFUNCTIONDERIV) (baseID, intVars,
						      &realVars.front(),
						      3, realVars.size(),
						      x, ierr);
#endif
}


int FFaFunctionManager::getTypeID (const std::string& functionType)
{
#ifdef _NO_FORTRAN
  return FFaFunctionProperties::getTypeID(functionType);
#else
  const char* fType = functionType.c_str();
  const int   nChar = functionType.length();
  return F90_NAME(getfunctiontypeid,GETFUNCTIONTYPEID) (fType, nChar);
#endif
}


int FFaFunctionManager::getSmartPoints (int funcType, int extrap,
					const double start, const double stop,
					const std::vector<double>& realVars,
					std::vector<double>& xvec,
					std::vector<double>& yvec)
{
  return FFaFunctionProperties::getSmartPoints (funcType, start, stop,
						extrap, realVars, xvec, yvec);
}


bool FFaFunctionManager::initWaveFunction (const std::string& fName,
					   const int nWave, const int rSeed,
					   std::vector<double>& realVars)
{
  if (nWave < 1) return false;

#ifdef _NO_FORTRAN
  std::cerr <<" *** FFaFunctionManager::initWaveFunction(dummy) "<< fName
            <<" "<< rSeed <<" "<< realVars.size() << std::endl;
  return false;
#else
  int ierr = 0;
  realVars.resize(3*nWave);
  F90_NAME(initwavefuncfromfile,INITWAVEFUNCFROMFILE) (fName.c_str(),
#ifdef _NCHAR_AFTER_CHARARG
						       fName.length(),
#endif
						       nWave, rSeed,
						       &realVars.front(), ierr
#ifndef _NCHAR_AFTER_CHARARG
						     , fName.length()
#endif
						       );
  return ierr < 0 ? false : true;
#endif
}


bool FFaFunctionManager::initWaveFunction (const int iop,
					   const int nWave, const int nDir,
					   const int sprExp, const int rSeed,
					   std::vector<double>& realVars)
{
  if (nWave < 1 || nDir < 1 || nDir%2 == 0) return false;
  if (nDir > 1 && (sprExp%2 == 1 || sprExp < 2)) return false;

#ifdef _NO_FORTRAN
  std::cerr <<" *** FFaFunctionManager::initWaveFunction(dummy) "<< iop
            <<" "<< rSeed <<" "<< realVars.size() << std::endl;
  return false;
#else
  int ierr = 0;
  realVars.resize(3*nWave*nDir);
  F90_NAME(initwavefuncspectrum,INITWAVEFUNCSPECTRUM) (iop, nWave, nDir,
						       sprExp, rSeed,
						       &realVars.front(), ierr);
  return ierr < 0 ? false : true;
#endif
}


bool FFaFunctionManager::initWaveFunction (const int iop,
					   const double g, const double d,
					   std::vector<double>& realVars)
{
  if (realVars.empty())
    return false;

#ifdef _NO_FORTRAN
  std::cerr <<" *** FFaFunctionManager::initWaveFunction(dummy) "<< iop
            <<" "<< g <<" "<< d << std::endl;
  return false;
#else
  int ierr = 0;
  F90_NAME(initnonlinwavefunc,INITNONLINWAVEFUNC) (iop, g, d,
						   &realVars.front(), ierr);
  if (ierr < 0) return false;
  realVars.resize(ierr);
  return true;
#endif
}


bool FFaFunctionManager::initWaveFunction (const int iop,
					   const int nWave, const int rSeed,
					   const double g, const double d,
					   std::vector<int>& intVars,
					   std::vector<double>& realVars)
{
  int ierr = intVars[3];
  if (ierr < 1) return false;

#ifdef _NO_FORTRAN
  std::cerr <<" *** FFaFunctionManager::initWaveFunction(dummy) "<< iop
            <<" "<< nWave <<" "<< rSeed <<" "<< g <<" "<< d
            <<" "<< realVars.size() << std::endl;
  return false;
#else
  intVars.resize(4+ierr,0);
  realVars.resize(3*nWave+50*ierr);
  intVars[4] = realVars.size();
  F90_NAME(initembeddedwave,INITEMBEDDEDWAVE) (iop, nWave, rSeed, g, d,
					       &intVars.front(),
					       &realVars.front(), ierr);
  if (ierr < 0) return false;
  realVars.resize(ierr);
  return true;
#endif
}


double FFaFunctionManager::getWaveValue (const std::vector<double>& realVars,
					 const double g, const double d,
					 const FaVec3& x, const double t,
					 int iop)
{
  std::vector<int> intVars;
  if (iop > 100)
  {
    // Both the function type ID and the user-defined function ID are assumed
    // encoded into iop argument. The latter needs to be stored in intVars.
    intVars.resize(3,0);
    intVars.back() = iop/100;
    iop %= 100;
  }

  return FFaFunctionManager::getWaveValue(intVars,realVars,g,d,x,t,iop);
}


double FFaFunctionManager::getWaveValue (const std::vector<int>& intVars,
					 const std::vector<double>& realVars,
					 const double g, const double d,
					 const FaVec3& x, const double t,
					 int iop)
{
  int ldr = realVars.size();
  int nWave = 1, nDir = 1;
  if (iop < 2 || iop == 4)
  {
    // Airy wave theory, with one or many wave components
    if (intVars.size() > 3 && intVars[3] > 1)
      nDir = intVars[3];
    nWave = ldr / (3*nDir);
    if (nWave < 1) return 0.0;
    ldr = 3;
    iop = d > 0.0 ? 1 : 0;
  }

#ifdef _NO_FORTRAN
  std::cerr <<"  ** FFaFunctionManager::getWaveValue(dummy) "<< g <<" "<< d
            <<" "<< x <<" "<< t <<" "<< iop << std::endl;
  return 0.0;
#else
  return F90_NAME(waveprofile,WAVEPROFILE) (iop, intVars.size(), ldr,
					    nWave, nDir, &intVars.front(),
					    &realVars.front(),
					    g, d, x.getPt(), t);
#endif
}


double FFaFunctionManager::getWaveValue (const std::vector<int>& intVars,
					 const std::vector<double>& realVars,
					 const double g, const double d,
					 const FaVec3& x, const double t,
					 FaVec3& v, FaVec3& a, int iop)
{
  int ldr = realVars.size();
  int nWave = 1, nDir = 1;
  if (iop < 2 || iop == 4)
  {
    // Airy wave theory, with one or many wave components
    if (intVars.size() > 3 && intVars[3] > 1)
      nDir = intVars[3];
    nWave = ldr / (3*nDir);
    if (nWave < 1) return 0.0;
    ldr = 3;
    iop = d > 0.0 ? 1 : 0;
  }

  double eta = 0.0;
#ifdef _NO_FORTRAN
  std::cerr <<"  ** FFaFunctionManager::getWaveValue(dummy) "<< g <<" "<< d
            <<" "<< x <<" "<< t <<" "<< v <<" "<< a <<" "<< iop << std::endl;
#else
  F90_NAME(evalwave,EVALWAVE) (iop, intVars.size(), ldr,
			       nWave, nDir, &intVars.front(), &realVars.front(),
			       g, d, x.getPt(), t, eta, v.getPt(), a.getPt());
#endif
  return eta;
}
