// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

/*!
  \file FiDeviceFunctionFactory.H
  \brief External device function factory (functions from file).
*/

#ifndef FI_DEVICE_FUNCTION_FACTORY_H
#define FI_DEVICE_FUNCTION_FACTORY_H

#include "FFaLib/FFaPatterns/FFaSingelton.H"
#include "FFaLib/FFaOS/FFaIO.H"
#include <string>
#include <vector>
#include <map>

class FiDeviceFunctionBase;


//! \brief Enum for file opening modes.
enum FiStatus {
  IO_READ  = 1, //!< Read only access
  IO_WRITE = 2  //!< Write access
};

//! \brief Enum for file formats.
enum FiDevFormat {
  NON_EXISTING = -1, //!< Non-existing file
  UNKNOWN_FILE =  0, //!< Unknown or undefined file format
  DAC_FILE,    //!< DAC file (nCode binary format)
  ASC_FILE,    //!< Two-column ASCII format
  ASC_MC_FILE, //!< Multi-column ASCII format
  RPC_TH_FILE, //!< RPC time history fomrat (MTS binary format)
  EXT_FUNC     //!< External function (no file)
};


/*!
  \brief Singleton class for external device functions (functions from file).
*/

class FiDeviceFunctionFactory : public FFaSingelton<FiDeviceFunctionFactory>
{
protected:
  //! \brief Default constructor.
  FiDeviceFunctionFactory() : myExtFnFile(0) { myNumExtFun = 0; }
  //! \brief The destructor closes all opened files.
  virtual ~FiDeviceFunctionFactory();

public:
  //! \brief Opens the named file
  //! \param[in] fileName Full path of the file to open
  //! \param[in] format Format of the file to open
  //! \param[in] status File opening mode
  //! \param[in] inputChannel Column index for multi-channel files
  //! \param[in] littleEndian If \e true, use little endian output formatting
  //! \return 1-based index to the opened file
  int open(const std::string& fileName, FiDevFormat format = UNKNOWN_FILE,
           FiStatus status = IO_READ, int inputChannel = 0,
           bool littleEndian = false);

  //! \brief Closes the named file.
  void close(const std::string& fileName);
  //! \brief Closes the indexed file.
  void close(int fileIndex);

  //! \brief Evaluates the indexed function for the specified argument value.
  //! \param[in] fileIndex 1-based index to the function to evaluate
  //! \param[in] arg Argument value
  //! \param[out] stat Error flag (negative on error)
  //! \param[in] channel Column index for multi-channel files
  //! \param[in] zeroAd If &gt; 0, shift function value to zero for \a arg=0
  //! \param[in] shift Additional shift of function value
  //! \param[in] scale Function value scaling factor (before shifting)
  //! \return The function value
  double getValue(int fileIndex, double arg, int& stat, int channel = 0,
                  int zeroAd = 0, double shift = 0.0, double scale = 1.0) const;
  //! \brief Evaluate the indexed function for the specified argument range.
  //! \param[in] fileIndex 1-based index to the function to evaluate
  //! \param[in] x0 Start value of argument range
  //! \param[in] x1 End value of argument range
  //! \param[out] x Argument values of all points in range [x0,x1]
  //! \param[out] y Function values of all points in range [x0,x1]
  //! \param[in] channel Column index for multi-channel files
  //! \param[in] zeroAdjust If &gt; 0, shift function values to zero for \a x=0
  //! \param[in] shift Additional shift of function values
  //! \param[in] scale Function value scaling factor (before shifting)
  bool getValues(int fileIndex, double x0, double x1,
                 std::vector<double>& x, std::vector<double>& y,
                 int channel = 0, int zeroAdjust = 0,
                 double shift = 0.0, double scale = 1.0) const;

  //! \brief Returns (constant) argument increment of the indexed function.
  double getStep(int fileIndex) const;
  //! \brief Returns (constant) sampling frequency of the indexed function.
  double getFrequency(int fileIndex) const;

