// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#ifndef FI_RPC3_FILE_H
#define FI_RPC3_FILE_H

#include "FiDeviceFunctionBase.H"


class FiRPC3File : public FiDeviceFunctionBase
{
public:
  FiRPC3File();
  FiRPC3File(const char* devicename=0);
  FiRPC3File(const char* devicename, int endianFormat);
  FiRPC3File(const char* devicename, int endianFormat, int numberOfChannels);
  virtual ~FiRPC3File() {}

  // --- Read parameters
  virtual size_t getValueCount() const { return myNumDatavals; }
  virtual void getValueRange(double& min, double& max) const;

  // --- Read single file value
  virtual double getValue(double x, int channel = 0,
			  bool zeroAdjust = false, double vertShift = 0.0,
			  double scaleFac = 1.0);

  // --- Read all file values between the given range
  virtual bool getValues(double x0, double x1,
			 std::vector<double>& x, std::vector<double>& y,
			 int channel = 0, bool zeroAdjust = false,
			 double shift = 0.0, double scale = 1.0);

  // --- Read all file values
  virtual void getRawData(std::vector<double>& x, std::vector<double>& y,
			  double minX, double maxX, int channel = 0);

  virtual void getData(std::vector<double>& x, std::vector<double>& y,
		       const std::string& channel,
		       double minX = 0.0, double maxX = -1.0);

  // --- Write all file values
  virtual void setValue(double, double) {}
  virtual bool setData(const std::vector<double>& x,
		       const std::vector<double>& y);

  // --- Utilities
  virtual bool getChannelList(std::vector<std::string>& list);
  virtual int  isChannelPresentInFile(const std::string& channel);
  virtual bool isChannelPresentInFile(int channel);
  virtual void setDescription(const std::string& desc);
  virtual void setTimeSpan(double timeSpan) { myTimeSpan = timeSpan; }
  virtual void setPrecision(int type) { myDataType = type; }

  int getRepeats() const { return myRepeats; }
  void setRepeats(int rep) { myRepeats = rep; }

  int getAverages() const { return myAverages; }
  void setAverages(int avg) { myAverages = avg; }

  int getFramePoints() const { return myNumFrmPts; }
  void setFramePoints(int frm) { myNumFrmPts = frm; }

  int getGroupPoints() const { return myNumGrpPts; }
  void setGroupPoints(int grp) { myNumGrpPts = grp; }

protected:
  virtual bool initialDeviceRead();
  virtual bool preliminaryDeviceWrite();
  virtual bool concludingDeviceWrite(bool);

private:
  void commonConstructor();

  // --- Internal utilities
  bool readChannelList();
  bool initTHChannel(int channel, int action);
  int  getPartition(int channel);
  void setReadParams(int channel);

  // --- Key handling
  void   setKeyInt    (const std::string& key, const int val, const int numb);
  void   setKeyFloat  (const std::string& key, const float val, const int numb);
  void   setKeyString (const std::string& key, const std::string& val, const int numb);
  int    getKeyInt    (const std::string& key, bool& ok);
  float  getKeyFloat  (const std::string& key, bool& ok);
  std::string getKeyString (const std::string& key, bool& ok);

  // --- Direct read and write
  void   writeString(const std::string& buf, FT_int pos, bool skipRepos = false);
  void   writeDouble(const double val,  FT_int pos, bool skipRepos = false);
  void   writeFloat (const float val,   FT_int pos, bool skipRepos = false);
  void   writeInt16 (const short val,   FT_int pos, bool skipRepos = false);
  std::string readString(FT_int pos, bool castToUpperCase = false);
  double readDouble(FT_int pos, bool asciiField = false);
  float  readFloat (FT_int pos, bool asciiField = false);
  short  readInt16 (FT_int pos, bool asciiField = false);
  int    readInt32 (FT_int pos, bool asciiField = false);

  // --- Temporary test tool
  void dumpAllKeysToScreen() const;

  // enums
  enum FileType {Time_History,Matrix,Histogram,Regions,Configuration,
		 Fatigue,Road_Surface,Spectral,Start};
  enum TimeType {Drive,Response,Mult_Drive,Mult_Resp,Config_Drive,
		 Config_Resp,Peak_pick};
  enum DataType {ShortInt,Float,Double}; // Double is non-standard
  enum Action   {toRead,toWrite};

  // vars
  double myMaxVal;
  double myMinVal;

  std::vector<std::string> myKeys;
  std::vector<std::string> myChannels;
  size_t myNumDatavals;
  int myNumHeaderBlocks;
  int myNumPartitions;
  int myNumChannels;
  int myNumGrpPts;
  int myNumFrames;
  int myNumFrmPts;
  int myNumParams;
  int myDataSize;
  bool myHalfFrameUse;

  double myXaxisOrigin;
  double myFirstReadValue;
  FT_int myByteShift;
  FT_int myPartShift;
  int myFileType;
  int myDataType;
  int myDataFormat;
  int myPartition;
  int myRepeats;
  int myAverages;
  int myChannel;

  bool myBypassFilter;
  double myChannelScale;
  double myTimeSpan;
  int myPhysChan;

  struct chParams
  {
    double chScale, maxVal, minVal, X0val;
    int    part;
    FT_int partShift, byteShift;
  };

  std::map<int,chParams> parameters;

  bool swapStringBytes;
  bool stepSet;
  int  kInd;
};

#endif
