// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#ifndef FFP_CURVE_DEF_H
#define FFP_CURVE_DEF_H

#include <vector>
#include <string>
#include <iostream>

#include "FFpLib/FFpCurveData/FFpDFTparams.H"
#include "FFaLib/FFaDefinitions/FFaResultDescription.H"


class FFpCurveDef
{
public:
  FFpCurveDef() { myScaleShiftDo = myDftDo = false; }
  ~FFpCurveDef() {}

  static bool readAll(std::istream& is, std::vector<FFpCurveDef*>& curves);

  int getId() const { return myId; }
  int getBaseId() const { return myBaseId; }
  const std::string& getDescr() const { return myDescr; }

  const FFaResultDescription& getResult(int a) const { return myResults[a]; }
  const std::string& getResultOper(int a) const { return myResultOpers[a]; }

  bool getScaleShiftDo() const { return myScaleShiftDo; }
  bool getDftDo() const { return myDftDo; }
  const DFTparams& getDFTparameters() const { return myDft; }

private:
  static FFpCurveDef* readCurve(std::istream& is, const int n);

  bool parse(char* keyWord, std::istream& activeStatement);

  int                  myId;
  int                  myBaseId;
  std::string          myDescr;
  FFaResultDescription myResults[2];
  std::string          myResultOpers[2];
  bool                 myScaleShiftDo;
  bool                 myDftDo;
  DFTparams            myDft;
};

#endif
