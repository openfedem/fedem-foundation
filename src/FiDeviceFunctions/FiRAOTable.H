// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#ifndef FI_RAO_TABLE_H
#define FI_RAO_TABLE_H

#include <map>
#include <vector>
#include <iostream>


enum RAOdof { SURGE, SWAY, HEAVE, ROLL, PITCH, YAW, NDOF };

struct RAOcomp
{
  double ampl, phase;
  RAOcomp(double a = 0.0, double p = 0.0) : ampl(a), phase(p) {}

  friend std::istream& operator>>(std::istream& is, RAOcomp& rao)
  { is >> rao.ampl >> rao.phase; return is; }
  friend std::ostream& operator<<(std::ostream& os, const RAOcomp& rao)
  { os << rao.ampl <<' '<< rao.phase; return os; }
};

struct RAOentry
{
  RAOcomp dof[NDOF];
  RAOcomp& Surge;
  RAOcomp& Sway;
  RAOcomp& Heave;
  RAOcomp& Roll;
  RAOcomp& Pitch;
  RAOcomp& Yaw;

  RAOentry() : Surge(dof[SURGE]), Sway(dof[SWAY]), Heave(dof[HEAVE]),
               Roll(dof[ROLL]), Pitch(dof[PITCH]), Yaw(dof[YAW])
  {
    Surge = Sway = Heave = Roll = Pitch = Yaw = 0.0;
  }

  const RAOcomp& operator[](int i) const { return dof[i]; }

  RAOentry& operator=(const RAOentry& rao)
  { for (int i = 0; i < NDOF; i++) dof[i] = rao.dof[i]; return *this; }

  friend std::istream& operator>>(std::istream& is, RAOentry& rao)
  { for (int i = 0; i < NDOF; i++) is >> rao.dof[i]; return is; }
  friend std::ostream& operator<<(std::ostream& os, const RAOentry& rao)
  { for (int i = 0; i < NDOF; i++) os <<'\t'<< rao.dof[i]; return os; }
};

typedef std::map<double,RAOentry> RAOTable;
typedef RAOTable::const_iterator  RAOIter;


struct WaveComp
{
  double A, omega, eps;
  WaveComp() { A = omega = eps = 0.0; }
  WaveComp(double a, double o, double e) : A(a), omega(o), eps(e) {}
  WaveComp& operator=(const WaveComp& wc)
  { A = wc.A; omega = wc.omega; eps = wc.eps; return *this; }
};

typedef std::vector<WaveComp> FiWave;


class FiRAOTable
{
public:
  size_t size() const { return myData.size(); }

  bool getValues(std::vector<double>& x,
                 std::vector<RAOcomp>& y, RAOdof idof) const;

  bool readDirection(std::istream& is, int angle = 0);

private:
  RAOcomp getValue(double freq, RAOdof idof) const;

  void applyToWave(FiWave& wave, RAOdof d) const;

public:
  static bool getDirections(const std::string& RAOfile,
                            std::vector<int>& angles);

  static bool applyRAO(const std::string& RAOfile, int direction,
                       const FiWave& wave, std::vector<FiWave>& motion);

  static bool applyRAO(const std::string& RAOfile, int direction,
                       int nRw, int nComp, const double* waveData,
                       std::vector<FiWave>& motion);

  static bool applyRAO(const std::string& RAOfile, int direction,
                       int nRw, int nComp, const double* waveData,
                       double** motionData);

  static bool extractMotion(const std::vector<FiWave>& motion,
                            int dof, double* motionData);

  friend std::istream& operator>>(std::istream& is, FiRAOTable& rao);
  friend std::ostream& operator<<(std::ostream& os, const FiRAOTable& rao);

private:
  RAOTable myData;
};

#endif
