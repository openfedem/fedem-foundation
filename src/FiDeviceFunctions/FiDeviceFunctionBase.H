// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

/*!
  \file FiDeviceFunctionBase.H
  \brief Base class for external device functions.
*/

#ifndef FI_DEVICE_FUNCTION_BASE_H
#define FI_DEVICE_FUNCTION_BASE_H

#include <string>
#include <vector>
#include <array>

#include "FFaLib/FFaOS/FFaIO.H"


/*!
  \brief Base class for external device functions.
*/

class FiDeviceFunctionBase
{
protected:
  //! \brief The default constructor initialize the data members.
  explicit FiDeviceFunctionBase(const char* devicename = NULL);
  //! \brief Deleted copy constructor.
  FiDeviceFunctionBase(const FiDeviceFunctionBase&) = delete;
  //! \brief Deleted assignment operator.
  FiDeviceFunctionBase& operator=(const FiDeviceFunctionBase&) = delete;

  using Doubles = std::vector<double>;      //!< Convenience type alias
  using Strings = std::vector<std::string>; //!< Convenience type alias

public:
  //! \brief The destructor closes the device function file.
  virtual ~FiDeviceFunctionBase() { this->close(); }

  //! \brief Enumerator for endianness values.
  enum Endianness {LittleEndian, BigEndian};
  //! \brief Enumerator for the file formats.
  enum FileFormat {ascii, binary};
  //! \brief Enumerator for file opening statuses.
  enum FileStatus {Not_Loaded=-1, Not_Open=0, Read_Only=1, Write_Only=2};
  //! \brief Enumerator for interpolation and extrapolation policies.
  enum InterpolationPolicy {Constant, Linear, Previous_Value, Next_Value};

  //! \brief Opens the specified device function file for reading or writing.
  bool open(const char* devicename, FileStatus status = Read_Only);
  //! \brief Opens the file #myDatasetDevice for reading or writing.
  bool open(FileStatus status = Read_Only) { return this->open(NULL,status); }

  //! \brief Closes the device function file.
  //! \details If the file is #Write_Only, the content is flushed to disk first.
  bool close(bool noHeader = false);

  //! \brief Returns the #myDatasetDevice of this device function.
  const std::string& getDevicename() const { return myDatasetDevice; }

  //! \brief Defines the extrapolation policy to be used.
  void setExtrapolationPolicy(InterpolationPolicy policy = Constant)
  { myInterpolationPolicy[1] = policy; }
  //! \brief Defines the interpolation policy to be used.
  void setInterpolationPolicy(InterpolationPolicy policy = Linear)
  { myInterpolationPolicy[0] = policy; }

  //! \brief Returns the number of (x,y) pairs for this device.
  virtual size_t getValueCount() const = 0;
  //! \brief Calculates the function value range for this device.
  virtual void getValueRange(double& mn, double& mx) const { mn = mx = 0.0; }
  //! \brief Returns the function argument origin for this device.
  virtual double getXAxisOrigin() const { return 0.0; }

  //! \brief Gets the list channel names for a multi-channel device.
  virtual bool getChannelList(Strings&) { return true; }
  //! \brief Checks if an indexed channel is present in the device.
  virtual bool isChannelPresentInFile(int) { return true; }
  //! \brief Checks if a nameed channel is present in the device.
  virtual int  isChannelPresentInFile(const std::string&) { return 1; }
  //! \brief Sets the maximum time span for the device function.
  virtual void setTimeSpan(double) {}
  //! \brief Sets the precision to be used when writing the device to file.
  virtual void setPrecision(int) {}

  //! \brief Evaluates the function for a given \a x value.
  virtual double getValue(double x, int channel = 0,
                          bool zeroAdjust = false, double vertShift = 0.0,
                          double scaleFac = 1.0) = 0;

  //! \brief Evaluates the function at all points within specified interval.
  virtual bool getValues(double x0, double x1, Doubles& x, Doubles& y,
			 int channel, bool zeroAdjust = false,
			 double shift = 0.0, double scale = 1.0) = 0;

  //! \brief Extracts all function data point values within specified interval.
  virtual void getRawData(Doubles& x, Doubles& y,
			  double minX, double maxX, int channel = 0) = 0;

