// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#include "FFpLib/FFpCurveData/FFpGraph.H"
#include "FFrLib/FFrExtractor.H"
#include "FFaLib/FFaDefinitions/FFaMsg.H"
#include "FiDeviceFunctions/FiCurveASCFile.H"
#include "FiDeviceFunctions/FiASCFile.H"
#include "FiDeviceFunctions/FiDACFile.H"
#include "FiDeviceFunctions/FiRPC3File.H"
#include <cmath>
#include <cfloat>
#include <numeric>


FFpGraph::FFpGraph (FFpCurve* curve)
{
  internal = noHeader = noXvalues = false;

  if (curve)
    curves = { curve };

  tmin = -HUGE_VAL;
  tmax = HUGE_VAL;
}


FFpGraph::FFpGraph (size_t nCurves, bool populateGraph)
{
  internal = true;
  noHeader = noXvalues = false;

  if (populateGraph)
  {
    curves.reserve(nCurves);
    for (size_t i = 0; i < nCurves; i++)
      curves.push_back(new FFpCurve());
  }
  else
    curves.resize(nCurves,NULL);

  tmin = -HUGE_VAL;
  tmax = HUGE_VAL;
}


FFpGraph::~FFpGraph ()
{
  if (internal)
    for (FFpCurve* c : curves) delete c;
}


void FFpGraph::setTimeInterval (double t0, double t1)
{
  if (t1 > t0)
  {
    tmin = t0;
    tmax = t1;
  }
}


/*!
  Load time history data from results data base for all RDB-curves.
*/

bool FFpGraph::loadTemporalData (FFrExtractor* extractor, std::string& errMsg)
{
  if (!extractor || curves.empty())
    return true; // No curves

  // Check that the extractor actually have time step data
  const double epsT = 1.0e-12;
  double firstTimeStep = extractor->getFirstTimeStep();
  double lastTimeStep  = extractor->getLastTimeStep();
#ifdef FFP_DEBUG
  std::cout <<"FFpGraph::loadTemporalData: ["
            << firstTimeStep <<","<< lastTimeStep
            <<"]\n                Time range: ["
            << tmin <<","<< tmax <<"]"<< std::endl;
#endif

  if (lastTimeStep == -HUGE_VAL)
    return true; // Extractor has no results yet
  if (lastTimeStep < tmin-epsT || firstTimeStep > tmax+epsT)
    return true; // Out-of-range

  // Find variable references and associated read operations for the curves
  bool status = true;
  size_t nCurve = 0;
  for (FFpCurve* curve : curves)
    if (curve->findVarRefsAndOpers(extractor,errMsg))
      nCurve++; // A valid curve with data
    else
      status = false;

#ifdef FFP_DEBUG
  std::cout <<"                nCurves = "<< nCurve << std::endl;
#endif
  if (nCurve < 1)
    return status; // No curves with valid data

  // Find the first time step to read data from
  bool hasTimeStep = false;
  for (FFpCurve* curve : curves)
    if (curve->notReadThisFar(lastTimeStep))
      hasTimeStep = true;

  if (!hasTimeStep)
    return status; // No new time steps since last read

  // Position the results extractor to the first time step
  if (firstTimeStep < tmin) firstTimeStep = tmin;
  if (lastTimeStep < firstTimeStep) lastTimeStep = firstTimeStep;

  double currentTime = 0.0;
  extractor->positionRDB(lastTimeStep,currentTime,true);
  tmin = currentTime;
#ifdef FFP_DEBUG
  std::cout <<"                time = "<< currentTime << std::endl;
  size_t nStep = 0;
#endif

  // Now read curve data, step by step
  double previousTime = currentTime;
  do
  {
    currentTime = extractor->getCurrentRDBPhysTime();
    if (currentTime > tmax+epsT) break;

#ifdef FFP_DEBUG
    nStep++;
#endif
    for (FFpCurve* curve : curves)
      curve->loadTemporalData(currentTime);
    previousTime = currentTime;
  }
  while (extractor->incrementRDB());
#ifdef FFP_DEBUG
  std::cout <<"                Read "<< nStep <<" step"<< std::endl;
#endif

  // Cleaning up
  for (FFpCurve* curve : curves)
    curve->unref();

  tmax = previousTime;
  return status;
}


/*!
  Load spatial data from results data base for all RDB-curves.
*/

