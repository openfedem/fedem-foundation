// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#include <iostream>
#include <cstdlib>
#include <cmath>
#include <ctime>
#include <cstring>

#include "FFaLib/FFaString/FFaStringExt.H"
#include "FiDeviceFunctions/FiRPC3File.H"
#include "FiDeviceFunctions/FiSwappedIO.H"


const int BLOCK_SIZE = 512;
const int REC_SIZE   = 128;
const int KEY_SIZE   = 32;
const int VAL_SIZE   = 96;

const FT_int skipFileRepos = -999999;


FiRPC3File::FiRPC3File()
{
  this->commonConstructor();
}

FiRPC3File::FiRPC3File(const char* devicename)
  : FiDeviceFunctionBase(devicename)
{
  this->commonConstructor();
}

FiRPC3File::FiRPC3File(const char* devicename, int endianFormat)
  : FiDeviceFunctionBase(devicename)
{
  this->commonConstructor();
  myOutputEndian = (Endianness)endianFormat;
}

FiRPC3File::FiRPC3File(const char* devicename, int endianFormat,
		       int numberOfChannels)
  : FiDeviceFunctionBase(devicename)
{
  this->commonConstructor();
  myOutputEndian = (Endianness)endianFormat;
  myNumChannels = numberOfChannels;
}


void FiRPC3File::commonConstructor()
{
  myMaxVal = 0.0;
  myMinVal = 0.0;

  // Time history file info
  myKeys.clear();
  myChannels.clear();

  myNumHeaderBlocks = 0;
  myNumPartitions   = 0;
  myNumDatavals     = 0;
  myNumChannels     = 0;
  myNumParams = 0;
  myNumFrames = 0;
  myNumFrmPts = 1024;
  myNumGrpPts = 2048;

  myHalfFrameUse = false;
  myDataSize = 0;
  myByteShift = 0;
  myPhysChan = 0;
  myPartShift = 0;
  myDataFormat = 0;
  myPartition = 0;
  myFileType = 0;
  myChannel = 0;

  myDataType     = ShortInt; // Default data type on write
  myBypassFilter = false;
  myChannelScale = 0;
  myTimeSpan     = 0;
  myRepeats      = 0;
  myAverages     = 0;

  myXaxisOrigin = myFirstReadValue = 0.0;
  swapStringBytes = stepSet = false;
  kInd = 0;
}


// ---------- Logic section ---------

bool FiRPC3File::initialDeviceRead()
{
  // Quick check on validity of file
  char buf[6];
  FT_seek(myFile,(FT_int)0,SEEK_SET);
  for (int t = 0; t < 6; t++) buf[t] = toupper(FT_getc(myFile));
  if (strncmp(buf,"FORMAT",6) == 0)
    swapStringBytes = false;
  else if (strncmp(buf,"OFMRTA",6) == 0)
    swapStringBytes = true;
  else
  {
#ifdef FI_DEBUG
    std::cerr <<" *** Error: "<< myDatasetDevice
	      <<" is not a valid RPC-file.\n";
#endif
    return false;
  }

  // Start reading parameters
  parameters.clear();
  myNumParams = readInt32(2*REC_SIZE+KEY_SIZE,true);
  myKeys.reserve(myNumParams);
  myKeys.clear();
  for (int i = 0; i < myNumParams; i++)
    myKeys.push_back(readString(i*REC_SIZE,true));

  // -- Read commmon header data
  bool okRead = true;
  bool dontCare = true;
  std::string formatting(getKeyString("FORMAT",okRead));
  if (!okRead) return false;

  if (formatting == "BINARY_IEEE_LITTLE_END")
  {
    myInputEndian = FiDeviceFunctionBase::LittleEndian;
    myDataFormat  = FiDeviceFunctionBase::binary;
  }
  else if (formatting == "BINARY_IEEE_BIG_END")
  {
    myInputEndian = FiDeviceFunctionBase::BigEndian;
    myDataFormat  = FiDeviceFunctionBase::binary;
  }
  else if (formatting == "BINARY")
  {
    myInputEndian = FiDeviceFunctionBase::LittleEndian;
    myDataFormat  = FiDeviceFunctionBase::binary;
  }
  else if (formatting == "ASCII")
    myDataFormat = FiDeviceFunctionBase::ascii;
  else
  {
#ifdef FI_DEBUG
    std::cerr <<" *** Error: Invalid format \""<< formatting
	      <<"\" of RPC-file "<< myDatasetDevice << std::endl;
#endif
    return false;
  }

  myNumHeaderBlocks = getKeyInt("NUM_HEADER_BLOCKS",okRead);
  if (!okRead) return false;

  // -- Determine file type
  std::string fileType(getKeyString("FILE_TYPE",okRead));
  if (!okRead) return false;

  if (fileType == "TIME_HISTORY")
  {
    // -- Read time_hist specific header data
    myFileType      = Time_History;
    myNumChannels   = getKeyInt("CHANNELS",okRead);
    myNumPartitions = getKeyInt("PARTITIONS",okRead);
    myNumGrpPts     = getKeyInt("PTS_PER_GROUP",okRead);
    myNumFrames     = getKeyInt("FRAMES",okRead);
    myNumFrmPts     = getKeyInt("PTS_PER_FRAME",okRead);
    myNumDatavals   = myNumFrames*myNumFrmPts;
    myStep          = getKeyFloat("DELTA_T",okRead);
    myBypassFilter  = getKeyInt("BYPASS_FILTER",dontCare) != 0;
    myHalfFrameUse  = getKeyInt("HALF_FRAMES",dontCare) != 0;
    myRepeats       = getKeyInt("REPEATS",dontCare);
    myAverages      = getKeyInt("AVERAGES",dontCare);
    std::string dataType(getKeyString("DATA_TYPE",dontCare));
    if (dataType == "DOUBLE_PRECISION") // Non-standard
    {
      myDataType = Double;
      myDataSize = 8;
    }
    else if (dataType == "FLOATING_POINT")
    {
      myDataType = Float;
      myDataSize = 4;
    }
    else
    {
      myDataType = ShortInt;
      myDataSize = 2;
    }
    okRead = readChannelList();
  }
  else
  {
#ifdef FI_DEBUG
    std::cerr <<" *** Error: Invalid type \""<< fileType
	      <<"\" of RPC-file "<< myDatasetDevice
	      <<"\n            Only \"TIME_HISTORY\" is allowed.\n";
#endif
    return false;
  }
#if FI_DEBUG > 1
  dumpAllKeysToScreen();
#endif
  return okRead;
}


