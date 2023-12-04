// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#include "FFpLib/FFpCurveData/FFpCurveDef.H"
#include "FFaLib/FFaDefinitions/FFaMsg.H"
#include "FFaLib/FFaString/FFaParse.H"


bool FFpCurveDef::readAll (std::istream& is, std::vector<FFpCurveDef*>& curves)
{
  enum
  {
    CURVE_SET = 1,
    END = 2
  };

  const char* key_words[] =
  {
    "CURVE_SET",
    "END",
    0
  };

  curves.clear();
  bool dataIsRead = false;
  while (is.good() && !dataIsRead)
  {
    FFpCurveDef* newCurve;
    std::stringstream statement;
    char keyWord[BUFSIZ];
    if (FaParse::parseFMFASCII(keyWord,is,statement,'{','}'))
      switch (FaParse::findIndex(key_words,keyWord))
      {
        case CURVE_SET:
	  newCurve = readCurve(statement,curves.size()+1);
	  if (newCurve) curves.push_back(newCurve);
	  break;

        case END:
	  dataIsRead = true;
	  break;
      }
  }

  if (!curves.empty()) return true;

  ListUI <<" *** Error: No curves to export.\n";
  return false;
}


FFpCurveDef* FFpCurveDef::readCurve (std::istream& is, const int n)
{
  FFpCurveDef* obj = new FFpCurveDef();

  while (is.good())
  {
    char keyWord[BUFSIZ];
    std::stringstream activeStatement;
    if (FaParse::parseFMFASCII(keyWord,is,activeStatement,'=',';'))
      if (!obj->parse(keyWord,activeStatement))
      {
	delete obj;
	return (FFpCurveDef*)0;
      }
  }

  // The exported curve will always shifted and scaled
  if (obj->myDft.zeroAdjustX || obj->myDft.zeroAdjustY ||
      obj->myDft.offsetX != 0.0 || obj->myDft.offsetY != 0.0 ||
      obj->myDft.scaleX != 1.0 || obj->myDft.scaleY != 1.0)
    obj->myScaleShiftDo = true;
  else
    obj->myScaleShiftDo = false;

  // Log what is going to be exported
  ListUI <<"     Channel "<< n <<": Curve "<< obj->getId();
  if (obj->getResult(0).getText() == "Physical time")
    ListUI <<" ("<< obj->getResult(1).getText();
  else
    ListUI <<" (X: "<< obj->getResult(0).getText()
	   <<"  Y: "<< obj->getResult(1).getText();
  ListUI <<") with description \""<< obj->getDescr() <<"\"\n";

  return obj;
}


bool FFpCurveDef::parse (char* keyWord, std::istream& activeStatement)
{
  enum
  {
    ID = 1,
    BASE_ID = 2,
    DESCR = 3,
    X_AXIS_RESULT = 4,
    X_AXIS_RESULT_OPER = 5,
    Y_AXIS_RESULT = 6,
    Y_AXIS_RESULT_OPER = 7,
    DFT_PERFORMED = 8,
    DFT_USING_ENTIRE_DOMAIN = 9,
    DFT_DOMAIN_START = 10,
    DFT_DOMAIN_STOP = 11,
    DFT_REMOVE_STATIC_COMPONENT = 12,
    DFT_RESAMPLE_DATA = 13,
    DFT_RESAMPLE_RATE = 14,
    ZERO_ADJUST = 15,
    SCALE_FACTOR = 16,
    OFFSET = 17,
    ZERO_ADJUST_X = 18,
    SCALE_FACTOR_X = 19,
    OFFSET_X = 20,
    ZERO_ADJUST_Y = 21,
    SCALE_FACTOR_Y = 22,
    OFFSET_Y = 23,
    DATA_ANALYSIS = 24,
    EXPORT_AUTOMATICALLY = 25
  };

  static const char* key_words[] =
  {
    "ID",
    "BASE_ID",
    "DESCR",
    "X_AXIS_RESULT",
    "X_AXIS_RESULT_OPER",
    "Y_AXIS_RESULT",
    "Y_AXIS_RESULT_OPER",
    "DFT_PERFORMED",
    "DFT_USING_ENTIRE_DOMAIN",
    "DFT_DOMAIN_START",
    "DFT_DOMAIN_STOP",
    "DFT_REMOVE_STATIC_COMPONENT",
    "DFT_RESAMPLE_DATA",
    "DFT_RESAMPLE_RATE",
    "ZERO_ADJUST",
    "SCALE_FACTOR",
    "OFFSET",
    "ZERO_ADJUST_X",
    "SCALE_FACTOR_X",
    "OFFSET_X",
    "ZERO_ADJUST_Y",
    "SCALE_FACTOR_Y",
    "OFFSET_Y",
    "DATA_ANALYSIS",
    "EXPORT_AUTOMATICALLY",
    0
  };

  std::string myValue;
  switch (FaParse::findIndex(key_words,keyWord))
    {
    case ID:
      activeStatement >> myId;
      break;

    case BASE_ID:
      activeStatement >> myBaseId;
      break;

    case DESCR:
      myDescr = FaParse::extractDescription(activeStatement);
      break;

    case X_AXIS_RESULT:
      activeStatement >> myResults[0];
      break;

    case X_AXIS_RESULT_OPER:
      myResultOpers[0] = FaParse::extractDescription(activeStatement);
      break;

    case Y_AXIS_RESULT:
      activeStatement >> myResults[1];
      break;

    case Y_AXIS_RESULT_OPER:
      myResultOpers[1] = FaParse::extractDescription(activeStatement);
      break;

    case DFT_PERFORMED:
      activeStatement >> myValue;
      myDftDo = myValue.substr(0,4) == "true" ? true : false;
      break;

    case DFT_USING_ENTIRE_DOMAIN:
      activeStatement >> myValue;
      myDft.entiredomain = myValue.substr(0,4) == "true" ? true : false;
      break;

    case DFT_DOMAIN_START:
      activeStatement >> myDft.startDomain;
      break;

    case DFT_DOMAIN_STOP:
      activeStatement >> myDft.endDomain;
      break;

    case DFT_REMOVE_STATIC_COMPONENT:
      activeStatement >> myValue;
      myDft.removeComp = myValue.substr(0,4) == "true" ? true : false;
      break;

    case DFT_RESAMPLE_DATA:
      activeStatement >> myValue;
      myDft.resample = myValue.substr(0,4) == "true" ? true : false;
      break;

    case DFT_RESAMPLE_RATE:
      activeStatement >> myDft.resampleRate;
      break;

    case ZERO_ADJUST:
    case ZERO_ADJUST_Y:
      activeStatement >> myValue;
      myDft.zeroAdjustY = myValue.substr(0,4) == "true" ? true : false;
      break;

    case SCALE_FACTOR:
    case SCALE_FACTOR_Y:
      activeStatement >> myDft.scaleY;
      break;

    case OFFSET:
    case OFFSET_Y:
      activeStatement >> myDft.offsetY;
      break;

    case ZERO_ADJUST_X:
      activeStatement >> myValue;
      myDft.zeroAdjustX = myValue.substr(0,4) == "true" ? true : false;
      break;

    case SCALE_FACTOR_X:
      activeStatement >> myDft.scaleX;
      break;

    case OFFSET_X:
      activeStatement >> myDft.offsetX;

    case DATA_ANALYSIS:
      activeStatement >> myValue;
      if (myValue.substr(0,3) == "DFT") myDftDo = true;
      break;

    case EXPORT_AUTOMATICALLY:
      activeStatement >> myValue;
      if (myValue.substr(0,5) == "false") return false;
    }

  return true;
}
