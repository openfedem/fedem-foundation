// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#include <cstring>
#include <cmath>
#include <cfloat>

#include "FiDeviceFunctions/FiDACFile.H"
#include "FiDeviceFunctions/FiSwappedIO.H"

const int BLOCK_SIZE = 512;
const int REAL_BYTE  = 4;

const int skipFileRepositioning = -999999;


FiDACFile::FiDACFile(const char* devicename, Endianness format)
  : FiDeviceFunctionBase(devicename)
{
  myOutputEndian = format;
  myNumDatavals = 0;
  myXaxisOrigin = 0.0;
  myFirstReadValue = 0.0;
  myMaxVal = -FLT_MAX;
  myMinVal = FLT_MIN;
  myLastXVal = myLastYVal = 0.0;
  myMean = myMS = 0.0;
  maxPos = minPos = 0;
  isDataWriteInited = false;
}


bool FiDACFile::initialDeviceRead()
{
  // check the byteorder, change endian if needed
  myInputEndian = myMachineEndian; //Force unswapped read
  int dataformat = readInt16(2);
  if (dataformat > 0xff)
  {
    if (myMachineEndian == LittleEndian)
      myInputEndian = BigEndian;
    else if (myMachineEndian == BigEndian)
      myInputEndian = LittleEndian;
  }

  // Now: Read the header
  // First: Version specific stuff
  myNumDatavals = 0;
  if (readInt16(32) > 1) // version 5 (++?)
  {
    int nVals = readInt32(32);
    if (nVals > 0) myNumDatavals = nVals;
  }
  if (myNumDatavals == 0) // pre 5.0
  {
    float nVals = readFloat(1);
    if (nVals > 0.0f) myNumDatavals = nVals;
  }

  myXaxisOrigin = readFloat(3);
  float freq = readFloat(2);
  if (freq > 0.0f)
    myStep = 1.0 / (double)freq;
  else
    myStep = readFloat(4);

  // get axis title
  myAxisInfo[X].title = this->readString(96,101);
  myAxisInfo[Y].title = this->readString(90,95);

  // X-axis units
  if (readInt16(11) == -2)
    myAxisInfo[X].unit = this->readString(113,116) + this->readString(38,41,2,2);

  // Y-axis units
  if (readInt16(4) == -2)
    myAxisInfo[Y].unit = this->readString(109,112) + this->readString(35,37);

  return true; //TODO: Add validity checks and error messages
}


bool FiDACFile::preliminaryDeviceWrite()
{
  const char* cleanChar = "\0";
  const char* endStr    = "END";

  // fill the header with zeroes:
  FT_seek(myFile,(FT_int)0,SEEK_SET);
  for (int i = 0; i < BLOCK_SIZE; i++)
    FT_write(cleanChar,1,2,myFile);

  FT_write(endStr,1,3,myFile);

  // "space out" the unit and title information
  std::string cleanString(30,' ');

  // title
  this->writeString( 96,101,cleanString);
  this->writeString( 90, 95,cleanString);

  // X-units
  this->writeString(113,116,cleanString);
  this->writeString( 38, 41,cleanString,2,2);

  // Y-units
  this->writeString(109,112,cleanString);
  this->writeString( 35, 37,cleanString);

  FT_seek(myFile,(FT_int)BLOCK_SIZE,SEEK_SET);

  return true; //TODO: Add error messages on write error
}


