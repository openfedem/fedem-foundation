// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#include "FFaFunctionLib/FFaFunctionDLL/FFaFunction.h"
#include "FFaFunctionLib/FFaFunctionProperties.H"
#include "FFaFunctionLib/FFaFunctionManager.H"
#include "FiDeviceFunctions/FiDeviceFunctionFactory.H"
#include "FFaMathExpr/FFaMathExprFactory.H"
#include <iostream>
#include <cstring>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif


static std::vector<double> Buffer;


int FFaFunctionInit (const double* data, int ndata,
                     int funcId, int funcType, const char* strval)
{
  Buffer.clear();

  switch (funcType) {
  case SINUSOIDAL_p:
    if (ndata >= 3)
    {
      Buffer.resize(3);
      Buffer[0] = data[2];
      Buffer[1] = 2.0*M_PI*data[0];
      Buffer[2] = 2.0*M_PI*data[1];
    }
    break;

  case WAVE_SINUS_p:
    if (strval)
    {
      FILE* fp = fopen(strval,"r");
      if (!fp) return -funcType;

      // Count the number of sine waves in the file
      int nWave = 0;
      char buf[128];
      if (fgets(buf,128,fp))
      {
        // The first line does not count if it contains # columns
        if (strncmp(buf,"#ncol",5) && strncmp(buf,"#NCOL",5))
          nWave++;
        while (fgets(buf,128,fp))
          nWave++;
      }
      fclose(fp);
      if (FFaFunctionManager::initWaveFunction(strval,nWave,0,Buffer))
        break;
    }
    else if (ndata >= 8)
    {
      Buffer.insert(Buffer.end(),data,data+5);
      if (FFaFunctionManager::initWaveFunction(data[5],data[6],data[7],Buffer))
        break;
    }
    std::cerr <<" *** FFaFunctionInit: Invalid wave function"<< std::endl;
    return -funcType;

  case WAVE_STOKES5_p:
  case WAVE_STREAMLINE_p:
    if (ndata >= 5)
    {
      Buffer.resize(55,0.0);
      Buffer[0] = 1.0/data[0];
      Buffer[1] = 2.0*data[2];
      Buffer[2] = 2.0*M_PI*data[1];
      if (FFaFunctionManager::initWaveFunction(funcType,data[3],data[4],Buffer))
        break;
    }
    std::cerr <<" *** FFaFunctionInit: Invalid wave function"<< std::endl;
    return -funcType;

  case WAVE_EMBEDDED_p:
    std::cerr <<" *** FFaFunctionInit: Function type not supported: "
              << funcType << std::endl;
    return -funcType;

  case DEVICE_FUNCTION_p:
    if (strval && ndata >= 3)
    {
      char* scopy = strdup(strval);
      char* fName = strtok(scopy,"|");
      char* cName = strtok(NULL,"|");
      int fInd = FiDeviceFunctionFactory::instance()->open(fName);
      int cInd = FiDeviceFunctionFactory::instance()->channelIndex(fInd,cName);
      Buffer.insert(Buffer.end(),data,data+3);
      Buffer.push_back(fInd);
      Buffer.push_back(cInd);
      free(scopy);
    }
    break;

  case MATH_EXPRESSION_p:
    if (strval)
      if (FFaMathExprFactory::instance()->create(funcId,strval) <= 0)
      {
        std::cerr <<" *** FFaFunctionInit: Invalid function expression: "
                  << strval << std::endl;
        return -funcType;
      }
    break;

  case USER_DEFINED_p:
    std::cerr <<" *** FFaFunctionInit: Function type not supported: "
              << funcType << std::endl;
    return -funcType;

  default:
    // Nothing to do for the other function types, just copy the parameters
    if (ndata > 0)
      Buffer.insert(Buffer.end(),data,data+ndata);
  }

  return Buffer.size();
}


int FFaFunctionGetData (double* data, int ndata)
{
  if (ndata > (int)Buffer.size())
    ndata = Buffer.size();

  if (ndata > 0)
    memcpy(data,&Buffer.front(),ndata*sizeof(double));

  return ndata;
}


double FFaFunctionEvaluate (double x, const double* data, int ndata,
                            int funcId, int funcType, int extrapType)
{
  int ierr = 0;
  double f = 0.0;
  switch (funcType) {
  case DEVICE_FUNCTION_p:
    if (data && ndata >= 5)
      f = FiDeviceFunctionFactory::instance()->getValue(data[3],x,ierr,
                                                        data[4],data[0],
                                                        data[1],data[2]);
    else
      ierr = -funcType;
    break;

  case MATH_EXPRESSION_p:
    f = FFaMathExprFactory::instance()->getValue(funcId,x,ierr);
    break;

  default:
    if (data && ndata >= 0)
      f = FFaFunctionManager::getValue(funcId,funcType,extrapType,
                                       std::vector<double>(data,data+ndata),
                                       x,ierr);
    else
      ierr = -funcType;
    break;
  }

  if (ierr < 0)
    std::cerr <<" *** FFaFunctionEvaluate: Failure, ierr="<< ierr << std::endl;

  return f;
}
