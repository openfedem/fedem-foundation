// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

/*!
  \file FiASCFile.H
  \brief External device function based on multi-column ASCII file.
*/

#ifndef FI_ASC_FILE_H
#define FI_ASC_FILE_H

#include "FiDeviceFunctionBase.H"


/*!
  \brief Class representing external device functions based on ASCII files.
  \details This class is used to represent a polyline (or piece-wise linear)
  function defined through a sequence of (x,y) points, where the x- and y-values
  are stored as two columns of a multi-column ASCII file (the file itself may
  contain two or more columns of data). The first column of the file is always
  the x-values, which need to be monotonically increasing, i.e.,
  <i>x<sub>i-1</sub> &le; x<sub>i</sub> &le; x<sub>i+1</sub></i>
  for all <i>i</i>.
*/

class FiASCFile : public FiDeviceFunctionBase
{
  typedef std::vector<double>      Doubles; //!< Convenience type definition
  typedef std::vector<std::string> Strings; //!< Convenience type definition

public:
  //! \brief Default constructor.
  //! \param[in] fname Name of file containing the function data.
  //! \param[in] nchan Number of channels in file (used when writing the file)
  FiASCFile(const char* fname = NULL, int nchan = 1);
  //! \brief Empty destructor.
  virtual ~FiASCFile() {}

  //! \brief Returns the number of (x,y) pairs of the function.
  virtual size_t getValueCount() const { return myValues.size(); }

  //! \brief Evaluates the function for a given \a x value.
  //! \param[in] x The function argument to evaluate for
  //! \param[in] channel Column index identifying the function values to use
  //! \param[in] zeroAdjust If \e true, the function values are shifted
  //! such that it evaluates to zero (or \a vertShift) at its first point value
  //! \param[in] vertShift Additional function-value shift
  //! \param[in] scaleFac Function-value scaling factor
  virtual double getValue(double x, int channel,
                          bool zeroAdjust, double vertShift, double scaleFac);

  //! \brief Evaluates the function at all points within specified interval.
  //! \param[in] x0 Lower bound of function argument interval
  //! \param[in] x1 Upper bound of function argument interval
  //! \param[out] x X-values (argument values) of all points within the interval
  //! \param[out] y Y-values (function values) of all points within the interval
  //! \param[in] channel Column index identifying the function values to use
  //! \param[in] zeroAdj If \e true, the function values are shifted
  //! such that it evaluates to zero (or \a shift) at its first point value
  //! \param[in] shift Additional function-value shift
  //! \param[in] scale Function-value scaling factor
  virtual bool getValues(double x0, double x1, Doubles& x, Doubles& y,
                         int channel, bool zeroAdj, double shift, double scale);

  //! \brief Extracts all function data point values within specified interval.
  //! \param[out] x X-values (argument values) of all points within the interval
  //! \param[out] y Y-values (function values) of all points within the interval
  //! \param[in] minX Lower bound of function argument interval
  //! \param[in] maxX Upper bound of function argument interval
  //! \param[in] channel Column index identifying the function values to use
  virtual void getRawData(Doubles& x, Doubles& y,
                          double minX, double maxX, int channel);
  //! \brief Extracts all function data point values within specified interval.
  //! \param[out] x X-values (argument values) of all points within the interval
  //! \param[out] y Y-values (function values) of all points within the interval
  //! \param[in] channel Channel name identifying the function values to use
  //! \param[in] minX Lower bound of function argument interval
  //! \param[in] maxX Upper bound of function argument interval
  virtual void getData(Doubles& x, Doubles& y,
                       const std::string& channel, double minX, double maxX);

  //! \brief Assigns a function value for a single-channel function.
  virtual void setValue(double x, double y);
  //! \brief Adds a channel to current file by interpolation.
  virtual bool setData(const Doubles& x, const Doubles& y);

  //! \brief Assigns a name to the (last) channel in file.
  virtual void setDescription(const std::string& desc);
  //! \brief Adds an empty channel to the file.
  virtual void setEmptyChannel(const std::string& desc);

  //! \brief Defines the output precision.
  virtual void setPrecision(int type) { outputFormat = type; }

  //! \brief Counts the number of channels (number of columns minus one).
  static int  getNoChannels(const char* fname);
  //! \brief Checks if the specified file consists of two or more columns.
  static bool isMultiChannel(FT_FILE fd, const char* fname = NULL,
                             bool rewind = false);

  //! \brief Extracts the \a list of channel names (file column labels).
  virtual bool getChannelList(Strings& list);
  //! \brief Returns the column index of the named \a channel.
  virtual int  isChannelPresentInFile(const std::string& channel);
  //! \brief Returns \e true if \a channel is a valid column index.
  virtual bool isChannelPresentInFile(int channel);

  //! \brief Reads the header section of a specified file.
  //! \param[in] fd File descriptor to read from
  //! \param[out] header Array of channel names (column labels) of the file
  //! \return Number of (non-blank) column names, negative if no header
  static int readHeader(FT_FILE fd, Strings& header);
  //! \brief Reads the next line of data from specified file.
  //! \param[in] fd File descriptor to read from
  //! \param[in] columns Column indices of the channels to read
  //! \param[out] values Array of channel values
  //! \return Always \e true, unless end of file is reached
  static bool readNext(FT_FILE fd, const std::vector<int>& columns,
                       Doubles& values);

protected:
  //! \brief Writes all function data to file.
  virtual bool concludingDeviceWrite(bool noHeader);
  //! \brief Reads the file and loads the first channel into core.
  virtual bool initialDeviceRead();
  //! \brief Dummy method doing nothing.
  virtual bool preliminaryDeviceWrite() { return true; }

private:
  //! \brief Reads the specified \a channel into core.
  int readChannel(int channel);

  //! \brief Reads one line of data from specified file.
  //! \param[in] fd File descriptor to read from
  //! \param[in] commentsOnly If \e true, only read comment lines
  //! \return Pointer to a static buffer containing the last read line
  static char* readLine(FT_FILE fd, bool commentsOnly = false);
  //! \brief Returns the number of columns in the file \a fd.
  static int countColumns(FT_FILE fd);

public:
  static size_t bufferSize; //!< Buffer size for ASCII output in KBytes

private:
  //! Mapping from x-value (time) to channel values
  typedef std::map<double,Doubles> ValuesMap;
  typedef ValuesMap::iterator      ValuesIter; //!< Convenience type definition

  Strings   chn;      //!< Channel names (columns labels)
  ValuesMap myValues; //!< Curve data, either for all or one channel
  ValuesIter vit0;    //!< Start iterator of last evaluation interval
  ValuesIter vit1;    //!< End iterator of last evaluation interval
  int myChannel;      //!< Channel (column index) that currently resides in core
  int myNumChannels;  //!< Number of columns minus 1 (column 1 is the X-values)
  int outputFormat;   //!< 0: 4-digits, 1: 8 digits, 2: 16-digits
  bool isCSVt;        //!< If \e true, this file has microsec as first column
};

#endif