bool FFpGraph::loadSpatialData (FFrExtractor* extractor, std::string& errMsg)
{
  if (!extractor || curves.empty())
    return true; // No curves

  // Check that the extractor actually have time step data
  double firstTimeStep = extractor->getFirstTimeStep();
  double lastTimeStep  = extractor->getLastTimeStep();

  if (lastTimeStep == -HUGE_VAL)
    return true; // Extractor has no results

  // Find variable references and associated read operations for the curves
  bool status = true;
  bool useInitialXaxis = false;
  const double epsT = 1.0e-8;
  tmin = HUGE_VAL; tmax = -HUGE_VAL;
  for (FFpCurve* curve : curves)
  {
    double t0 = curve->getTimeRange().first;
    double t1 = curve->getTimeRange().second;
    if (t1 >= t0 && t0 <= lastTimeStep+epsT && t1 >= firstTimeStep-epsT)
    {
      if (curve->findVarRefsAndOpers(extractor,errMsg))
      {
	// A valid curve with data was found, update time domain to read from
	if (t0 < tmin) tmin = t0;
	if (t1 > tmax) tmax = t1;
	curve->clear();
	if (curve->usingInitialXaxis())
	  useInitialXaxis = !noXvalues;
      }
      else
      {
	status = false;
#ifdef FFP_DEBUG
	std::cout <<"FFpGraph::loadSpatialData(): Couldn't find read operation."
		  << errMsg << std::endl;
#endif
      }
    }
#ifdef FFP_DEBUG
    else
      std::cout <<"FFpGraph::loadSpatialData(): Curve range ["<< t0 <<","<< t1
		<<"] is outside the extractor range.\n";
#endif
  }
  if (tmax == -HUGE_VAL)
    return status; // No curves with valid data

#ifdef FFP_DEBUG
  std::cout <<"FFpGraph::loadSpatialData():  Extractor range "
            << firstTimeStep <<","<< lastTimeStep <<"  Curve range "
            << tmin <<","<< tmax << std::endl;
#endif

  double currentTime = 0.0;
  if (useInitialXaxis)
  {
    // Load the X-axis values from the initial configuration
    extractor->positionRDB(firstTimeStep,currentTime);
    for (FFpCurve* curve : curves)
      if (curve->usingInitialXaxis())
        status &= curve->loadCurrentSpatialX();
  }

  // Position the results extractor to the first time step
  if (firstTimeStep < tmin) firstTimeStep = tmin;
  if (lastTimeStep  > tmax) lastTimeStep  = tmax;
  extractor->positionRDB(firstTimeStep,currentTime,true);

  // Now read curve data, step by step
  do
  {
    currentTime = extractor->getCurrentRDBPhysTime();
    if (currentTime > lastTimeStep+epsT) break;

    for (FFpCurve* curve : curves)
      status &= curve->loadSpatialData(currentTime,epsT);
  }
  while (extractor->incrementRDB());

  // Cleaning up
  for (FFpCurve* curve : curves)
  {
    curve->finalizeTimeOp();
    curve->unref();
  }

#ifdef FFP_DEBUG
  std::cout <<"FFpGraph::loadSpatialData():  Final time "<< currentTime
            << std::endl;
#endif

  return status;
}


/*!
  Write a single curve to an ASCII, DAC or one-channel RPC-file.
*/

