// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#ifndef FFP_SN_CURVE_LIB_H
#define FFP_SN_CURVE_LIB_H

#include <string>
#include <vector>

#include "FFaLib/FFaPatterns/FFaSingelton.H"

class FFpSNCurve;
typedef std::vector<FFpSNCurve*>          SNCurveVec;
typedef std::pair<std::string,SNCurveVec> SNCurveStd;


class FFpSNCurveLib : public FFaSingelton<FFpSNCurveLib>
{
public:
  bool readSNCurves(const std::string& filename);

  std::string getCurveId(long int stdIdx, long int curveIdx) const;

  FFpSNCurve* getCurve(long int stdIdx, long int curveIdx) const;
  FFpSNCurve* getCurve(const std::string& snStd, const std::string& name) const;

  size_t getNoCurveStds() const { return myCurves.size(); }
  size_t getNoCurves(long int stdIdx) const;

  const std::string& getCurveStd(long int stdIdx) const;
  const std::string& getCurveName(long int stdIdx, long int curveIdx) const;
  double getThicknessExp(long int stdIdx, long int curveIdx) const;

  void getCurveStds(std::vector<std::string>& snStds) const;
  void getCurveNames(std::vector<std::string>& names,
                     const std::string& snStd) const;

protected:
  FFpSNCurveLib() {}
  virtual ~FFpSNCurveLib();

  bool read(std::istream& is);

private:
  std::vector<SNCurveStd> myCurves;

  friend class FFaSingelton<FFpSNCurveLib>;
};

#endif