  //! \brief Returns the axis title of the indexed function.
  void getAxisTitle(int fileIndex, int axis, char* axisText, size_t n) const;
  //! \brief Returns the axis unit of the indexed function.
  void getAxisUnit(int fileIndex, int axis, char* unitText, size_t n) const;

  //! \brief Sets an argument/value pair for an indexed function.
  bool setValue(int fileIndex, double xVal, double yVal);

  //! \brief Sets the constant argument increment of the indexed function.
  void setStep(int fileIndex, double step) const;
  //! \brief Sets the constant sampling frequency of the indexed function.
  void setFrequency(int fileIndex, double freq) const;

  //! \brief Sets the axis title of the indexed function.
  void setAxisTitle(int fileIndex, int axis, const char* axisText) const;
  //! \brief Sets the axis unit of the indexed function.
  void setAxisUnit(int fileIndex, int axis, const char* unitText) const;

  //! \brief Returns the index of a named channel of the indexed function.
  int channelIndex(int fileIndex, const std::string& channel) const;
  //! \brief Returns the list of channel labels for the indexed function.
  bool getChannelList(int fileIndex, std::vector<std::string>& ch) const;

  //! \brief Returns the list of channel labels for the named function file.
  static bool getChannelList(const std::string& fileName,
                             std::vector<std::string>& channels);

  //! \brief Returns the file format of a named function file.
  static FiDevFormat identify(const std::string& fileName,
                              const std::string& path = "",
                              FiStatus status = IO_READ);

  //! \brief Prints a summary of the currently open files to std::cout.
  void dump() const;

  //! \brief Opens a file to read external function values from.
  //! \param[in] fileName Full path of the file to open
  //! \param[in] labels Labels "<lab1,lab2,...>" identifying the columns to use
  bool initExtFuncFromFile(const std::string& fileName,
                           const std::string& labels = "");
  //! \brief Updates the external function values from file.
  //! \param[in] nstep Number of steps to read
  bool updateExtFuncFromFile(int nstep = 1);
  //! \brief Evaluates an indexed external function.
  //! \param[in] idx External function index
  //! \param[in] stat Integration flag
  //! \return The function value
  double getExtFunc(int idx, int stat = 0) const
  {
    return this->getValue(0,0.0,stat,idx);
  }

  //! \brief Stores/extracts external function values from in-core array.
  //! \param data Solution state array
  //! \param[in] ndat Length of the solution state array
  //! \param[in] iop Option telling what to do:
  //! (0: return size, 1: store in \a data, 2: restore from \a data)
  //! \param offs Running array offset, negative value indicates an error
  void storeExtFuncValues(double* data, unsigned int ndat, int iop, int& offs);

protected:
  //! \brief Opens the named file, not already opened
  //! \param[in] fileName Full path of the file to open
  //! \param[in] inputChannel Column index for multi-channel files
  //! \param[in] format Format of the file to open
  //! \param[in] status File opening mode
  //! \param[in] useLittleEndian If \e true, use little endian output formatting
  //! \return 1-based index to the opened file
  int create(const std::string& fileName, int inputChannel,
             FiDevFormat format, FiStatus status, bool useLittleEndian);

  //! \brief Returns the device object for the indexed function.
  FiDeviceFunctionBase* getDevice(size_t fileIndex) const;

public:
  static int myNumExtFun; //!< Number of external functions in current model

private:
  //! File name to device index mapping.
  typedef std::map<std::string,size_t> FileMap;
  //! File name and column index pair.
  typedef std::pair<std::string,int>   FileChannel;
  //! File name and column to device index mapping
  typedef std::map<FileChannel,size_t> FileChnMap;

  std::vector<FiDeviceFunctionBase*> myDevices; //!< Device function container

  FileMap    myFileToIndexMap; //!< File name to device index mapping
  FileChnMap myFileChannelMap; //!< File name/channel to device index mapping

  std::vector<double> myExtValues; //!< Function values for external functions
  std::vector<int>    myExtIndex;  //!< Associated file column indices
  FT_FILE             myExtFnFile; //!< External function value file handle

  friend class FFaSingelton<FiDeviceFunctionFactory>;
};

#endif
