// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#include "FiDeviceFunctionBase.H"
#include <cmath>
#include <cstdio>
#include <cstring>
#if FI_DEBUG > 2
#include <iostream>
#endif


FiDeviceFunctionBase::Endianness FiDeviceFunctionBase::myMachineEndian =
#if defined(win32) || defined(win64)
FiDeviceFunctionBase::LittleEndian;
#else
FiDeviceFunctionBase::BigEndian;
#endif


FiDeviceFunctionBase::FiDeviceFunctionBase(const char* filename)
{
  myFile = 0;
  myRefCount = 0;
  myFileStatus = Not_Loaded;
  myInterpolationPolicy = Linear;
  myExtrapolationPolicy = Constant;
  myStep = -1.0;
  myInputEndian = myOutputEndian = BigEndian;
  myDatasetDevice = filename;
}


int FiDeviceFunctionBase::unref()
{
  if (--myRefCount > 0) return myRefCount;

  this->close();
  delete this;
  return 0;
}


bool FiDeviceFunctionBase::open(const char* fname, FileStatus status)
{
  if (myRefCount > 0) return true;

  if (fname) myDatasetDevice = fname;
  if (myDatasetDevice.empty()) return false;

  switch (status)
  {
    case Read_Only:
      if ((myFile = FT_open(myDatasetDevice.c_str(),FT_rb)))
        if (this->initialDeviceRead())
          myFileStatus = Read_Only;
      break;
    case Write_Only:
      if ((myFile = FT_open(myDatasetDevice.c_str(),FT_wb)))
        if (this->preliminaryDeviceWrite())
          myFileStatus = Write_Only;
      break;
    default:
      myFile = 0;
  }

  if (!myFile)
    perror(myDatasetDevice.c_str());
  else if (myFileStatus <= Not_Open)
  {
    FT_close(myFile);
    myFile = 0;
  }
  else
    this->ref();

  return myFile ? true : false;
}


void FiDeviceFunctionBase::getAxisUnit(int axis, char* uText, size_t n) const
{
  std::map<int,axisInfo>::const_iterator it = myAxisInfo.find(axis);
  if (it != myAxisInfo.end())
    strncpy(uText,it->second.unit.c_str(),n);
  else
    uText[0] = '\0';
}

void FiDeviceFunctionBase::getAxisTitle(int axis, char* aText, size_t n) const
{
  std::map<int,axisInfo>::const_iterator it = myAxisInfo.find(axis);
  if (it != myAxisInfo.end())
    strncpy(aText,it->second.title.c_str(),n);
  else
    aText[0] = '\0';
}


void FiDeviceFunctionBase::setAxisUnit(int axis, const char* unitText)
{
  myAxisInfo[axis].unit = unitText;
}

void FiDeviceFunctionBase::setAxisTitle(int axis, const char* titleText)
{
  myAxisInfo[axis].title = titleText;
}


bool FiDeviceFunctionBase::writeString(const char* str)
{
  return FT_write(str,strlen(str),1,myFile) == 1;
}

bool FiDeviceFunctionBase::writeString(const char* lab, const std::string& val)
{
  return (FT_write(lab,strlen(lab),1,myFile) +
          FT_write(val.c_str(),val.size(),1,myFile)) == 2;
}


bool FiDeviceFunctionBase::close(bool noHeader)
{
  bool ok = false;
  if (myFileStatus == Write_Only)
    ok = this->concludingDeviceWrite(noHeader);
  else if (myFileStatus >= Not_Open)
    ok = true;

  myFileStatus = ok ? Not_Open : Not_Loaded;
  if (myFile) FT_close(myFile);
  myFile = 0;
  return ok;
}


double FiDeviceFunctionBase::interpolate(double x,
					 double x0, double f0,
					 double x1, double f1) const
{
  switch (myInterpolationPolicy)
  {
    case Linear:
      if (x1 > x0)
        return f0 + (x-x0) * ((f1-f0) / (x1-x0));
    case Previous_Value:
      return f0;
    default: // Next_Value
      return f1;
  }
}


double FiDeviceFunctionBase::extrapolate(double x,
					 double x0, double f0,
					 double x1, double f1) const
{
  switch (myExtrapolationPolicy)
  {
    case Linear:
      if (x1 > x0)
        return f0 + (x-x0) * ((f1-f0) / (x1-x0));
    default: // Constant
      return x <= x0 ? f0 : f1;
  }
}


