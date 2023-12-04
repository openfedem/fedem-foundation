// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#ifndef FI_DAC_FILE_H
#define FI_DAC_FILE_H

#include "FiDeviceFunctionBase.H"


class FiDACFile : public FiDeviceFunctionBase
{
public:
  FiDACFile(const char* devicename = NULL, Endianness format = LittleEndian);
  virtual ~FiDACFile() {}

  virtual size_t getValueCount() const { return myNumDatavals; }
  virtual void getValueRange(double& min, double& max) const;
  virtual double getXAxisOrigin() const { return myXaxisOrigin; }

  virtual double getValue(double x, int = 0,
                          bool zeroAdjust = false, double vertShift = 0.0,
                          double scaleFac = 1.0);
  virtual bool getValues(double x0, double x1,
			 std::vector<double>& x, std::vector<double>& y,
			 int channel = 0, bool zeroAdjust = false,
			 double shift = 0.0, double scale = 1.0);
  virtual void getRawData(std::vector<double>& x, std::vector<double>& y,
			  double minX = 0.0, double maxX = -1.0, int = 0);
  virtual void setValue(double x, double y);
  virtual bool setData(const std::vector<double>& x,
		       const std::vector<double>& y);

protected:
  virtual bool initialDeviceRead();
  virtual bool preliminaryDeviceWrite();
  virtual bool concludingDeviceWrite(bool);

  void updateStatistics(double val);

private:
  double getValueAt(unsigned long pos);

  float readFloat(int pos); // DAC dump position
  short readInt16(int pos); // DAC INT no (=Dump position 33A +)
  int   readInt32(int pos); // DAC Dump position
  std::string readString(int start, int end, // DAC dump start & stop
                         int startOffset = 0, int endoffset = 0);

  void writeFloat(int pos, float val); // DAC dump position
  void writeInt16(int pos, int val);   // DAC INT no (=Dump position 33A +)
  void writeInt32(int pos, int val);   // DAC Dump position
  int writeString(int start, int end,  // DAC dump start & stop
                  const std::string& val,
                  int startOffset = 0, int endoffset = 0);

protected:
  unsigned long myNumDatavals;
  double myXaxisOrigin;

  double myFirstReadValue;

  double myLastXVal;
  double myLastYVal;

  double myMean;
  double myMS;
  double myMaxVal;
  double myMinVal;

  unsigned long maxPos;
  unsigned long minPos;

private:
  bool isDataWriteInited;
};

#endif
