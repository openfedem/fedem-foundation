// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#include <fstream>
#include <sstream>

#include "FFpLib/FFpExport/FFpBatchExport.H"
#include "FFpLib/FFpCurveData/FFpCurveDef.H"
#include "FFpLib/FFpCurveData/FFpGraph.H"
#include "FFrLib/FFrExtractor.H"
#include "FFaLib/FFaString/FFaStringExt.H"
#include "FFaLib/FFaString/FFaTokenizer.H"
#include "FFaLib/FFaString/FFaParse.H"
#include "FFaLib/FFaOS/FFaFilePath.H"
#include "FFaLib/FFaDefinitions/FFaMsg.H"
#include "FFaLib/FFaOperation/FFaBasicOperations.H"


FFpBatchExport::FFpBatchExport (const std::vector<std::string>& frsFiles)
{
  myExtractor = new FFrExtractor;
  if (myExtractor && !frsFiles.empty())
    myExtractor->addFiles(frsFiles,false,true);
}

FFpBatchExport::~FFpBatchExport ()
{
  delete myExtractor;
  FFrExtractor::releaseMemoryBlocks(true);
  for (FFpCurveDef* curve : myCurves) delete curve;
}


/*!
  Recursive function that parses a RESULT_STATUS_DATA model file entry.
*/

static bool processTokens (std::vector<std::string>& fNames,
			   const std::vector<std::string>& tokens,
			   const std::string& path)
{
  if (tokens.size() < 2)
  {
    ListUI <<"  -> Syntax error in result status data - check model file.\n";
    return false;
  }

  // First two are RSD info, use it to expand the file path
  std::string newPath = FFaFilePath::appendFileNameToPath(path,tokens[0]);
  newPath += FFaNumStr("_%04d",atoi(tokens[1].c_str()));

  for (size_t i = 2; i < tokens.size(); i++)
    if (tokens[i][0] == '<')
      // The first char is a '<', i.e., we have a sub RSD-entry
      processTokens(fNames,FFaTokenizer(tokens[i],'<','>'),newPath);
    else if (FFaFilePath::isExtension(tokens[i],"frs"))
      fNames.push_back(FFaFilePath::appendFileNameToPath(newPath,tokens[i]));

  return !fNames.empty();
}


bool FFpBatchExport::readFrsFiles (std::vector<std::string>& frsFiles,
				   const std::string& modelFile)
{
  ListUI <<"\n===> Extracting frs-file names from "<< modelFile <<"\n";

  std::ifstream is(modelFile.c_str(),std::ios::in);
  if (!is) return false;

  while (is.good())
  {
    std::stringstream statement;
    char keyWord[BUFSIZ];
    if (FaParse::parseFMFASCII(keyWord,is,statement,'{','}'))
    {
      int userID = 0;
      if (!strcmp(keyWord,"MECHANISM"))
	userID = -1;
      else if (strcmp(keyWord,"SIMULATION_EVENT"))
        continue;

      while (statement.good())
      {
	std::stringstream activeStatement;
	if (FaParse::parseFMFASCII(keyWord,statement,activeStatement,'=',';'))
        {
	  if (!strcmp(keyWord,"ID") && userID == 0)
	    activeStatement >> userID;
	  else if (!strcmp(keyWord,"RESULT_STATUS_DATA"))
	  {
	    char c; // check that the first char is a '<'
	    while (activeStatement.get(c) && isspace(c));
	    if (c != '<') return false;

	    std::string path = FFaFilePath::getBaseName(modelFile) + "_RDB";
	    if (userID > 0)
	      FFaFilePath::appendToPath(path,FFaNumStr("event_%03d",userID));
	    FFaTokenizer firstLevel(activeStatement,'<','>');
	    if (processTokens(frsFiles,firstLevel,path)) return true;
	  }
        }
      }
    }
  }

  return false;
}


bool FFpBatchExport::readCurves (const std::string& defFile)
{
  for (FFpCurveDef* curve : myCurves) delete curve;

  std::ifstream is(defFile.c_str(),std::ios::in);
  if (is) return FFpCurveDef::readAll(is,myCurves);

  ListUI <<"===> ERROR: Could not open curve definition file: "
	 << defFile <<"\n";
  return false;
}