double FiDeviceFunctionBase::integrate(double x, int order, int channel,
				       double vertShift, double scaleFac)
{
  // The zero-order integral is the function value itself
  if (order < 1)
    return this->getValue(x,channel,false,vertShift,scaleFac);

  const double eps = 1.0e-16;
  if (fabs(x) < eps) return 0.0;

  // Fetch the curve point data
  if (Xval.empty()) this->getRawData(Xval,Yval,0.0,-1.0,channel);
  long int nVal = Xval.size();

  // Special treatment of single-point functions (constant)
  if (nVal < 1)
    return 0.0;
  else if (nVal == 1)
    return vertShift + scaleFac*Yval.front() * (order == 1 ? x : 0.5*x*x);

  // Find the start interval and the function value at x=0.0
  long int i0, i1, i2 = 0;
  double f0, f1;
  if (Xval.front() >= 0.0)
  {
    // All points are in positive domain, extrapolate from the first two points
    i0 = 0;
    i1 = 1;
    f1 = this->extrapolate(0.0,Xval[i0],Yval[i0],Xval[i1],Yval[i1]);
    if (x > 0.0 && x <= Xval.front())
      i0 = -1;
    else
      i2 = 0;
  }
  else if (Xval.back() <= 0.0)
  {
    // All points are in negative domain, extrapolate from the last two points
    i0 = nVal-2;
    i1 = nVal-1;
    f1 = this->extrapolate(0.0,Xval[i0],Yval[i0],Xval[i1],Yval[i1]);
    if (x < 0.0 && x >= Xval.back())
      i1 = nVal;
    else
      i2 = nVal-1;
  }
  else
  {
    // Find the actual interval containing x=0.0
    for (i0 = 0; i0 < nVal-1; i0++)
      if (Xval[i0] <= 0.0 && Xval[i0+1] >= 0.0) break;

    // Interpolate at x=0.0
    i1 = i0+1;
    if (Xval[i1]-Xval[i0] > eps)
      f1 = this->interpolate(0.0,Xval[i0],Yval[i0],Xval[i1],Yval[i1]);
    else if (x > 0.0)
      f1 = Yval[++i0];
    else
      f1 = Yval[i0--];

    i1 = i0+1;
    i2 = x < 0.0 ? i0 : i1;
  }

  f1 *= scaleFac;
  f1 += vertShift;

  // Integrate the function, interval by interval
  double dx, x0, x1 = 0.0, v0, v1 = 0.0, v2 = 0.0;
  while (fabs(x1) < fabs(x))
  {
    x0 = x1;
    f0 = f1;
    v0 = v1;

    if (i0 < 0)
    {
      i0 = 0;
      i1 = 1;
      x1 = x; // Extrapolate from the first two points to x
      f1 = this->extrapolate(x1,Xval[i0],Yval[i0],Xval[i1],Yval[i1]);
#if FI_DEBUG > 2
      std::cout <<"Extrapolating last point "<< x1 <<" ==> "<< f1 << std::endl;
#endif
    }
    else if (i1 >= nVal)
    {
      i0 = nVal-2;
      i1 = nVal-1;
      x1 = x; // Extrapolate from the last two points to x
      f1 = this->extrapolate(x1,Xval[i0],Yval[i0],Xval[i1],Yval[i1]);
#if FI_DEBUG > 2
      std::cout <<"Extrapolating last point "<< x1 <<" ==> "<< f1 << std::endl;
#endif
    }
    else if (fabs(Xval[i2]) > fabs(x))
    {
      x1 = x; // Interpolate the function at x
      f1 = this->interpolate(x1,Xval[i0],Yval[i0],Xval[i1],Yval[i1]);
#if FI_DEBUG > 2
      std::cout <<"Interpolating last point "<< x1
                <<" ==> "<< f1 << std::endl;
#endif
    }
    else
    {
      x1 = Xval[i2];
      f1 = Yval[i2];
#if FI_DEBUG > 2
      std::cout <<"Whole interval ["<< x0 <<","<< x1
                <<"] ==> ["<< f0 <<","<< f1 <<"]"<< std::endl;
#endif
      if (x > 0.0)
      {
        if (i2 > 0) i0++;
        i2++;
      }
      else
      {
        if (i2 < nVal-1) i0--;
        i2--;
      }
      i1 = i0 + 1;
    }

    f1 *= scaleFac;
    f1 += vertShift;

    dx = x1 - x0;
    v1 = v0 + (f0+f1)*dx/2.0;
    if (order > 1) v2 += (v0+v1)*dx/2.0 + (f0-f1)*dx*dx/12.0;
  }

  return order > 1 ? v2 : v1;
}