void FiRPC3File::getValueRange(double& min, double& max) const
{
  min = myMinVal;
  max = myMaxVal;
}


bool FiRPC3File::getChannelList(std::vector<std::string>& list)
{
  if (myChannels.empty()) return false;
  list = myChannels;
  return true;
}


bool FiRPC3File::readChannelList()
{
  myChannels.reserve(myNumChannels);
  myChannels.clear();

  // File not initiated?
  if (myNumChannels == 0 || myKeys.empty()) return false;
  // Keys not initiated?
  if (myKeys[0] != std::string("FORMAT")) return false;

  for (int i = 1; i <= myNumChannels; i++)
  {
    bool ok1 = true, ok2 = true;
    std::string chMap = getKeyString(FFaNumStr("MAP.CHAN_%d",i),ok1);
    if (!ok1) return false; // MAP.CHAN is mandatory

    std::string chDesc = getKeyString(FFaNumStr("DESC.CHAN_%d",i),ok1);
    std::string chUnit = getKeyString(FFaNumStr("UNITS.CHAN_%d",i),ok2);
    if (chMap == chDesc) chMap = FFaNumStr(i); // Avoid duplicated info

    if (ok1 && ok2)
      myChannels.push_back(chMap + ": " + chDesc + " [" + chUnit + "]");
    else if (ok1)
      myChannels.push_back(chMap + ": " + chDesc);
    else if (ok2)
      myChannels.push_back(chMap + " [" + chUnit + "]");
    else
      myChannels.push_back(chMap);
  }

  return true;
}


int FiRPC3File::isChannelPresentInFile(const std::string& channel)
{
  for (size_t i = 0; i < myChannels.size(); i++)
    if (channel == myChannels[i])
      if (isChannelPresentInFile(i+1))
	return i+1;

  return 0;
}

bool FiRPC3File::isChannelPresentInFile(int channel)
{
  std::string channelDescriptors[4];
  channelDescriptors[0] = FFaNumStr("MAP.CHAN_%d",channel);
  channelDescriptors[1] = FFaNumStr("SCALE.CHAN_%d",channel);
  channelDescriptors[2] = FFaNumStr("UPPER_LIMIT.CHAN_%d",channel);
  channelDescriptors[3] = FFaNumStr("LOWER_LIMIT.CHAN_%d",channel);

  bool ok = true;
  for (int i = 0; i < 4 && ok; i++)
    getKeyString(channelDescriptors[i],ok);

  return ok;
}