bool FFpGraph::writeCurve (const std::string& fileName, int fileType,
			   const std::string& curveId, const std::string& descr,
			   const std::string& xTitle, const std::string& yTitle,
			   const std::string& modelName, std::string& errMsg,
			   int curveNo)
{
  if (curveNo < 1 || (size_t)curveNo > curves.size()) return false;
  if (!curves[curveNo-1]) return false;

  FiDeviceFunctionBase* fileWriter = NULL;
  switch (fileType%10)
  {
    case ASCII:
      fileWriter = new FiCurveASCFile(fileName.c_str());
      break;
    case DAC_LITTLE_ENDIAN:
      fileWriter = new FiDACFile(fileName.c_str(),
				 FiDeviceFunctionBase::LittleEndian);
      break;
    case DAC_BIG_ENDIAN:
      fileWriter = new FiDACFile(fileName.c_str(),
				 FiDeviceFunctionBase::BigEndian);
      break;
    case RPC_LITTLE_ENDIAN:
      fileWriter = new FiRPC3File(fileName.c_str(),
				  FiDeviceFunctionBase::LittleEndian,1);
      break;
    case RPC_BIG_ENDIAN:
      fileWriter = new FiRPC3File(fileName.c_str(),
				  FiDeviceFunctionBase::BigEndian,1);
      break;
    default:
      return false;
  }
  if (!fileWriter) return false;

  const std::vector<double>& x = curves[curveNo-1]->getAxisData(0);
  const std::vector<double>& y = curves[curveNo-1]->getAxisData(1);
  if (y.empty())
  {
    errMsg += "\nCurve: \"" + curveId + "\". No XY-data exported.\n";
    errMsg += "The curve is empty.";
    delete fileWriter;
    return false;
  }
  else if (!x.empty() && x.size() != y.size())
  {
    errMsg += "\nCurve: \"" + curveId + "\". No XY-data exported.\n";
    errMsg += "The axes do not have the same size.";
    delete fileWriter;
    return false;
  }

  bool isRPCfile = (fileType%10 == RPC_LITTLE_ENDIAN ||
		    fileType%10 == RPC_BIG_ENDIAN);

  const double dMin = x.empty() ? 1.0 : minIncrement(x);
  if (fileType%10 != ASCII && dMin <= 0.0)
  {
    errMsg += "\nCurve: \"" + curveId + "\". No XY-data exported.\nThe ";
    errMsg += (isRPCfile ? "RPC" : "DAC");
    errMsg += " format requires strictly increasing x-axis data.";
    delete fileWriter;
    return false;
  }

  if (isRPCfile)
  {
    if (y.size() < 2)
    {
      errMsg += "\nCurve: \"" + curveId + "\". No XY-data exported.\n";
      errMsg += "The RPC format requires at least two data points.";
      delete fileWriter;
      return false;
    }
    else if (!x.empty() && x.front() < 0.0)
      ListUI <<"===> Warning: Curve: \""<< curveId
	     <<"\". The RPC format does not allow negative x-axis data.\n"
	     <<"              Data set shifted accordingly.\n";
  }

  if (fileType < 30) fileWriter->setPrecision(fileType/10);

  if (!fileWriter->open(FiDeviceFunctionBase::Write_Only))
  {
    errMsg += "Unable to open " + fileName + "\n       for writing. ";
    errMsg += "Please check that the file is not used by another application.";
    delete fileWriter;
    return false;
  }

  bool success = true;
  fileWriter->setParent(modelName);
  fileWriter->setDescription(descr);
  fileWriter->setAxisTitle(FiDeviceFunctionBase::X,xTitle.c_str());
  fileWriter->setAxisTitle(FiDeviceFunctionBase::Y,yTitle.c_str());
  fileWriter->setStep(dMin);
  if (x.empty())
  {
    // If no X-values stored, just use the sequence 0,1,2,...,n-1
    std::vector<double> n(y.size());
    std::iota(n.begin(),n.end(),0.0);
    success = fileWriter->setData(n,y);
  }
  else
    success = fileWriter->setData(x,y);

  if (!success)
    errMsg += "\nFailed to write data for Curve \"" + curveId + "\".";

  fileWriter->close();
  delete fileWriter;
  return success;
}


/*!
  Write all curves in a graph to a single RPC-file or a multi-column ASCII-file.
*/