bool FiDACFile::concludingDeviceWrite(bool)
{
  const char* cleanChar = "\0";

  // minimal DAC header info
  writeFloat(1, myNumDatavals);
  writeFloat(2, myStep > 0.0 ? 1.0/myStep : 0.0);
  writeFloat(3, myXaxisOrigin);
  writeFloat(4, myStep);

  // write addl. calculated data:
  writeFloat(5, myMean);
  writeFloat(6, myMS > myMean*myMean ? sqrt(myMS - myMean*myMean) : 0.0);
  writeFloat(9, myMaxVal);
  writeFloat(10, myMinVal);
  writeFloat(18, myMS > 0.0 ? sqrt(myMS) : 0.0);
  writeInt16(27, 1); // valid calculated statistics

  writeInt16(1, 1);
  writeInt16(2, myOutputEndian == LittleEndian ? 3 : 12);
  writeInt16(32, 2); // File format: V5.0

  writeInt32(29, maxPos);
  writeInt32(30, minPos);
  writeInt32(31, fabs(myMinVal) > myMaxVal ? minPos : maxPos);
  writeInt32(32, myNumDatavals); // In V5.0 format

  float floatMaxPos = (float)maxPos;
  float floatMinPos = (float)minPos;
  writeFloat(126, floatMaxPos);
  writeFloat(127, floatMinPos);
  writeFloat(128, fabs(myMinVal) > fabs(myMaxVal) ? floatMinPos : floatMaxPos);

  // write the title data

  std::map<int,axisInfo>::const_iterator xit = myAxisInfo.find(X);
  if (xit != myAxisInfo.end())
    this->writeString(96,101,xit->second.title);

  std::map<int,axisInfo>::const_iterator yit = myAxisInfo.find(Y);
  if (yit != myAxisInfo.end())
    this->writeString(90,95,yit->second.title);

  // write the units data

  if (xit != myAxisInfo.end())
  {
    this->writeString(113,116,xit->second.unit.substr(0,16));
    if (xit->second.unit.size() > 16)
      this->writeString(38,41,xit->second.unit.substr(16,28),2,2);
  }

  writeInt16(11,-2); // write the validity flag anyway. TTrack defect 1134/nCode support

  if (yit != myAxisInfo.end())
  {
    this->writeString(109,112,yit->second.unit.substr(0,16));
    if (yit->second.unit.size() > 16)
      this->writeString(35,37,yit->second.unit.substr(16,28));
  }

  writeInt16(4,-2); // write the validity flag anyway. TTrack defect 1134/nCode support

  // pad the last part of the file so it consists of 512-byte records
  FT_seek(myFile,BLOCK_SIZE+(FT_int)myNumDatavals*REAL_BYTE,SEEK_SET);
  for (int i = (myNumDatavals*REAL_BYTE)%BLOCK_SIZE; i < BLOCK_SIZE; i++)
    FT_write(cleanChar,1,2,myFile);

  return true; //TODO: Add error messages on write error
}


void FiDACFile::getValueRange(double& min, double& max) const
{
  min = myMinVal;
  max = myMaxVal;
}


double FiDACFile::getValue(double x, int, bool zeroAdjust,
			   double vertShift, double scaleFactor)
{
  double retval = -FLT_MAX;

  long pos = floor((x-myXaxisOrigin)/myStep);
  double f0, f1, x0, x1;

  if (pos+1 >= (long)myNumDatavals && myNumDatavals > 1)
  {
    FT_seek(myFile,BLOCK_SIZE+(FT_int)(myNumDatavals-2)*REAL_BYTE,SEEK_SET);
    //TSJ: Doesn't this read take place 4 bytes to early ?
    //TSJ: Refer to readFloat implementation...
    f0 = readFloat(skipFileRepositioning);
    f1 = readFloat(skipFileRepositioning);

    x1 = myXaxisOrigin + myStep*myNumDatavals;
    x0 = x1 - myStep;

    retval = extrapolate(x,x0,f0,x1,f1);
  }
  else if (pos < 0)
  {
    FT_seek(myFile,(FT_int)BLOCK_SIZE,SEEK_SET);
    f0 = readFloat(skipFileRepositioning);
    f1 = readFloat(skipFileRepositioning);

    x0 = myXaxisOrigin;
    x1 = x0 + myStep;

    retval = extrapolate(x,x0,f0,x1,f1);
  }
  else
  {
    FT_seek(myFile,BLOCK_SIZE+(FT_int)pos*REAL_BYTE,SEEK_SET);
    f0 = readFloat(skipFileRepositioning);
    f1 = readFloat(skipFileRepositioning);

    x0 = myXaxisOrigin + pos*myStep;
    x1 = x0 + myStep;

    retval = this->interpolate(x,x0,f0,x1,f1);
  }

  //TODO: No point in doing this every time - rewrite
  if (zeroAdjust && myFirstReadValue == 0.0)
  {
    FT_seek(myFile,(FT_int)BLOCK_SIZE,SEEK_SET);
    myFirstReadValue = readFloat(skipFileRepositioning);
  }

  retval *= scaleFactor;

  double shiftVal = 0.0;

  if (zeroAdjust)
    shiftVal = -myFirstReadValue*scaleFactor + vertShift;
  else
    shiftVal = vertShift;

  retval += shiftVal;

  return (double)retval;
}


