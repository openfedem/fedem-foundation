// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#include "FFpLib/FFpDLL/FFpRead.h"
#include "FFpLib/FFpCurveData/FFpReadResults.H"
#include "FFrLib/FFrExtractor.H"
#include "FFrLib/FFrReadOpInit.H"
#include "FFaLib/FFaOperation/FFaBasicOperations.H"

static FFrExtractor* ourExtractor = NULL;
static DoubleVectors ourBuffer;


static std::vector<std::string> SplitString (const char* str, const char* delim)
{
  std::vector<std::string> strings;

  char* strCopy = strdup(str);
  char* p = strtok(strCopy,delim);
  for (; p; p = strtok(NULL,delim))
    strings.push_back(p);
  free(strCopy);

  return strings;
}


bool FFpReadInit (const char* fileNames)
{
  FFr::initReadOps();
  FFa::initBasicOps();

  delete ourExtractor;
  ourExtractor = new FFrExtractor();

  return ourExtractor->addFiles(SplitString(fileNames,";"),false,true);
}


void FFpReadDone ()
{
  delete ourExtractor;
  ourExtractor = NULL;

  FFr::clearReadOps();
}


int FFpReadHistories (const char* objType, const char* IDs, const char* vars,
                      double* startTime, double* endTime, bool readTime)
{
  if (!ourExtractor || !vars || !startTime || !endTime)
    return 0;

  // Get base IDs, if any
  std::vector<int> baseIds;
  if (IDs)
  {
    if (!objType) return false;
    std::vector<std::string> baseIDs = SplitString(IDs,";");
    for (const std::string& bid : baseIDs)
      baseIds.push_back(atol(bid.c_str()));
  }
  else if (objType)
    return 0;

  // Get variable description
  std::vector<std::string> varList = SplitString(vars,";");
  std::vector<FFpVar> variables(varList.size());
  for (size_t i = 0; i < varList.size(); i++)
  {
    std::vector<std::string> varDesc = SplitString(varList[i].c_str(),":");
    if (varDesc.size() >= 1) variables[i].name = varDesc[0];
    if (varDesc.size() >= 2) variables[i].type = varDesc[1];
    if (varDesc.size() >= 3) variables[i].oper = varDesc[2];
  }

  // Read from FRS
  std::string errorMsg;
  bool status = FFp::readHistories(objType, baseIds, variables,
                                   ourExtractor, *startTime, *endTime,
                                   readTime, ourBuffer, errorMsg);
  if (!errorMsg.empty()) std::cout << errorMsg << std::endl;
  if (!status || ourBuffer.empty())
    return 0;

  return ourBuffer.size()*ourBuffer.front().size();
}


int FFpReadHistory (const char* vars, double* startTime, double* endTime)
{
  return FFpReadHistories(NULL,NULL,vars,startTime,endTime,false);
}


bool FFpGetReadData (double* data, int* ncol, int* nrow)
{
  if (ourBuffer.empty() || ourBuffer.front().empty())
    return false;

  *ncol = ourBuffer.size();
  *nrow = ourBuffer.front().size();

  double* pt = data;
  for (size_t i = 0; i < ourBuffer.size(); i++, pt += (*nrow))
    memcpy(pt,&(ourBuffer[i][0]),(*nrow)*sizeof(double));

  ourBuffer.clear();
  return true;
}