double FiRPC3File::getValue(double x, int channel,
			    bool zeroAdjust, double vertShift,
			    double scaleFactor)
{
  // Check for invalid channel number, return zero if erroneous channel
  if (channel <= 0 || channel > myNumChannels)
  {
#ifdef FI_DEBUG
    std::cerr <<" *** Error: Invalid channel "<< channel
	      <<" for RPC-file "<< myDatasetDevice << std::endl;
#endif
    return 0.0;
  }

  // Check if this channel has been initiated from file, if not do so
  if (parameters.find(channel) == parameters.end())
  {
    if (myFileType != Time_History) return 0.0;

    this->initTHChannel(channel,toRead);
  }

  // Retrieve positioning parameters for current channel
  setReadParams(channel);

  // Check the point's channel position
  FT_int pos = 0;
  int chPos  = (int)floor((x-myXaxisOrigin)/myStep);
  int grpNum = 0;
  int grpPos = 0;

  double x0, x1, f0, f1, retval;

  if (chPos < 0)
  {
    // Before channel start - extrapolate from channel start
    x0 = myXaxisOrigin;
    x1 = x0 + myStep;

    pos = myByteShift;
    if (myDataType == Double)
    {
      f0 = readDouble(pos);
      f1 = readDouble(skipFileRepos);
    }
    else if (myDataType == Float)
    {
      f0 = (double)readFloat(pos);
      f1 = (double)readFloat(skipFileRepos);
    }
    else
    {
      f0 = (double)readInt16(pos);
      f1 = (double)readInt16(skipFileRepos);
    }

    retval = extrapolate(x,x0,f0,x1,f1);
  }
  else if ((size_t)(chPos+1) >= myNumDatavals)
  {
    // After channel end - extrapolate from channel end
    x0 = (myNumDatavals-1)*myStep + myXaxisOrigin;
    x1 = x0 + myStep;

    grpNum = (int)floor((double)(myNumDatavals-1)/myNumGrpPts) + 1;
    grpPos = myNumGrpPts - 2;

    pos = (myNumChannels*(grpNum-1)*myNumGrpPts + grpPos)*myDataSize
      + myByteShift;

    if (myDataType == Double)
    {
      f0 = readDouble(pos);
      f1 = readDouble(skipFileRepos);
    }
    else if (myDataType == Float)
    {
      f0 = (double)readFloat(pos);
      f1 = (double)readFloat(skipFileRepos);
    }
    else
    {
      f0 = (double)readInt16(pos);
      f1 = (double)readInt16(skipFileRepos);
    }

    retval = extrapolate(x,x0,f0,x1,f1);
  }
  else
  {
    // On valid channel position - position and read
    // TODO: Replace myNumChannels with part.nChan_myPartition
    //       (Current shift will fail for partitioned files.)
    grpNum = (int)floor((double)chPos/myNumGrpPts) + 1;
    grpPos = chPos % myNumGrpPts;

    // Note that two points in succession need not lie in the same group...
    pos = (myNumChannels*(grpNum-1)*myNumGrpPts + grpPos)*myDataSize
      + myByteShift;

    int chPos2    = chPos + 1;
    int grpNum2   = (int)floor((double)chPos2/myNumGrpPts) + 1;
    int grpPos2   = chPos2 % myNumGrpPts;

    // ...which is why we reposition for the second read
    // (If optimal speed is a point we could check for group switches before
    // repositioning. Do?)
    FT_int pos2 = (myNumChannels*(grpNum2-1)*myNumGrpPts + grpPos2)*myDataSize
      + myByteShift;

    x0 = chPos*myStep + myXaxisOrigin;
    x1 = x0 + myStep;

    if (myDataType == Double)
    {
      f0 = readDouble(pos);
      if ((size_t)(chPos2+1) >= myNumDatavals)
	f1 = f0;
      else if (grpNum == grpNum2)
	f1 = readDouble(skipFileRepos);
      else
	f1 = readDouble(pos2);
    }
    else if (myDataType == Float)
    {
      f0 = (double)readFloat(pos);
      if ((size_t)(chPos2+1) >= myNumDatavals)
	f1 = f0;
      else if (grpNum == grpNum2)
	f1 = (double)readFloat(skipFileRepos);
      else
	f1 = (double)readFloat(pos2);
    }
    else
    {
      f0 = (double)readInt16(pos);
      if ((size_t)(chPos2+1)>= myNumDatavals)
	f1 = f0;
      else if (grpNum == grpNum2)
	f1 = (double)readInt16(skipFileRepos);
      else
	f1 = (double)readInt16(pos2);
    }

    retval = interpolate(x,x0,f0,x1,f1);
  }

  // Check internal file filter
  if (!myBypassFilter) retval *= myChannelScale;

  // Scale and vertical shift provided by user
  retval *= scaleFactor;
  double shiftVal = vertShift;
  if (zeroAdjust) shiftVal -= myFirstReadValue*scaleFactor;
  retval += shiftVal;

  return retval;
}


int FiRPC3File::getPartition(int channel)
{
  int fChan, nChan;
  bool okRead = true;
  for (int i = 1; i <= myNumPartitions; i++)
  {
    fChan = getKeyInt(FFaNumStr("PART.CHAN_%d",i),okRead);
    nChan = getKeyInt(FFaNumStr("PART.NCHAN_%d",i),okRead);
    if (!okRead)
      return -i;
    else if (channel >= fChan && channel < fChan+nChan)
      return i;
    else
      myPartShift += myNumDatavals*nChan*myDataSize;
  }

  return -1;
}