double FiDACFile::getValueAt(unsigned long int pos)
{
  if (pos > myNumDatavals)
    pos = myNumDatavals; // return the last value

  FT_seek(myFile,BLOCK_SIZE+(FT_int)pos*REAL_BYTE,SEEK_SET);
  return readFloat(skipFileRepositioning);
}


void FiDACFile::getRawData(std::vector<double>& x, std::vector<double>& y,
                           double minX, double maxX, int)
{
  x.clear();
  y.clear();
  x.reserve(this->getValueCount());
  y.reserve(this->getValueCount());
  double currentX = this->getXAxisOrigin();

  for (size_t i = 0; i < this->getValueCount(); i++, currentX += myStep)
    if (minX > maxX || (currentX >= minX && currentX <= maxX))
    {
      x.push_back(currentX);
      y.push_back(this->getValueAt(i));
    }
}


bool FiDACFile::getValues(double x0, double x1,
                          std::vector<double>& x, std::vector<double>& y,
                          int, bool zeroAdjust, double shift, double scale)
{
  x.clear();
  y.clear();
  double currX = this->getXAxisOrigin();
  size_t nVals = this->getValueCount();

  if (x0 < currX) x0 = currX;
  if (x1 > currX + (nVals-1)*myStep) x1 = currX + (nVals-1)*myStep;

  int nPoints  = (int)ceil((x1-x0)/myStep);
  x.reserve(nPoints);
  y.reserve(nPoints);

  if (zeroAdjust && myFirstReadValue == 0.0)
  {
    FT_seek(myFile,(FT_int)BLOCK_SIZE,SEEK_SET);
    myFirstReadValue = readFloat(skipFileRepositioning);
  }

  double currY;
  for (size_t i = 0; i < nVals && currX <= x1; i++)
  {
    if (currX >= x0)
    {
      currY = shift + this->getValueAt(i)*scale;
      if (zeroAdjust) currY -= myFirstReadValue*scale;
      x.push_back(currX);
      y.push_back(currY);
    }
    currX += myStep;
  }
  return true;
}


//////////////////////////////////////////////////////////////////////
// Assumes that data set does not have doublebacking x-axis
//

bool FiDACFile::setData(const std::vector<double>& x,
			const std::vector<double>& y)
{
  // Assumes the data has strictly increasing x-axis
  if (x.size() < 2 || x.size() != y.size()) return false;

  this->preliminaryDeviceWrite(); //this is just positions the stream. Cut?
  isDataWriteInited = true;

  myXaxisOrigin = x.front();
  int numData = (int)ceil((x[x.size()-1]-myXaxisOrigin)/myStep);

  int ind = 0;
  double xVal, x0, f0, x1, f1, f = 0.0;

  for (int l = 0; l < numData; l++)
  {
    xVal = x.front() + l*myStep;
    for (int m = ind; m < numData-1; m++)
      if (xVal <= x[m+1])
      {
        x0 = x[m]; x1 = x[m+1];
        f0 = y[m]; f1 = y[m+1];
        f  = interpolate(xVal,x0,f0,x1,f1);
	ind = m > 2 ? m-2 : 0;
	break;
      }

    writeFloat(skipFileRepositioning,(float)f);
    updateStatistics(f);
  }
  return true;
}


void FiDACFile::setValue(double x, double y)
{
  //TODO: Rethink this... Is it safe for non-constant incs?
  if (!isDataWriteInited)
  {
    this->preliminaryDeviceWrite();
    myXaxisOrigin = x;
    myLastYVal = y;
    myLastXVal = 0;
    writeFloat(skipFileRepositioning,(float)y);
    updateStatistics(y);
    isDataWriteInited = true;
    return;
  }

  if (myStep < 0.0) // write values directly
  {
    writeFloat(skipFileRepositioning,(float)y);
    updateStatistics(y);
  }
  else // need to interpolate...
  {
    x -= myXaxisOrigin;
    double delta = x + myStep - myNumDatavals*myStep;
    int stepCount = (int)floor(delta/myStep);

    for (int i = 0; i < stepCount; i++)
    {
      double wVal = interpolate(myNumDatavals*myStep,myLastXVal,myLastYVal,x,y);
      writeFloat(skipFileRepositioning,(float)wVal);
      updateStatistics(wVal);
    }
    myLastYVal = y;
    myLastXVal = x;
  }
}


