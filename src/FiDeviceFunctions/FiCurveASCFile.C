// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#include <cstdlib>
#include <cstring>
#include <iostream>

#include "FiDeviceFunctions/FiCurveASCFile.H"
#include "FFaLib/FFaDefinitions/FFaAppInfo.H"


FiCurveASCFile::FiCurveASCFile() : FiDeviceFunctionBase()
{
  outputFormat = 1;
}


FiCurveASCFile::FiCurveASCFile(const char* fname) : FiDeviceFunctionBase(fname)
{
  outputFormat = 1;
}


bool FiCurveASCFile::initialDeviceRead()
{
  char line[BUFSIZ];
  bool okRead = true;
  for (int currentLine = 1; !FT_eof(myFile); currentLine++)
  {
    if (!FT_gets(line,BUFSIZ,myFile)) continue;

    // Ignore white-space in the beginning of the line
    char* c = NULL;
    for (int i = 0; i < BUFSIZ; i++)
      if (line[i] && !isspace(line[i]))
      {
        c = line+i;
        break;
      }

    // Find all values on this line
    int valCount = 0;
    double xVal;

    while (c && *c != '\0' && *c != '\n' && *c != '\r')
    {
      char* end = c;
      double tmpVal = strtod(c,&end);

      if (end != c)
      {
        c = end;
        if (++valCount == 1)
          xVal = tmpVal;
        else if (valCount == 2)
        {
          myXValues.push_back(xVal);
          myYValues.push_back(tmpVal);
          valCount = 0;
        }
      }
      else
	switch (*c)
	  {
	  case ',':
	  case ' ':
	  case '\t':
	    c++;
	    break;
	  case '#':
	    c = NULL; // comment line, stop searching
	    break;
	  default:
	    c = NULL; // invalid data, stop searching
#ifndef FI_DEBUG
	    if (okRead)
#endif
	      std::cerr <<" *** Error: Invalid format on line "<< currentLine
			<<" in ASCII-file "<< myDatasetDevice << std::endl;
	    okRead = false;
	  }
    }
  }

  return okRead;
}


bool FiCurveASCFile::concludingDeviceWrite(bool)
{
  const char* fmts[3] =
  {
    "%.4e\t%.4e\n",
    "%.8e\t%.8e\n",
    "%.16e\t%.16e\n"
  };

  int nLines = 0;
  bool success = true;
  char cline[64];

  if (!myParent.empty())
  {
    FFaAppInfo current;
    this->writeString("#FEDEM\t",current.version);
    this->writeString("\n#PARENT\t",myParent);
    this->writeString("\n#FILE \t",myDatasetDevice);
    this->writeString("\n#USER \t",current.user);
    this->writeString("\n#DATE \t",current.date);
    this->writeString("\n#\n");
  }

  if (!myAxisInfo[X].title.empty() && !myAxisInfo[Y].title.empty())
  {
    cline[0] = '\t';
    cline[63] = '\0';
    success = this->writeString("#AXES");
    for (int i = 0; i < 2 && success; i++)
    {
      strncpy(cline+1,myAxisInfo[i].title.c_str(),62);
      success = this->writeString(cline);
    }
    if (success)
      success = this->writeString("\n");
  }

  for (size_t i = 0; i < myXValues.size() && success; i++)
  {
    nLines++;
    snprintf(cline,64,fmts[outputFormat],myXValues[i],myYValues[i]);
    success = this->writeString(cline);
  }

  if (!success)
    std::cerr <<" *** Error: Failed to write ASCII-file "<< myDatasetDevice
              <<"\n            Failure occured writing line # "<< nLines
              << std::endl;
  return success;
}


void FiCurveASCFile::setValue(double x, double y)
{
  myXValues.push_back(x);
  myYValues.push_back(y);
}


bool FiCurveASCFile::setData(const std::vector<double>& x,
			     const std::vector<double>& y)
{
  if (x.size() < 2 || x.size() != y.size())
    return false;

  myXValues = x;
  myYValues = y;
  return true;
}


void FiCurveASCFile::getRawData(std::vector<double>& x, std::vector<double>& y,
				double, double, int)
{
  x = myXValues;
  y = myYValues;
}


bool FiCurveASCFile::getValues(double, double,
                               std::vector<double>& x, std::vector<double>& y,
                               int, bool, double, double)
{
  x = myXValues;
  y = myYValues;
  return true;
}
