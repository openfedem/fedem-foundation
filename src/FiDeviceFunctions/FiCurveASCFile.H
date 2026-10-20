// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

/*!
  \file FiCurveASCFile.H
  \brief External device function based on single-column ASCII file.
*/

#ifndef FI_CURVE_ASC_FILE_H
#define FI_CURVE_ASC_FILE_H

#include "FiDeviceFunctionBase.H"


/*!
  \brief ASCII file for curve plotting.

  \details
  This class is very much like FiASCFile, but uses vectors to store data
  instead of maps. This allows for export/import of curves with any shape,
  and not only for strictly growing x-values.

  There will be no interpolation, scaling or shifting what so ever.
  Just raw data in and out.
*/

class FiCurveASCFile : public FiDeviceFunctionBase
{
public:
  //! \brief Default constructor.
  explicit FiCurveASCFile(const char* devicename = NULL);

  virtual size_t getValueCount() const { return myXValues.size(); }

  virtual double getValue(double, int, bool, double, double) { return 0.0; }
  virtual bool getValues(double, double, Doubles& x, Doubles& y,
                         int, bool, double, double);
  virtual void getRawData(Doubles& x, Doubles& y, double, double, int);

  virtual void setValue(double x, double y);
  virtual bool setData(const Doubles& x, const Doubles& y);

  virtual void setPrecision(int type) { outputFormat = type; }

protected:
  virtual bool concludingDeviceWrite(bool);
  virtual bool initialDeviceRead();
  virtual bool preliminaryDeviceWrite() { return true; }

private:
  Doubles myXValues; //!< X-axis curve data values
  Doubles myYValues; //!< Y-axis curve data values
  int  outputFormat; //!< 0: 4-digits, 1: 8 digits, 2: 16-digits
};

#endif