bool FiRPC3File::initTHChannel(int channel, int action)
{
  // Initialize the channel-specific stuff in a time_hist file
  int headerSize = myNumHeaderBlocks * BLOCK_SIZE;

  FFaNumStr valString1("SCALE.CHAN_%d",channel);
  FFaNumStr valString2("MAP.CHAN_%d",channel);
  FFaNumStr valString3("UPPER_LIMIT.CHAN_%d",channel);
  FFaNumStr valString4("LOWER_LIMIT.CHAN_%d",channel);
  FFaNumStr valString5("DESC.CHAN_%d",channel);
  FFaNumStr valString6("UNITS.CHAN_%d",channel);

  if (action == toRead)
  {
    bool okRead = true;
    chParams currParams;
    currParams.chScale  = getKeyFloat(valString1,okRead);
    currParams.maxVal   = getKeyFloat(valString3,okRead);
    currParams.minVal   = getKeyFloat(valString4,okRead);
    currParams.part     = getPartition(channel);
    if (!okRead || currParams.part < 0) return false;

    currParams.partShift = myPartShift;
    currParams.byteShift = myPartShift + headerSize;
    currParams.byteShift += (channel-1)*myNumGrpPts*myDataSize;
    if (myHalfFrameUse) currParams.byteShift += myNumFrmPts/2;

    if (myDataType == Double)
      currParams.X0val = readDouble(currParams.byteShift);
    else if (myDataType == Float)
      currParams.X0val = (double)readFloat(currParams.byteShift);
    else
      currParams.X0val = (double)readInt16(currParams.byteShift);

    if (!myBypassFilter) currParams.X0val *= currParams.chScale;

    parameters.insert(std::map<int,chParams>::value_type(channel,currParams));
  }
  else if (action == toWrite)
  {
    if (myDataType == Double || myDataType == Float)
      setKeyInt(valString1, 1, kInd++);
    else
    {
      double absMax = myMaxVal >= 0 ? myMaxVal : -myMaxVal;
      double absMin = myMinVal >= 0 ? myMinVal : -myMinVal;
      double absAbs = absMax > absMin ? absMax : absMin;
      myChannelScale = absAbs/32752.0; // Is this value correct? (2^15=32768)
      setKeyFloat(valString1, (float)myChannelScale, kInd++);
    }
    setKeyInt(valString2, channel, kInd++);
    setKeyFloat(valString3, (float)myMaxVal, kInd++);
    setKeyFloat(valString4, (float)myMinVal, kInd++);
    if (!myAxisInfo[1].title.empty())
      setKeyString(valString5, myAxisInfo[1].title, kInd++);
    else if (channel <= (int)myChannels.size())
      setKeyString(valString5, myChannels[channel-1], kInd++);
    else
      setKeyString(valString5, FFaNumStr(channel), kInd++);
    if (!myAxisInfo[1].unit.empty())
      setKeyString(valString6, myAxisInfo[1].unit, kInd++);
    else
      setKeyString(valString6, "NONE", kInd++);
    myPhysChan = channel;
    myByteShift = myPartShift + headerSize;
    myByteShift += (FT_int)(channel-1)*myNumGrpPts*myDataSize;
    if (myHalfFrameUse) myByteShift += myNumFrmPts/2;
    myXaxisOrigin = 0;
  }

  return true;
}


void FiRPC3File::setReadParams(int channel)
{
  std::map<int,chParams>::iterator pos = parameters.find(channel);
  if (pos == parameters.end()) return;

  const chParams& currParams = pos->second;
  myChannelScale   = currParams.chScale;
  myMaxVal         = currParams.maxVal;
  myMinVal         = currParams.minVal;
  myPhysChan       = channel;
  myPartition      = currParams.part;
  myPartShift      = currParams.partShift;
  myByteShift      = currParams.byteShift;
  myFirstReadValue = currParams.X0val;
  myXaxisOrigin    = 0;
}


void FiRPC3File::getData(std::vector<double>& x, std::vector<double>& y,
			 const std::string& channel, double minX, double maxX)
{
  this->getRawData (x, y, minX, maxX, this->isChannelPresentInFile(channel));
}


void FiRPC3File::getRawData(std::vector<double>& x, std::vector<double>& y,
			    double minX, double maxX, int channel)
{
  // These data are not entirely raw though (since they do use getValue)
  x.clear();
  y.clear();

  // Check for invalid channel number here,
  // to avoid repetive error messages from getValue
  if (channel <= 0 || channel > myNumChannels)
  {
#ifdef FI_DEBUG
    std::cerr <<" *** Error: Invalid channel "<< channel
	      <<" for RPC-file "<< myDatasetDevice << std::endl;
#endif
    return;
  }

  x.reserve(myNumDatavals);
  y.reserve(myNumDatavals);

  double xVal, yVal;
  for (size_t i = 0; i < myNumDatavals; i++)
  {
    xVal = i*myStep;
    if (minX > maxX || (xVal >= minX && xVal <= maxX))
    {
      yVal = this->getValue(xVal,channel);
      x.push_back(xVal);
      y.push_back(yVal);
    }
  }
}


