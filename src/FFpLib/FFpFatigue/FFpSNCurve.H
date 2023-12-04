// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#ifndef FFP_SNCURVE_H
#define FFP_SNCURVE_H

#include <string>
#include <vector>


class FFpSNCurve
{
public:

  enum StandardId {
    NORSOK,
    BRITISH
  };

  FFpSNCurve() {}
  virtual ~FFpSNCurve() {}

  virtual double getValue(double s) const = 0;
  virtual double getThicknessExponent() const { return 0.0; }

  bool isValid() const { return !myName.empty(); }

  const std::string& getName() const { return myName; }
  void setName(const std::string& name) { myName = name; }

  virtual StandardId getStdId() const = 0;

  std::vector<double> loga;
  std::vector<double> m;

private:
  std::string myName;
};


class FFpSNCurveNorSok : public FFpSNCurve
{
public:
  FFpSNCurveNorSok(double K = 0.0) { k = K; }
  FFpSNCurveNorSok(double loga1, double loga2, double m1, double m2 = 5.0);
  virtual ~FFpSNCurveNorSok() {}

  virtual double getValue(double s) const;
  virtual double getThicknessExponent() const { return k; }

  virtual StandardId getStdId() const { return NORSOK; }

  std::vector<double> logN0;

private:
  double k;
};


class FFpSNCurveBritish : public FFpSNCurve
{
public:
  FFpSNCurveBritish() {}
  virtual ~FFpSNCurveBritish() {}

  virtual double getValue(double s) const;

  virtual StandardId getStdId() const { return BRITISH; }
};

#endif