bool FFpGraph::writeGraph (const std::string& fileName, int fileType,
			   const std::vector<std::string>& curveId,
			   const std::vector<std::string>& cDescr,
			   const std::string& modelName, std::string& errMsg,
			   int repeats, int averages, int frmPts, int grpPts)
{
  size_t k, nCurves = curves.size();
  if (curveId.size() < nCurves) nCurves = curveId.size();
  if (cDescr.size() < nCurves) nCurves = cDescr.size();
  if (nCurves < 1) return false;

  // Check sanity of curves selected to be exported
  size_t nChannel = 0, nPoints = 0;
  bool isRPCfile = fileType%10 < RPC_LITTLE_ENDIAN ? false : true;
  double dt, dtmin = DBL_MAX;
  double maxTimeSpan = 0.0;
  std::vector<bool> okCurve(nCurves,false);
  for (k = 0; k < nCurves; k++)
  {
    if (!curves[k])
    {
      errMsg += "\nEmpty Curve: \"" + curveId[k] + "\". No XY-data exported.";
      continue;
    }
    const std::vector<double>& x = curves[k]->getAxisData(0);
    if (x.empty())
      errMsg += "\nEmpty Curve: \"" + curveId[k] + "\". No XY-data exported.";

    else if (x.size() != curves[k]->getAxisData(1).size())
    {
      errMsg += "\nCurve: \"" + curveId[k] + "\". No XY-data exported.\n";
      errMsg += "The axes do not have the same size.";
    }
    else if ((dt = minIncrement(x)) <= 0.0 && isRPCfile)
    {
      errMsg += "\nCurve: \"" + curveId[k] + "\". No XY-data exported.\nThe ";
      errMsg += "RPC format requires strictly increasing x-axis data.";
    }
    else if (dt == 0.0)
    {
      errMsg += "\nCurve: \"" + curveId[k] + "\". No XY-data exported.\nThe ";
      errMsg += "multi-column ASCII format requires the x-axis data to be ";
      errMsg += "either monotoninc increasing or decreasing.";
    }
    else if (x.size() < 2 && isRPCfile)
    {
      errMsg += "\nCurve: \"" + curveId[k] + "\". No XY-data exported.\n";
      errMsg += "The RPC format requires at least two data points.";
    }
    else
    {
      nChannel++;
      okCurve[k] = true;
      double currTimeSpan = fabs(x.back() - x.front());
      if (currTimeSpan > maxTimeSpan) maxTimeSpan = currTimeSpan;
      if (fabs(dt) < dtmin) dtmin = fabs(dt);
      if (nPoints < 1) nPoints = x.size();
      if (x.front() < 0.0 && isRPCfile)
	ListUI <<"===> Warning: Curve: \""<< curveId[k]
	       <<"\". The RPC format does not allow negative x-axis data.\n"
	       <<"              Data set shifted accordingly.\n";
      else if (x.size() != nPoints && !isRPCfile)
	ListUI <<"===> Warning: Curve: \""<< curveId[k]
	       <<(x.size() > nPoints ? "\". More" : "\". Fewer")
	       <<" curve points than in first curve in graph.\n"
	       <<"              This curve: "<< (int)x.size()
	       <<" points. First curve: "<< (int)nPoints <<" points.\n"
	       <<"              Some points of the current curve will be "
	       <<(x.size() > nPoints ? "skipped.\n" : "interpolated.\n");
    }
  }
  if (nChannel < 1) return false;

  // Create file writer for current graph
  FiDeviceFunctionBase* writer = NULL;
  switch (fileType%10)
  {
    case ASCII:
      writer = new FiASCFile("", nCurves); // include empty channels also
      break;
    case RPC_LITTLE_ENDIAN:
      writer = new FiRPC3File("", FiDeviceFunctionBase::LittleEndian, nChannel);
      break;
    case RPC_BIG_ENDIAN:
      writer = new FiRPC3File("", FiDeviceFunctionBase::BigEndian, nChannel);
      break;
    default:
      ListUI <<"===> ERROR: Invalid fileType for multi-column export: "
	     << fileType <<"\n";
      return false;
  }

  if (fileType < 30) writer->setPrecision(fileType/10);

  if (isRPCfile)
  {
    // Set some RPC-specific parameters
    FiRPC3File* rpcWriter = (FiRPC3File*)writer;
    if (repeats  > 0) rpcWriter->setRepeats(repeats);
    if (averages > 0) rpcWriter->setAverages(averages);
    if (frmPts   > 0) rpcWriter->setFramePoints(frmPts);
    if (grpPts   > 0) rpcWriter->setGroupPoints(grpPts);
  }

  // Open writer
  if (!writer->open(fileName.c_str(), FiDeviceFunctionBase::Write_Only))
  {
    errMsg += "\nUnable to open " + fileName + "\nfor writing. ";
    errMsg += "Please check that the file is not used by another application.";
    delete writer;
    return false;
  }

  // Set buffer size for ASCII output, if defined
  const char* bufszASC = getenv("FEDEM_ASCII_BUFSIZE");
  if (bufszASC) FiASCFile::bufferSize = atoi(bufszASC);

  // Set common-channel file parameters
  writer->setParent(modelName);
  writer->setTimeSpan(maxTimeSpan);
  writer->setStep(dtmin);

  // Loop over the graph's curve-set, write data to file
  bool success = true;
  for (k = 0; k < nCurves; k++)
    if (okCurve[k])
    {
      writer->setDescription(cDescr[k]);
      if (!writer->setData(curves[k]->getAxisData(0),curves[k]->getAxisData(1)))
      {
        success = false;
        errMsg += "\nFailed to write data for Curve \"" + curveId[k] + "\".";
      }
    }
    else
      writer->setEmptyChannel(cDescr[k]);

  writer->close(noHeader);
  delete writer;

  FiASCFile::bufferSize = 0;
  return success;
}


/*!
  Get the smallest increment in X (which also is != 0).
  Returns zero value if not strictly increasing or decreasing.
*/

double FFpGraph::minIncrement (const std::vector<double>& xVals)
{
  double direction = -2.0;
  double inc, minInc = DBL_MAX;
  for (size_t i = 1; i < xVals.size(); i++)
    if ((inc = xVals[i] - xVals[i-1]) != 0.0)
    {
      if (fabs(inc) < minInc) minInc = fabs(inc);
      if (direction == -2.0)
        direction = inc > 0.0 ? 1.0 : -1.0;
      else if (inc*direction < 0.0)
        direction = 0.0;
    }

  return direction*minInc;
}