bool FiRPC3File::getValues(double x0, double x1,
			   std::vector<double>& x, std::vector<double>& y,
			   int channel, bool zeroAdjust,
			   double shift, double scale)
{
  x.clear();
  y.clear();

  // Check for invalid channel number here,
  // to avoid repetive error messages from getValue
  if (channel <= 0 || channel > myNumChannels)
  {
#ifdef FI_DEBUG
    std::cerr <<" *** Error: Invalid channel "<< channel
	      <<" for RPC-file "<< myDatasetDevice << std::endl;
#endif
    return false;
  }

  if (x0 < 0.0) x0 = 0.0;
  if (x1 > (myNumDatavals-1)*myStep) x1 = (myNumDatavals-1)*myStep;

  int nPoints = (int)ceil((x1-x0)/myStep);
  x.reserve(nPoints);
  y.reserve(nPoints);

  double xVal = 0.0, yVal;
  for (int i = 0; xVal <= x1; i++)
    if ((xVal = i*myStep) >= x0)
    {
      yVal = this->getValue(xVal,channel,zeroAdjust,shift,scale);
      x.push_back(xVal);
      y.push_back(yVal);
    }

  return true;
}


bool FiRPC3File::setData(const std::vector<double>& x,
			 const std::vector<double>& y)
{
  myChannel++;
  if (x.size() < 2 || x.size() != y.size()) return false;

  double channelSpan = x[x.size()-1] - x[0];
  if (myTimeSpan <= 0.0) myTimeSpan = channelSpan;
  int numChVals = (int)ceil(channelSpan/myStep);

  myMinVal = myMaxVal = y[0];
  for (size_t i = 1; i < y.size(); i++)
    if (y[i] < myMinVal)
      myMinVal = y[i];
    else if (y[i] > myMaxVal)
      myMaxVal = y[i];

  if (!stepSet)
  {
    setKeyFloat("DELTA_T", myStep, 4);
    myNumDatavals = (int)ceil(myTimeSpan/myStep);
    while (numChVals < myNumFrmPts/2 && myNumFrmPts > 256) myNumFrmPts /= 2;
    setKeyInt("PTS_PER_FRAME", myNumFrmPts, 5);
    myNumFrames = (int)ceil((double)myNumDatavals/myNumFrmPts);
    setKeyInt("FRAMES", myNumFrames, 6);
    stepSet = true;
  }

  initTHChannel(myChannel,toWrite);

  FT_int pos;
  int grpNum = 1, grpPos = 0;
  double xVal, x0, x1, f0, f1, f = 0.0;
  double lastX1 = x[0] - myStep;

  for (size_t chPos = 0; chPos < myNumDatavals; chPos++)
  {
    if ((int)chPos < numChVals)
    {
      xVal = x[0] + chPos*myStep;
      if (xVal > lastX1)
	for (size_t j = 0; j < x.size()-1; j++)
	  if (xVal <= x[j+1])
	  {
	    x0 = x[j]; x1 = x[j+1];
	    f0 = y[j]; f1 = y[j+1];
	    f  = interpolate(xVal,x0,f0,x1,f1);
	    lastX1 = x1;
	    break;
	  }
    }
    else
      f = 0.0;

    grpNum = (int)floor((double)chPos/myNumGrpPts) + 1;
    grpPos = chPos % myNumGrpPts;
    pos    = (myNumChannels*(grpNum-1)*myNumGrpPts + grpPos)*myDataSize
      + myByteShift;

    if (myDataType == Double)
      writeDouble(f,pos);
    else if (myDataType == Float)
      writeFloat((float)f,pos);
    else
      writeInt16((short)(f/myChannelScale),pos);
  }

  // Pad the group with zeros
  int endPos = (myNumDatavals-1) % myNumGrpPts;
  if (endPos > 0)
    for (grpPos = endPos+1; grpPos <= myNumGrpPts; grpPos++)
    {
      pos = (myNumChannels*(grpNum-1)*myNumGrpPts + grpPos)*myDataSize
	+ myByteShift;
      if (myDataType == Double)
	writeDouble((double)0,pos);
      else if (myDataType == Float)
	writeFloat((float)0,pos);
      else
	writeInt16((short)0,pos);
    }

  // If last channel then pad remainder of block
  if (myChannel == myNumChannels)
  {
    int endPos = myNumChannels*myNumGrpPts*grpNum % (BLOCK_SIZE/myDataSize);
    if (endPos > 0)
      for (grpPos = endPos+1; grpPos <= BLOCK_SIZE/myDataSize; grpPos++)
      {
	pos = (myNumChannels*(grpNum-1)*myNumGrpPts + grpPos)*myDataSize
	  + myByteShift;
	if (myDataType == Double)
	  writeDouble((double)0,pos);
	else if (myDataType == Float)
	  writeFloat((float)0,pos);
	else
	  writeInt16((short)0,pos);
      }
  }

  return true;
}


