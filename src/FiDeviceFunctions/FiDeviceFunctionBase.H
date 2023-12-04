// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#ifndef FI_DEVICE_FUNCTION_BASE_H
#define FI_DEVICE_FUNCTION_BASE_H

#include <string>
#include <vector>
#include <map>

#include "FFaLib/FFaOS/FFaIO.H"


class FiDeviceFunctionBase
{
protected:
  FiDeviceFunctionBase(const char* devicename = NULL);
  FiDeviceFunctionBase(const FiDeviceFunctionBase&) = delete;
  FiDeviceFunctionBase& operator=(const FiDeviceFunctionBase&) = delete;

public:
  virtual ~FiDeviceFunctionBase() { this->close(); }

  enum Axis {X=0, Y=1};
  enum Endianness {LittleEndian, BigEndian};
  enum FileFormat {ascii, binary};
  enum FileStatus {Not_Loaded=-1, Not_Open=0, Read_Only=1, Write_Only=2};
  enum InterpolationPolicy {Constant, Linear, Previous_Value, Next_Value};

  bool open(const char* devicename, FileStatus status = Read_Only);
  bool open(FileStatus status = Read_Only) { return this->open(NULL,status); }

  bool close(bool noHeader = false);

  const std::string& getDevicename() const { return myDatasetDevice; }

  void setExtrapolationPolicy(InterpolationPolicy policy = Constant)
  { myExtrapolationPolicy = policy; }
  void setInterpolationPolicy(InterpolationPolicy policy = Linear)
  { myInterpolationPolicy = policy; }

  virtual size_t getValueCount() const = 0;
  virtual void getValueRange(double& mn, double& mx) const { mn = mx = 0.0; }
  virtual double getXAxisOrigin() const { return 0.0; }

  virtual bool getChannelList(std::vector<std::string>&) { return true; }
  virtual bool isChannelPresentInFile(int) { return true; }
  virtual int  isChannelPresentInFile(const std::string&) { return 1; }
  virtual void setTimeSpan(double) {}
  virtual void setPrecision(int) {}

  virtual double getValue(double x, int channel = 0,
                          bool zeroAdjust = false, double vertShift = 0.0,
                          double scaleFac = 1.0) = 0;

  virtual bool getValues(double x0, double x1,
			 std::vector<double>& x, std::vector<double>& y,
			 int channel, bool zeroAdjust = false,
			 double shift = 0.0, double scale = 1.0) = 0;

  virtual void getRawData(std::vector<double>& x, std::vector<double>& y,
			  double minX, double maxX, int channel = 0) = 0;

  virtual void getData(std::vector<double>& x, std::vector<double>& y,
		       const std::string&, double minX = 0.0, double maxX = -1.0)
  { this->getRawData(x,y,minX,maxX); }

  virtual void setValue(double x, double y) = 0;
  virtual bool setData(const std::vector<double>& x, const std::vector<double>& y) = 0;

  double integrate(double x, int order = 1, int channel = 0,
		   double vertShift = 0.0, double scaleFac = 1.0);

  // reference counting (if user cares...)
  int getRefCount() const { return myRefCount; }
  int ref() { return ++myRefCount; }
  int unref();

  double getStep() const { return myStep; }
  double getFrequency() const { return 1.0/myStep; }

  void setStep(double aStep) { myStep = aStep; }
  void setFrequency(double aFreqency) { myStep = 1.0/aFreqency; }

  virtual void setDescription(const std::string&) {}
  virtual void setEmptyChannel(const std::string&) {}
  void setParent(const std::string& modelfile) { myParent = modelfile; }

  bool isReadOnly() const { return myFileStatus == Read_Only; }
  FileStatus getFileStatus() const { return myFileStatus; }

  void getAxisUnit(int axis, char* unitText, size_t n) const;
  void getAxisTitle(int axis, char* titleText, size_t n) const;

  void setAxisUnit(int axis, const char* unitText);
  void setAxisTitle(int axis, const char* titleText);

protected:
  // TODO: Use an iterator on the data set instead of x0 etc...
  //       This enables spline, quadratic or any other
  //       interpolation/extrapolation policy

  double interpolate(double x,
		     double x0, double f0,
		     double x1, double f1) const;

  double extrapolate(double x,
		     double x0, double f0,
		     double x1, double f1) const;

  // Convenience methods for writing text headers, etc.
  bool writeString(const char* str);
  bool writeString(const char* lab, const std::string& val);

  virtual bool initialDeviceRead() = 0;
  virtual bool preliminaryDeviceWrite() = 0;
  virtual bool concludingDeviceWrite(bool noHeader = false) = 0;

  // file data
  FT_FILE     myFile;
  std::string myParent;
  std::string myDatasetDevice;

  static Endianness myMachineEndian;
  Endianness myOutputEndian;
  Endianness myInputEndian;

  // time series information
  double myStep;

  struct axisInfo
  {
    std::string unit;
    std::string title;
  };

  std::map<int,axisInfo> myAxisInfo;

private:
  FileStatus          myFileStatus;
  InterpolationPolicy myInterpolationPolicy;
  InterpolationPolicy myExtrapolationPolicy;

  // used for integration
  std::vector<double> Xval;
  std::vector<double> Yval;

  // reference counting
  int myRefCount;
};

#endif