bool FFpBatchExport::exportCurves (const std::string& path,
				   const std::string& modelFile, int format)
{
  if (!myExtractor || myCurves.empty()) return false;

  std::string ext;
  switch (format%10)
    {
    case FFpGraph::DAC_LITTLE_ENDIAN:
    case FFpGraph::DAC_BIG_ENDIAN:
      ext = ".dac";
      break;
    case FFpGraph::RPC_LITTLE_ENDIAN:
    case FFpGraph::RPC_BIG_ENDIAN:
      ext = ".rsp";
      break;
    default:
      ext = ".asc";
    }

  // Find data for all the curves
  FFpGraph graphData(myCurves.size());
  readPlottingData(graphData);

  // Loop over all curves and write their data to files
  std::string message, descr, fileName;
  bool success = true;
  for (size_t c = 0; c < myCurves.size(); c++)
  {
    descr = FFaFilePath::distillName(myCurves[c]->getDescr());
    fileName = path + FFaNumStr("C_%d_",myCurves[c]->getId()) + descr + ext;

    // Write curve data to file
    if (!graphData.writeCurve(fileName, format, myCurves[c]->getDescr(), descr,
			      "Time", "Response",
			      modelFile, message, 1+c)) success = false;
  }

  if (!message.empty()) ListUI <<"\n"<< message <<"\n";
  return success;
}


bool FFpBatchExport::exportGraph (const std::string& fName,
				  const std::string& modelFile, int format,
				  const FFpRPC3Data& rpc)
{
  if (!myExtractor || myCurves.empty()) return false;

  // Find data for all the curves
  size_t nCurve = myCurves.size();
  FFpGraph graphData(nCurve);
  readPlottingData(graphData);

  // Create filename for this graph, also define path
  std::vector<std::string> curveDesc(nCurve), curveName(nCurve);
  for (size_t c = 0; c < nCurve; c++)
  {
    curveDesc[c] = myCurves[c]->getDescr();
    curveName[c] = FFaFilePath::distillName(curveDesc[c]);
  }

  // Write all curves to a single file
  std::string message;
  bool success = graphData.writeGraph(fName, format, curveDesc, curveName,
				      modelFile, message, rpc.repeats,
				      rpc.averages, rpc.framePts, rpc.groupPts);

  if (!message.empty()) ListUI <<"\n"<< message <<"\n";
  return success;
}


void FFpBatchExport::readPlottingData (FFpGraph& rdbCurves)
{
  size_t axis, c;

  // Initialize the axis definitions for all curves
  for (c = 0; c < rdbCurves.numCurves(); c++)
    if (myCurves[c])
      for (axis = 0; axis < 2; axis++)
	rdbCurves[c].initAxis(myCurves[c]->getResult(axis),
			      myCurves[c]->getResultOper(axis), axis);

  // Actually read the frs-file curves from file
  std::string message;
  rdbCurves.loadTemporalData (myExtractor,message);

  // Replace (if needed) the wanted curves by their Fourier transform, etc.
  for (c = 0; c < rdbCurves.numCurves(); c++)
    if (myCurves[c])
    {
      if (myCurves[c]->getDftDo())
	rdbCurves[c].replaceByDFT(myCurves[c]->getDFTparameters(),
				  myCurves[c]->getDescr(), message);
      else if (myCurves[c]->getScaleShiftDo())
	rdbCurves[c].replaceByScaledShifted(myCurves[c]->getDFTparameters());
    }

  if (!message.empty()) ListUI <<"\n"<< message <<"\n";
}


bool FFpBatchExport::printPosition (const std::string& fName)
{
  if (!myExtractor || myCurves.empty()) return false;

  size_t nCurve = myCurves.size();
  FFpGraph rdbCurves(nCurve);

  std::ostream* os = &std::cout;
  if (!fName.empty())
    os = new std::ofstream(fName.c_str());

  std::string errMsg;
  for (size_t c = 0; c < rdbCurves.numCurves(); c++)
    if (myCurves[c])
    {
      // Initialize the axis definitions for this curve
      for (int axis = 0; axis < 2; axis++)
	rdbCurves[c].initAxis(myCurves[c]->getResult(axis),
			      myCurves[c]->getResultOper(axis), axis);

      // Find variable references for this curve and print its position
      if (rdbCurves[c].findVarRefsAndOpers(myExtractor,errMsg))
	rdbCurves[c].printPosition(*os);
    }

  if (!errMsg.empty())
    ListUI <<"\n"<< errMsg <<"\n";

  if (!fName.empty())
    delete os;

  return true;
}