void FiRPC3File::setDescription(const std::string& desc)
{
  if (myChannel == (int)myChannels.size() && myChannel < myNumChannels)
    myChannels.push_back(desc);
}


bool FiRPC3File::preliminaryDeviceWrite()
{
  stepSet = false;

  // Settings for a binary time history file
  myDataFormat    = FiDeviceFunctionBase::binary;
  myFileType      = Time_History;
  myHalfFrameUse  = false;
  myNumPartitions = 1;
  myByteShift     = 0;
  myChannelScale  = 1;

  if (myDataType == Double)
  {
    myBypassFilter = true;
    myDataSize     = 8;
  }
  else if (myDataType == Float)
  {
    myBypassFilter = true;
    myDataSize     = 4;
  }
  else
  {
    myBypassFilter = false;
    myDataSize     = 2;
  }

  myMaxVal = 0.0;
  myMinVal = 0.0;

  myNumParams = 21 + myNumChannels*6;
  if (myAverages > 0) myNumParams++;
  myNumHeaderBlocks = (int)ceil((double)myNumParams/4);

  // Write header records, first three must be positioned as shown...
  if (myOutputEndian == FiDeviceFunctionBase::LittleEndian)
    setKeyString("FORMAT", "BINARY_IEEE_LITTLE_END", 1);
  else
    setKeyString("FORMAT", "BINARY_IEEE_BIG_END", 1);

  setKeyInt("NUM_HEADER_BLOCKS", myNumHeaderBlocks, 2);
  setKeyInt("NUM_PARAMS", myNumParams, 3);

  // ...we reserve position 4 for delta_t, position 5 for no. of frame pts,
  // and position 6 for no. of frames. After that the position is not important.
  kInd = 7;
  setKeyString("FILE_TYPE", "TIME_HISTORY", kInd++);
  setKeyString("TIME_TYPE", "RESPONSE", kInd++);

  if (myDataType == Double)
    setKeyString("DATA_TYPE", "DOUBLE_PRECISION", kInd++); // Non-standard
  else if (myDataType == Float)
    setKeyString("DATA_TYPE", "FLOATING_POINT", kInd++);
  else
    setKeyString("DATA_TYPE", "SHORT_INTEGER", kInd++);

  setKeyInt("PARTITIONS", 1, kInd++);
  setKeyInt("PART.CHAN_1", 1, kInd++);
  setKeyInt("PART.NCHAN_1", myNumChannels, kInd++);
  setKeyInt("PTS_PER_GROUP", myNumGrpPts, kInd++);
  setKeyInt("CHANNELS", myNumChannels, kInd++);
  setKeyInt("BYPASS_FILTER", myBypassFilter ? 1:0, kInd++);
  setKeyInt("HALF_FRAMES", myHalfFrameUse ? 1:0, kInd++);
  setKeyInt("REPEATS", myRepeats, kInd++);
  if (myAverages > 0) setKeyInt("AVERAGES", myAverages, kInd++);
  setKeyInt("INT_FULL_SCALE", 32752, kInd++);

  return true; //TODO: Add error messages on write error
}


bool FiRPC3File::concludingDeviceWrite(bool)
{
  time_t rawtime; time(&rawtime);
  setKeyString("PARENT_1", myParent, kInd++);
  setKeyString("DATE", ctime(&rawtime), kInd++);
  setKeyString("OPERATION", "FEDEM", kInd++);

  return true; //TODO: Add error messages on write error
}


// ---------- Set keys ----------

void FiRPC3File::setKeyInt(const std::string& key, const int val, int numb)
{
  FFaNumStr valBuf(val);
  writeString (key, (numb-1)*REC_SIZE);
  writeString (valBuf,(numb-1)*REC_SIZE + KEY_SIZE);
}


void FiRPC3File::setKeyFloat(const std::string& key, const float val, int numb)
{
  FFaNumStr valBuf("%e",val);
  writeString (key,(numb-1)*REC_SIZE);
  writeString (valBuf,(numb-1)*REC_SIZE + KEY_SIZE);
}


void FiRPC3File::setKeyString(const std::string& key, const std::string& val, int numb)
{
  writeString (key,(numb-1)*REC_SIZE);
  writeString (val,(numb-1)*REC_SIZE + KEY_SIZE);
}