void FiDACFile::updateStatistics(double val)
{
  // check for first value:
  if (++myNumDatavals == 1)
  {
    myMean   = val;
    myMS     = val*val;
    myMaxVal = val;
    myMinVal = val;
    maxPos = minPos = 1;
  }
  else
  {
    if (val > myMaxVal)
    {
      myMaxVal = val;
      maxPos   = myNumDatavals;
    }
    if (val < myMinVal)
    {
      myMinVal = val;
      minPos   = myNumDatavals;
    }

    // Mean
    myMean = myMean + (val-myMean)/myNumDatavals;
    // Squared mean
    myMS   = (myMS*(myNumDatavals-1) + val*val)/myNumDatavals;
  }
}


/////////////////////////////////////////////////////////////////////
// Direct read
// header and footer region

float FiDACFile::readFloat(int pos)
{
  float retVal = 0.0f;
  size_t nr = 0;

  if (pos != skipFileRepositioning)
    FT_seek(myFile,(FT_int)(pos-1)*4,SEEK_SET);

  if (myInputEndian == myMachineEndian)
    nr = FT_read((char*)&retVal, 4, 1, myFile);
  else
    nr = Fi::readSwapped((char*)(&retVal), retVal, myFile);

  return nr < 4 ? 0.0f : retVal;
}

short FiDACFile::readInt16(int pos)
{
  short retVal = 0;
  size_t nr = 0;

  if (pos != skipFileRepositioning)
    FT_seek(myFile,32*4+(FT_int)(pos-1)*2,SEEK_SET);

  if (myInputEndian == myMachineEndian)
    nr = FT_read((char*)&retVal, 2, 1, myFile);
  else
    nr = Fi::readSwapped((char*)(&retVal), retVal, myFile);

  return nr < 2 ? 0 : retVal;
}

int FiDACFile::readInt32(int pos)
{
  int retVal = 0;
  size_t nr = 0;

  if (pos != skipFileRepositioning)
    FT_seek(myFile,(FT_int)(pos-1)*4,SEEK_SET);

  if (myInputEndian == myMachineEndian)
    nr = FT_read((char*)&retVal, 4, 1, myFile);
  else
    nr = Fi::readSwapped((char*)(&retVal), retVal, myFile);

  return nr < 4 ? 0 : retVal;
}

std::string FiDACFile::readString(int start, int end,
                                  int startOffset, int endOffset)
{
  FT_seek(myFile,(FT_int)(start-1)*4+startOffset,SEEK_SET);

  int nChar = (end+1-start)*4 - endOffset;
  std::string buf(nChar+1,0);
  if (FT_read(buf.c_str(),1,nChar,myFile) < 1)
    buf.clear();

  return buf;
}


/////////////////////////////////////////////////////////////////////
// Direct write
// Header and footer region

void FiDACFile::writeFloat(int pos, float val)
{
  if (pos != skipFileRepositioning)
    FT_seek(myFile,(FT_int)(pos-1)*4,SEEK_SET);

  if (myOutputEndian == myMachineEndian)
    FT_write(&val,4,1,myFile);
  else
    Fi::writeSwapped(val,myFile);
}

void FiDACFile::writeInt16(int pos, int val)
{
  if (pos != skipFileRepositioning)
    FT_seek(myFile,32*4+(FT_int)(pos-1)*2,SEEK_SET);

  short val16 = val;
  if (myOutputEndian == myMachineEndian)
    FT_write(&val16,2,1,myFile);
  else
    Fi::writeSwapped(val16,myFile);
}

void FiDACFile::writeInt32(int pos, int val)
{
  if (pos != skipFileRepositioning)
    FT_seek(myFile,(FT_int)(pos-1)*4,SEEK_SET);

  if (myOutputEndian == myMachineEndian)
    FT_write(&val,4,1,myFile);
  else
    Fi::writeSwapped(val,myFile);
}

int FiDACFile::writeString(int start, int end, const std::string& val,
			   int startOffset, int endOffset)
{
  FT_seek(myFile,(FT_int)(start-1)*4+startOffset,SEEK_SET);

  int bufLen = val.length();
  int maxLen = (end+1-start)*4 - (endOffset + startOffset);
  int nChar  = bufLen < maxLen ? bufLen : maxLen;
  FT_write(val.c_str(),1,nChar,myFile);
  return nChar;
}