  //! \brief Extracts all function data point values within specified interval.
  virtual void getData(Doubles& x, Doubles& y, const std::string&,
		       double minX = 0.0, double maxX = -1.0)
  { this->getRawData(x,y,minX,maxX); }

  //! \brief Assigns a function value for a single-channel function.
  virtual void setValue(double x, double y) = 0;
  //! \brief Adds a complete set of x,y-pair values for a new channel.
  virtual bool setData(const Doubles& x, const Doubles& y) = 0;

  //! \brief Computes the integral of this device over the interval [0,x].
  double integrate(double x, int order = 1, int channel = 0,
		   double vertShift = 0.0, double scaleFac = 1.0);

  //! \brief Returns the current reference count for this device.
  int getRefCount() const { return myRefCount; }
  //! \brief Increments the reference count fir this device.
  int ref() { return ++myRefCount; }
  //! \brief Decrements the reference count for this device.
  //! \details If the reference count reaches zero the device is deleted.
  int unref();

  //! \brief returns the constant sampling time increment.
  double getStep() const { return myStep; }
  //! \brief returns the constant sampling frequency.
  double getFrequency() const { return 1.0/myStep; }

  //! \brief Sets the sampling increment for devices with constant sampling.
  void setStep(double aStep) { myStep = aStep; }
  //! \brief Sets the sampling frequency for devices with constant sampling.
  void setFrequency(double aFreqency) { myStep = 1.0/aFreqency; }

  //! \brief Assigns a name to the (last) channel in the device.
  virtual void setDescription(const std::string&) {}
  //! \brief Flags an empty channel for multi-channel devices.
  virtual void setEmptyChannel(const std::string&) {}
  //! \brief Sets the model file name associated with this device.
  void setParent(const std::string& modelfile) { myParent = modelfile; }

  //! \brief Returns whether this is a read-only device or not.
  bool isReadOnly() const { return myFileStatus == Read_Only; }
  //! \brief Returns the #myFileStatus of this device function.
  FileStatus getFileStatus() const { return myFileStatus; }

  //! \brief Gets the \a axis unit.
  void getAxisUnit(int axis, char* unitText, size_t n) const;
  //! \brief Gets the \a axis title.
  void getAxisTitle(int axis, char* titleText, size_t n) const;

  //! \brief Sets the \a axis unit.
  void setAxisUnit(int axis, const char* unitText);
  //! \brief Sets the \a axis title.
  void setAxisTitle(int axis, const char* titleText);

protected:
  //! \brief Calculates the function value at \a x by interpolation.
  double interpolate(double x,
		     double x0, double f0,
		     double x1, double f1) const;

  //! \brief Calculates the function value at \a x by extrapolation.
  double extrapolate(double x,
		     double x0, double f0,
		     double x1, double f1) const;

  //! \brief Convenience method for a writing text header.
  bool writeString(const char* str);
  //! \brief Convenience method for writing text headers.
  bool writeString(const char* lab, const std::string& val);

  //! \brief Reads the file content, or at least its meta data.
  virtual bool initialDeviceRead() = 0;
  //! \brief Writes the meta data for this device function to file.
  virtual bool preliminaryDeviceWrite() = 0;
  //! \brief Writes the content of this device function to file.
  virtual bool concludingDeviceWrite(bool noHeader = false) = 0;

  FT_FILE     myFile;          //!< File handle of this device function
  std::string myParent;        //!< Name of FEDEM model that owns this function
  std::string myDatasetDevice; //!< Name of file associated with #myFile

  static Endianness myMachineEndian; //!< Endianness for the running computer

  Endianness myOutputEndian; //!< Endianness assumed for binary output
  Endianness myInputEndian;  //!< Endianness assumed for binary input

  double myStep; //!< Time series information

  //! \brief Convenience struct holding curve axis information
  struct AxisInfo
  {
    std::string unit;  //!< The units of the axis values
    std::string title; //!< Name of the quantity represented by this axis
  };

  std::array<AxisInfo,2> myAxisInfo;

private:
  FileStatus myFileStatus; //!< Current file status after opening, etc.

  //! Options for interpolation and extrapolation of curve data
  std::array<InterpolationPolicy,2> myInterpolationPolicy;

  Doubles Xval; //!< Function argument values, used for integration
  Doubles Yval; //!< The function values associated with #Xval

  int myRefCount; //!< Used for reference counting
};

#endif