// ---------- Get keys ----------

int FiRPC3File::getKeyInt (const std::string& key, bool& okRead)
{
  for (int i = 0; i < myNumParams; i++)
    if (myKeys[i] == key)
      return readInt32(i*REC_SIZE+KEY_SIZE,true);

  okRead = false;
#ifdef FI_DEBUG
   std::cerr <<" *** Error: Key \""<< key <<"\" is not present in RPC-file "
       << myDatasetDevice << std::endl;
#endif
  return 0;
}


float FiRPC3File::getKeyFloat (const std::string& key, bool& okRead)
{
  for (int i = 0; i < myNumParams; i++)
    if (myKeys[i] == key)
      return readFloat(i*REC_SIZE+KEY_SIZE,true);

  okRead = false;
#ifdef FI_DEBUG
   std::cerr <<" *** Error: Key \""<< key <<"\" is not present in RPC-file "
       << myDatasetDevice << std::endl;
#endif
  return 0.0;
}


std::string FiRPC3File::getKeyString (const std::string& key, bool& okRead)
{
  for (int i = 0; i < myNumParams; i++)
    if (myKeys[i] == key)
      return readString(i*REC_SIZE+KEY_SIZE);

  okRead = false;
#ifdef FI_DEBUG
   std::cerr <<" *** Error: Key \""<< key <<"\" is not present in RPC-file "
	     << myDatasetDevice << std::endl;
#endif
  return std::string("");
}


// ---------- Direct write section ----------

void FiRPC3File::writeString(const std::string& buf, FT_int pos, bool skipRepos)
{
  if (!skipRepos) FT_seek(myFile,pos,SEEK_SET);

  // Keys with more than VAL_SIZE chars are slashed
  int nChar = buf.length() >= VAL_SIZE ? VAL_SIZE-1 : buf.length();
  FT_write(buf.c_str(),1,nChar,myFile);
  // RPC requires terminating null char
  FT_write("\0",1,1,myFile);
}


void FiRPC3File::writeInt16(const short val, FT_int pos, bool skipRepos)
{
  if (!skipRepos) FT_seek(myFile,pos,SEEK_SET);

  if (myOutputEndian == myMachineEndian)
    FT_write(&val,2,1,myFile);
  else
    Fi::writeSwapped(val,myFile);
}


void FiRPC3File::writeFloat(const float val, FT_int pos, bool skipRepos)
{
  if (!skipRepos) FT_seek(myFile,pos,SEEK_SET);

  if (myOutputEndian == myMachineEndian)
    FT_write(&val,4,1,myFile);
  else
    Fi::writeSwapped(val,myFile);
}


void FiRPC3File::writeDouble(const double val, FT_int pos, bool skipRepos)
{
  if (!skipRepos) FT_seek(myFile,pos,SEEK_SET);

  if (myOutputEndian == myMachineEndian)
    FT_write(&val,8,1,myFile);
  else
    Fi::writeSwapped(val,myFile);
}


// ---------- Direct read section ----------

static size_t readChars(char* buf, int nChar, FT_FILE fd,
                        bool swapStringBytes = false,
                        bool castToUpperCase = false)
{
  int i, j, c = 1;
  if (swapStringBytes)
    for (i = 0; i < nChar && c > 0; i++)
    {
      j = i%2 ? i-1 : i+1;
      if ((c = FT_getc(fd)) > 0 && c < 256)
        buf[j] = c;
      else
      {
        buf[j] = '\0';
        if (i < j)
        {
          int c2 = c == 0 ? FT_getc(fd) : 0;
          buf[i] = c2 > 0 && c2 < 256 ? c2 : 0;
        }
      }
    }
  else
    for (i = 0; i < nChar && c > 0; i++)
      if ((c = FT_getc(fd)) > 0 && c < 256)
        buf[i] = c;
      else
        buf[i] = '\0';

  if (castToUpperCase)
    for (j = 0; j < i; j++)
      buf[j] = toupper(buf[j]);

  return i;
}


std::string FiRPC3File::readString(FT_int pos, bool castToUpperCase)
{
  if (pos != skipFileRepos) FT_seek(myFile,pos,SEEK_SET);

  char buf[VAL_SIZE];
  size_t nbytes = readChars(buf,VAL_SIZE,myFile,swapStringBytes,castToUpperCase);

  // Trim the string for trailing blanks, if any
  while (nbytes > 0)
    if (buf[--nbytes] != ' ')
      return std::string(buf,nbytes+1);

  return std::string(""); // empty or blank string
}


short FiRPC3File::readInt16(FT_int pos, bool asciiField)
{
  short val = 0;
  size_t nr = 0;

  if (pos != skipFileRepos) FT_seek(myFile,pos,SEEK_SET);

  if (asciiField || myDataFormat == FiDeviceFunctionBase::ascii)
  {
    char buf[VAL_SIZE];
    nr = readChars(buf,VAL_SIZE,myFile,swapStringBytes);
    val = atoi(buf);
  }
  else if (myInputEndian == myMachineEndian)
    nr = FT_read((char*)&val, 2, 1, myFile);
  else
    nr = Fi::readSwapped((char*)&val, val, myFile);

  return nr < 2 ? 0 : val;
}


int FiRPC3File::readInt32(FT_int pos, bool asciiField)
{
  int val = 0;
  size_t nr = 0;

  if (pos != skipFileRepos) FT_seek(myFile,pos,SEEK_SET);

  if (asciiField || myDataFormat == FiDeviceFunctionBase::ascii)
  {
    char buf[VAL_SIZE];
    nr = readChars(buf,VAL_SIZE,myFile,swapStringBytes);
    val = atoi(buf);
  }
  else if (myInputEndian == myMachineEndian)
    nr = FT_read((char*)&val, 4, 1, myFile);
  else
    nr = Fi::readSwapped((char*)&val, val, myFile);

  return nr < 4 ? 0 : val;
}


float FiRPC3File::readFloat(FT_int pos, bool asciiField)
{
  float val = 0.0f;
  size_t nr = 0;

  if (pos != skipFileRepos) FT_seek(myFile,pos,SEEK_SET);

  if (asciiField || myDataFormat == FiDeviceFunctionBase::ascii)
  {
    char buf[VAL_SIZE];
    nr = readChars(buf,VAL_SIZE,myFile,swapStringBytes);
    val = (float)atof(buf);
  }
  else if (myInputEndian == myMachineEndian)
    nr = FT_read((char*)&val, 4, 1, myFile);
  else
    nr = Fi::readSwapped((char*)&val, val, myFile);

  return nr < 4 ? 0.0f : val;
}


double FiRPC3File::readDouble(FT_int pos, bool asciiField)
{
  double val = 0.0;
  size_t nr = 0;

  if (pos != skipFileRepos) FT_seek(myFile,pos,SEEK_SET);

  if (asciiField || myDataFormat == FiDeviceFunctionBase::ascii)
  {
    char buf[VAL_SIZE];
    nr = readChars(buf,VAL_SIZE,myFile,swapStringBytes);
    val = atof(buf);
  }
  else if (myInputEndian == myMachineEndian)
    nr = FT_read((char*)&val, 8, 1, myFile);
  else
    nr = Fi::readSwapped((char*)&val, val, myFile);

  return nr < 8 ? 0.0 : val;
}


void FiRPC3File::dumpAllKeysToScreen() const
{
  std::string format1(myInputEndian == FiDeviceFunctionBase::LittleEndian ? "LittleEndian" : "BigEndian");
  std::string format2(myDataFormat == FiDeviceFunctionBase::binary ? "Binary" : "Ascii");
  std::string typefile(myFileType == Time_History ? "Time_history" : "unknown");
  std::string filt(myBypassFilter ? "yes" : "no");
  std::string hfrm(myHalfFrameUse ? "yes" : "no");

  std::cout <<"\nKeys are:";
  for (const std::string& key : myKeys) std::cout <<"\n\t"<< key;
  std::cout <<"\nFormat:            "<< format2 <<" - "<< format1
	    <<"\nFileType:          "<< typefile
	    <<"\nChannels:          "<< myNumChannels
	    <<"\nDelta_T:           "<< myStep
	    <<"\nPtsPerFrame:       "<< myNumFrmPts
	    <<"\nPtsPerGroup:       "<< myNumGrpPts
	    <<"\nBypassFilter:      "<< filt
	    <<"\nHalfFrames:        "<< hfrm
	    <<"\nFrames:            "<< myNumFrames
	    <<"\nRepeats:           "<< myRepeats
	    <<"\nAverages:          "<< myAverages
	    <<"\nDataSize:          "<< myDataSize
	    <<"\nNumHeaderBlocks:   "<< myNumHeaderBlocks
	    <<"\nNumParams:         "<< myNumParams
	    <<"\nChannelScale:      "<< myChannelScale
	    <<"\nChannelMax:        "<< myMaxVal
	    <<"\nChannelMin:        "<< myMinVal
	    <<"\nChannelMapping:    "<< myPhysChan
	    <<"\nChannelPartition:  "<< myPartition << std::endl;

  std::cout <<"\nInternal parameters are:"
	    <<"\nbyteshift:         "<< (int)myByteShift
	    <<"\nmyStep:            "<< myStep
	    <<"\nPartShift:         "<< (int)myPartShift
	    <<"\nmyNumDatavals:     "<< myNumDatavals << std::endl;

  std::cout <<"\nChannels are:";
  for (const std::string& chn : myChannels) std::cout <<"\n\t"<< chn;
  std::cout <<"\n"<< std::endl;
}
