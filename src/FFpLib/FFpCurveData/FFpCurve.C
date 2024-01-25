// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#include <algorithm>
#include <numeric>
#include <cmath>

#include "FFpLib/FFpCurveData/FFpCurve.H"
#include "FFpLib/FFpCurveData/FFpFourier.H"
#include "FFpLib/FFpCurveData/FFpDFTparams.H"
#include "FFpLib/FFpFatigue/FFpFatigue.H"
#include "FFrLib/FFrExtractor.H"
#include "FFrLib/FFrVariableReference.H"
#include "FFaLib/FFaDefinitions/FFaResultDescription.H"
#include "FFaLib/FFaOperation/FFaOpUtils.H"
#include "FFaLib/FFaString/FFaStringExt.H"
#include "FFaMathExpr/FFaMathExprFactory.H"
#include "FiDeviceFunctions/FiDeviceFunctionFactory.H"
#include "FiDeviceFunctions/FiCurveASCFile.H"
#include "FiDeviceFunctions/FiASCFile.H"
#include "FiDeviceFunctions/FiDACFile.H"
#include "FiDeviceFunctions/FiRPC3File.H"


/*!
  The copy constructor only copies the curve data, not reader data.
*/

FFpCurve::FFpCurve (const FFpCurve& curve)
{
  dataChanged = false;
  needRainflow = true;
  useInitialXaxis = false;
  beamEndFlag = -1;

  lastKey = -HUGE_VAL;
  lastX = 0;

  for (int axis = 0; axis < N_AXES; axis++)
  {
    reader[axis].resize(curve.reader[axis].size());
    rdOper[axis] = (std::string*)NULL;
    points[axis] = curve.points[axis];
  }

  Xrange      = curve.Xrange;
  timeSamples = curve.timeSamples;
  timeRange   = curve.timeRange;
  timeOper    = curve.timeOper;
}


FFpCurve::~FFpCurve ()
{
  for (int axis = 0; axis < N_AXES; axis++)
    if (reader[axis].size() > 1)
      for (PointData& read : reader[axis])
        delete read.rDescr;
}


void FFpCurve::clear ()
{
  needRainflow = false;
  cycles.clear();

  Xrange = 0.0;
  lastKey = -HUGE_VAL;
  lastX = 0;

  for (int axis = 0; axis < N_AXES; axis++)
    if (!points[axis].empty())
    {
      dataChanged = true;
      points[axis].clear();
    }
}


void FFpCurve::resize (size_t nSpatialPoints)
{
  dataChanged = false;
  needRainflow = false;
  cycles.clear();

  useInitialXaxis = false;
  beamEndFlag = -1;

  lastKey = -HUGE_VAL;
  lastX = 0;

  for (int axis = 0; axis < N_AXES; axis++)
  {
    if (reader[axis].size() > 1)
      for (PointData& read : reader[axis])
        delete read.rDescr;
    reader[axis].resize(nSpatialPoints == 0 && axis == Y ? 1 : nSpatialPoints);
    rdOper[axis] = (std::string*)NULL;
    points[axis].clear();
  }

  Xrange      = 0.0;
  timeSamples = 0;
  timeRange   = { 0.0, 0.0 };;
  timeOper    = None;
}


void FFpCurve::unref (bool clearReadOp)
{
  for (int axis = 0; axis < N_AXES; axis++)
    for (PointData& read : reader[axis])
    {
      if (read.readOp) read.readOp->unref();
      if (clearReadOp) read.readOp = (FFaOperation<double>*)NULL;
    }
}


void FFpCurve::setDataChanged ()
{
  dataChanged = true;
  needRainflow = true;
  if (points[X].empty())
    Xrange = 0.0;
  else
    Xrange = points[X].back() - points[X].front();
}


bool FFpCurve::clipX (double Xmin, double Xmax)
{
  if (points[X].empty()) return false;
  size_t iMin = 0, iMax = points[X].size();
  while (iMin < points[X].size() && points[X][iMin] < Xmin) iMin++;
  while (iMax > iMin && points[X][iMax-1] > Xmax) iMax--;
  if (iMax-iMin == points[X].size()) return false;

  if (iMax <= iMin)
  {
    points[X].clear();
    points[Y].clear();
  }
  else if (iMax < points[X].size())
  {
    points[X].erase(points[X].begin()+iMax,points[X].end());
    points[Y].erase(points[Y].begin()+iMax,points[Y].end());
  }
  if (iMin > 0 && iMin < iMax)
  {
    points[X].erase(points[X].begin(),points[X].begin()+iMin);
    points[Y].erase(points[Y].begin(),points[Y].begin()+iMin);
  }

  this->setDataChanged();
  return true;
}


bool FFpCurve::checkAxesSize ()
{
  if (points[X].empty() && !points[Y].empty())
  {
    // No X-axis values provided - just create an identity vector
    points[X].resize(points[Y].size());
    std::iota(points[X].begin(),points[X].end(),0.0);
  }
  else if (points[X].size() != points[Y].size())
    return false;

  return true;
}


bool FFpCurve::initAxis (const FFaResultDescription& desc,
			 const std::string& oper, int axis)
{
  if (reader[axis].size() != 1) return false;

  reader[axis].front().rDescr = (FFaResultDescription*)&desc;
  reader[axis].front().varRef = (FFrVariableReference*)NULL;
  reader[axis].front().readOp = (FFaOperation<double>*)NULL;
  rdOper[axis] = (std::string*)&oper;
  return true;
}


bool FFpCurve::initAxes (const std::vector<FFaResultDescription>& xdesc,
			 const std::vector<FFaResultDescription>& ydesc,
			 const std::string& xOper, const std::string& yOper,
			 const std::pair<double,double>& tRange,
			 const std::string& tOper, short int end1)
{
  if (reader[X].size() > reader[Y].size()) return false;

  // Special x-axis variables for spatial curves
  bool useCurveLength = xOper.find("Length") != std::string::npos;
  std::string xVar(useCurveLength ? "Curve length" : "Position matrix");
  std::string xTyp(useCurveLength ? "SCALAR" : "TMAT34");
  static std::string NoOp("None");

  if (xdesc.size() == 2*ydesc.size() && end1 >= 0)
  {
    // Special treatment for element results where the X-axis value
    // is taken from separate objects (typically Triads)
    beamEndFlag = yOper.size() == 1 ? end1 : -1;
    for (size_t i = 0; i < reader[X].size() && i < xdesc.size(); i++)
    {
      reader[X][i].rDescr = new FFaResultDescription(xdesc[i]);
      reader[X][i].rDescr->varDescrPath = {xVar};
      reader[X][i].rDescr->varRefType = xTyp;
      reader[Y][i].rDescr = new FFaResultDescription(ydesc[i/2]);
      size_t endPos = reader[Y][i].rDescr->varDescrPath.front().find("end ");
      if ((short int)(i%2) == end1)
	reader[Y][i].rDescr->varDescrPath.front().replace(endPos,5,"end 1");
      else // end 2
	reader[Y][i].rDescr->varDescrPath.front().replace(endPos,5,"end 2");
      for (int axis = 0; axis < N_AXES; axis++)
      {
        reader[axis][i].varRef = (FFrVariableReference*)NULL;
        reader[axis][i].readOp = (FFaOperation<double>*)NULL;
      }
    }
  }
  else for (int axis = 0; axis < N_AXES; axis++)
    for (size_t i = 0; i < reader[X].size() && i < ydesc.size(); i++)
    {
      // Let both axes refer to same object, only variable path is different.
      // Note that rDescr here is allocated internally, whereas that is not
      // the case for the temporal curves (mostly for efficiency reasons).
      if (reader[axis][i].rDescr)
	*reader[axis][i].rDescr = ydesc[i];
      else
	reader[axis][i].rDescr = new FFaResultDescription(ydesc[i]);

      if (axis == X)
      {
	reader[X][i].rDescr->varDescrPath = {xVar};
	reader[X][i].rDescr->varRefType = xTyp;
      }
      reader[axis][i].varRef = (FFrVariableReference*)NULL;
      reader[axis][i].readOp = (FFaOperation<double>*)NULL;
    }

  useInitialXaxis = xOper.find("Initial ") == 0;
  rdOper[X] = (std::string*)(useCurveLength ? &NoOp : &xOper);
  rdOper[Y] = (std::string*)&yOper;
  timeRange = tRange;
  timeOper  = TimeOpEnum(tOper.c_str());
  if (timeOper == None)
    timeRange.second = timeRange.first;

  return reader[X].size() > 1 && reader[X].size() <= ydesc.size();
}


int FFpCurve::getSpatialXaxisObject (size_t i) const
{
  if (i < reader[X].size() && reader[X][i].rDescr)
    return reader[X][i].rDescr->baseId;

  return 0;
}


bool FFpCurve::notReadThisFar (double& lastTimeStep) const
{
  if (lastTimeStep <= lastKey) return false;

  for (int axis = 0; axis < N_AXES; axis++)
    if (!reader[axis].empty())
    {
      if (!reader[axis].front().varRef) return false;
      if (!reader[axis].front().readOp) return false;
    }

  lastTimeStep = lastKey;
  return true;
}


bool FFpCurve::findVarRefsAndOpers (FFrExtractor* extractor,
				    std::string& errMsg)
{
  // Check if curve is completely defined
  size_t nVar;
  int axis, fstAxis = rdOper[X] ? X : Y;
  for (nVar = 0, axis = fstAxis; axis < N_AXES; axis++)
    for (const PointData& read : reader[axis])
      if (read.rDescr && !read.rDescr->empty())
      {
        if (FFaOpUtils::hasOpers(read.rDescr->varRefType))
          nVar++;
        else
          errMsg += "\nError: No unary operations defined for "
            + read.rDescr->varRefType;
      }

  if (nVar < reader[X].size() + reader[Y].size()) return false;

  // Find the variable references for the curve
  FFrEntryBase* entry;
  for (nVar = 0, axis = fstAxis; axis < N_AXES; axis++)
    for (PointData& read : reader[axis])
      if ((entry = extractor->search(*read.rDescr)))
      {
	if (entry->isVarRef())
	{
	  nVar++;
	  read.varRef = (FFrVariableReference*)entry;
	}
	else
	  errMsg += "\nError: Result item " + read.rDescr->getText()
	    + " is not a variable reference.";
      }
      else
      {
	errMsg += "\nError: Could not find result item: "
	  + read.rDescr->getText();
	// If the X-axis is plotting physical time and this is a temporal curve,
	// then clear the Y-axis reader such that it will be identically zero
	if (axis == Y && reader[X].size() == 1 && reader[Y].size() == 1)
	  if (reader[X].front().rDescr->isTime())
	    reader[Y].clear();
      }

  if (nVar < reader[X].size() + reader[Y].size())
  {
    for (axis = fstAxis; axis < N_AXES; axis++)
      for (PointData& read : reader[axis])
        read.varRef = (FFrVariableReference*)NULL;
    return false;
  }

  // Find the read operations for the curve
  for (nVar = 0, axis = fstAxis; axis < N_AXES; axis++)
  {
    bool initialX = axis == X && useInitialXaxis && *rdOper[axis] != "None";
    std::string scalarOper(initialX ? rdOper[axis]->substr(8) : *rdOper[axis]);
    for (PointData& read : reader[axis])
      if (FFaOpUtils::getUnaryConvertOp(read.readOp,
                                        read.varRef->getReadOperation(),
                                        scalarOper))
        nVar++;
      else
        errMsg += "\nError: Cannot read data for result item: "
               + read.rDescr->getText() + ", " + *rdOper[axis];
  }

  timeSamples = 0;
  if (nVar == reader[X].size() + reader[Y].size()) return true;

  this->unref(true);
  return false;
}


std::ostream& operator<< (std::ostream& os, const FFpCurve::PointData& data)
{
  if (!data.varRef) return os;

  std::string var = data.rDescr->getText();
  if (var == "Physical time") return os;

  os <<"\n"<< var <<"\n";
  data.varRef->printPosition(os);
  return os;
}


void FFpCurve::printPosition (std::ostream& os) const
{
  for (int a = 0; a < N_AXES; a++)
    if (reader[a].size() == 1)
      os << reader[a].front();
}


bool FFpCurve::loadTemporalData (double currentTime)
{
  if (lastKey >= currentTime) return true;

  for (int a = 0; a < N_AXES; a++)
    if (reader[a].size() == 1)
    {
      if (!reader[a].front().readOp || !reader[a].front().varRef) return false;
      if (!reader[a].front().varRef->hasDataForCurrentKey()) return true;
    }
    else if (reader[a].size() > 1)
      return false;

  const size_t blockSize = 501;

  int axis = reader[X].empty() ? Y : X;
  for (; axis < N_AXES; axis++)
  {
    // (Re)allocate curve points
    std::vector<double>& values = points[axis];
    if (values.capacity() < values.size()+1)
      values.reserve(values.capacity()+blockSize);

    // Read curve point value
    if (reader[axis].empty())
      values.push_back(0.0);
    else
    {
      double value = 0.0;
      reader[axis].front().readOp->invoke(value);
      reader[axis].front().readOp->invalidate();
      values.push_back(value == HUGE_VAL ? 0.0 : value);
    }
  }

  lastKey = currentTime;
  this->setDataChanged();
  return true;
}


bool FFpCurve::loadSpatialData (double currentTime, const double epsT)
{
  // Check whether current time step is within the time range of this curve
  if (currentTime < timeRange.first-epsT) return true;
  if (currentTime > timeRange.second+epsT) return true;

#ifdef FFP_DEBUG
  std::cout <<"FFpCurve::loadSpatialData(): t="<< currentTime
            <<"\tTime range "<< timeRange.first <<","<< timeRange.second
            <<" op="<< timeOper << std::endl;
#endif

  double value = 0.0;
  if (timeOper == Min)
    value = HUGE_VAL;
  else if (timeOper == Max)
    value = -HUGE_VAL;

  // Allocate curve points, each point has its own read operation in this method
  for (int axis = useInitialXaxis; axis < N_AXES; axis++)
  {
    if (reader[axis].size() < 2) return false;
    points[axis].resize(reader[axis].size(),value);
  }
  if (!useInitialXaxis)
    if (points[X].size() < points[Y].size()) return false;

  // Now read the curve data, point-by-point
  for (size_t i = 0; i < points[Y].size(); i++)
    for (int axis = useInitialXaxis; axis < N_AXES; axis++)
    {
      if (!reader[axis][i].readOp) return false;
      if (!reader[axis][i].varRef) return false;

      if (reader[axis][i].varRef->hasDataForCurrentKey())
      {
        double scale = axis == Y && (short int)(i%2) == beamEndFlag ? -1.0 : 1.0;
        if (reader[axis][i].readOp->invoke(value))
          switch (timeOper) {
          case Min:
            if (scale*value < points[axis][i])
              points[axis][i] = scale*value;
            break;
          case Max:
            if (scale*value > points[axis][i] && scale*value < HUGE_VAL)
              points[axis][i] = scale*value;
            break;
          case AMax:
            if (fabs(value) > fabs(points[axis][i]) && value < HUGE_VAL)
              points[axis][i] = scale*value;
            break;
          case Mean:
            if (value != HUGE_VAL)
              points[axis][i] += scale*value;
            break;
          case RMS:
            if (value != HUGE_VAL)
              points[axis][i] += value*value;
            break;
          default:
            points[axis][i] = value == HUGE_VAL ? 0.0 : scale*value;
          }
        reader[axis][i].readOp->invalidate();
      }
      else if (axis == X)
        break;
    }

  timeSamples++;
  this->setDataChanged();
  return true;
}


bool FFpCurve::loadCurrentSpatialX ()
{
  if (reader[X].size() < 2) return false;
  points[X].resize(reader[X].size(),0.0);

  // Read the X-axis data, point-by-point
  for (size_t i = 0; i < points[X].size(); i++)
  {
    if (!reader[X][i].readOp) return false;
    if (!reader[X][i].varRef) return false;
    if (!reader[X][i].varRef->hasDataForCurrentKey()) return false;

    reader[X][i].readOp->invoke(points[X][i]);
    reader[X][i].readOp->invalidate();
  }

  this->setDataChanged();
  return true;
}


void FFpCurve::finalizeTimeOp ()
{
  if (timeOper >= Mean && timeSamples > 0)
    for (int axis = useInitialXaxis; axis < N_AXES; axis++)
      for (double& val : points[axis])
      {
        val /= (double)timeSamples;
	if (timeOper == RMS)
          val = sqrt(val);
      }

  timeSamples = 0;
}


bool FFpCurve::loadFileData (const std::string& filePath,
			     const std::string& channel, std::string& errMsg,
			     double minX, double maxX)
{
  if (filePath.size() < 1) return false;

  FiDeviceFunctionBase* reader;
  switch (FiDeviceFunctionFactory::identify(filePath))
  {
    case RPC_TH_FILE:
      reader = new FiRPC3File(filePath.c_str());
      break;
    case ASC_MC_FILE:
      reader = new FiASCFile(filePath.c_str());
      break;
    case DAC_FILE:
      reader = new FiDACFile(filePath.c_str());
      break;
    default:
      reader = new FiCurveASCFile(filePath.c_str());
  }
  if (!reader) return false;

  bool success = false;
  if (reader->open())
  {
    reader->getData(points[X], points[Y], channel, minX, maxX);
    reader->close();

    if (points[X].size() != points[Y].size())
      errMsg += "\nError reading curve file: " + filePath
	+       "\nThe axes do not have the same size.";
    else if (points[X].empty())
      errMsg += "\nCould not find curve data on file: " + filePath;
    else
      success = true;

    if (minX < maxX && success)
      this->clipX(minX,maxX);
    this->setDataChanged();
  }
  else
    errMsg += "\nUnable to open file " + filePath + " for reading.";

  delete reader;
  return success;
}


bool FFpCurve::combineData (int ID, const std::string& expression,
			    const std::vector<FFpCurve*>& compCurves,
			    const char** compNames, bool clipXdomain,
			    std::string& message)
{
  size_t i, j, nPoints = 0;
  size_t nc = compCurves.size();
  double minX = 0.0, maxX = 0.0;
  std::vector<bool> sameX(nc,false);
  for (i = 0; i < nc; i++)
    if (compCurves[i] && !compCurves[i]->empty())
    {
      const std::vector<double>& xci = compCurves[i]->getAxisData(X);
      if (xci.front() > xci.back() && compCurves[i]->reversePoints())
	message += "Reversing curve points of curve component " +
          std::string(compNames[i]) + ".\n";

      // Find X-interval that covers all curve components
      if (!nPoints)
      {
	minX = xci.front();
	maxX = xci.back();
      }
      else if (clipXdomain && minX < maxX)
      {
	if (xci.front() > minX)
	  minX = xci.front();
	if (xci.back() < maxX)
	  maxX = xci.back();
      }
      else
      {
	if (xci.front() < minX)
	  minX = xci.front();
	if (xci.back() > maxX)
	  maxX = xci.back();
      }

      // Choose X-axis values from the curve having the most points
      if (!nPoints || xci.size() > nPoints)
      {
	sameX[i] = true;
	points[X] = xci;
	nPoints = points[X].size();
	for (j = 0; j < i; j++)
	  sameX[j] = false;
      }
      else
	sameX[i] = (points[X] == xci);
    }

  if (!nPoints) return false;

  // Fetch or create the math expression tree
  if (FFaMathExprFactory::instance()->create(ID,expression,nc,compNames) <= 0)
  {
    message += "Invalid expression \'" + expression + "\'.\n";
    return false;
  }

  // Widen the X-domain a bit, to ensure we don't miss the first or last point
  // due to round off errors in the data
  double epsX = 0.01*(maxX-minX)/(double)nPoints;
  minX -= epsX;
  maxX += epsX;

  // Evaluate the expression at each curve point, using linear interpolation
  // for all curve components not have the same X-axis values as this one
  int error = 0;
  double* args = new double[nc];
  std::vector<bool> monotonic(nc,true);
  points[Y].resize(nPoints,0.0);
  for (j = 0; j < nPoints; j++)
  {
    if (points[X][j] < minX || points[X][j] > maxX) continue;

    for (i = 0; i < nc; i++)
      if (!compCurves[i] || compCurves[i]->empty())
        args[i] = 0.0;
      else if (sameX[i])
        args[i] = compCurves[i]->points[Y][j];
      else if (!clipXdomain || compCurves[i]->inDomain(points[X][j]))
      {
        bool tmp = true;
        args[i] = compCurves[i]->getValue(points[X][j],/*monotonic[i]*/tmp);
        monotonic[i] = tmp; // Workaround for VC 12.0 compiler bug(?)
      }
      else
        args[i] = 0.0;

    points[Y][j] = FFaMathExprFactory::instance()->getValue(ID,args,error);
  }

  bool ok = true;
  for (i = 0; i < nc; i++)
    if (!monotonic[i])
    {
      message += "Curve argument " + std::string(compNames[i]) +
        " does not have monotonically increasing abscissa values.\n";
      ok = false;
    }

  delete[] args;
  this->clipX(minX,maxX);
  this->setDataChanged();
  return ok;
}


bool FFpCurve::inDomain (double x) const
{
  double minX = points[X].front();
  double maxX = points[X].back();
  if (x > minX && x < maxX) return true;

  double epsX = 0.01*(maxX-minX)/(double)points[X].size();
  return x > minX-epsX && x < maxX+epsX;
}


double FFpCurve::getValue (double x, bool& monotonicX) const
{
  size_t n = points[X].size();
  if (n == 0 || n > points[Y].size())
    return 0.0;

  else if (n == 1)
    return points[Y].front();

  else if (x < points[X].front())
  {
    // Extrapolate before first point
    double x0 = points[X][0];
    double x1 = points[X][1];
    double y0 = points[Y][0];
    double y1 = points[Y][1];
    return y0 + (x-x0)*(y1-y0)/(x1-x0);
  }
  else if (x > points[X].back())
  {
    // Extrapolate after last point
    double x0 = points[X][n-2];
    double x1 = points[X][n-1];
    double y0 = points[Y][n-2];
    double y1 = points[Y][n-1];
    return y0 + (x-x0)*(y1-y0)/(x1-x0);
  }

  // The given x-value is within the domain
  if (lastX >= n || x < points[X][lastX])
    lastX = 0;

  for (; lastX < n; lastX++)
    if (x == points[X][lastX])
      return points[Y][lastX++];
    else if (x < points[X][lastX+1])
    {
      // Interpolate
      double x0 = points[X][lastX];
      double x1 = points[X][lastX+1];
      double y0 = points[Y][lastX];
      double y1 = points[Y][lastX+1];
      return y0 + (x-x0)*(y1-y0)/(x1-x0);
    }
    else if (points[X][lastX] > points[X][lastX+1])
    {
      monotonicX = false;
      return 0.0; // The X-values should be monotonically increasing
    }

  lastX = 0;
  return 0.0;
}


bool FFpCurve::replaceByScaledShifted (const DFTparams& dft)
{
  // Check that the curve has got data (at least one point)
  if (points[X].size() < 1) return false;
  if (points[X].size() != points[Y].size()) return false;

  // Get data
  std::vector<double>& x = points[X];
  double scaleX = dft.scaleX;
  double shiftX = dft.zeroAdjustX ? dft.offsetX-scaleX*x.front() : dft.offsetX;

  std::vector<double>& y = points[Y];
  double scaleY = dft.scaleY;
  double shiftY = dft.zeroAdjustY ? dft.offsetY-scaleY*y.front() : dft.offsetY;

  if (scaleX == 1.0 && scaleY == 1.0 && shiftX == 0.0 && shiftY == 0.0)
    return true;

  // Replace numbers. We already know the sizes are the same
  for (size_t i = 0; i < x.size(); i++) {
    x[i] = x[i]*scaleX + shiftX;
    y[i] = y[i]*scaleY + shiftY;
  }

  this->setDataChanged();
  return true;
}


bool FFpCurve::replaceByDerivative ()
{
  // Get data
  std::vector<double>& x = points[X];
  std::vector<double>& y = points[Y];
  size_t n = x.size();
  if (n < 2 || y.size() != n) return false;

  // Compute the derivatives
  double dy0 = y[1] - y[0];
  double dx0 = x[1] - x[0];
  if (dx0 <= 0.0) return false;
  y.front() = dy0/dx0;
  for (size_t i = 1; i < n-1; i++) {
    double dy1 = y[i+1] - y[i];
    double dx1 = x[i+1] - x[i];
    if (dx1 <= 0.0) return false;
    y[i] = 0.5*(dy1/dx1 + dy0/dx0);
    dx0 = dx1;
    dy0 = dy1;
  }
  y.back() = dy0/dx0;

  this->setDataChanged();
  return true;
}


bool FFpCurve::replaceByIntegral ()
{
  // Get data
  std::vector<double>& x = points[X];
  std::vector<double>& y = points[Y];
  size_t n = x.size();
  if (n < 1 || y.size() < n) return false;

  // Compute the integrated curve
  double y0 = y.front();
  y.front() = 0.0;
  for (size_t i = 1; i < n; i++) {
    double y1 = y[i];
    double dx = x[i] - x[i-1];
    if (dx < 0.0) return false;
    y[i] = y[i-1] + 0.5*(y0+y1)*dx;
    y0 = y1;
  }

  this->setDataChanged();
  return true;
}


bool FFpCurve::replaceByDFT (const DFTparams& dft, const std::string& cId,
			     std::string& errMsg)
{
  // Check that the curve has got data (at least two points)
  if (points[X].size() < 2) return false;
  if (points[X].size() != points[Y].size()) return false;

  // Get data
  std::vector<double>& x = points[X];
  std::vector<double>& y = points[Y];

  // Check that set domain is sane
  double startDomain = x.front();
  double endDomain   = x.back();
  if (!dft.entiredomain)
  {
    if (dft.startDomain > startDomain) startDomain = dft.startDomain;
    if (dft.endDomain   < endDomain)   endDomain   = dft.endDomain;
    if (startDomain >= endDomain || startDomain > x.back())
    {
      errMsg += "\nError: Could not perform DFT on curve: \"" + cId
	+    "\".\n       Unable to find data in given time domain.";
      return false;
    }
  }

  // Check sample rate in data set
  const double eps = 0.01;
  double delta = dft.resample ? dft.resampleRate : x[1]-x[0];
  bool deltaOk = delta > 0.0;

  // If not resampling, check that sample rate remains constant
  if (!dft.resample)
    for (size_t i = 1; i < x.size() && deltaOk; i++)
      if (x[i-1] >= startDomain && x[i-1] <= endDomain)
	if (x[i]-x[i-1] < delta-delta*eps || x[i]-x[i-1] > delta+delta*eps)
	  deltaOk = false;

  if (!deltaOk)
  {
    errMsg += "\nError: Could not perform DFT on curve: \"" + cId
      + "\".\n       Sample rate for the curve domain could not be determined.";
    if (!dft.resample)
      errMsg += "\n       Consider specifying the sample rate explicitly"
	" in the 'Use sample rate' field.";
    return false;
  }

  // If we've established a sample rate and a domain, start sampling
  std::vector<double> yReIn;
  double shiftY = dft.zeroAdjustY ? dft.offsetY-y.front() : dft.offsetY;
  size_t N = this->numberOfSamples(delta,startDomain,endDomain);
  if (!this->sample(startDomain,endDomain,shiftY,dft.scaleY,dft.removeComp,
		    N,yReIn,errMsg))
  {
    errMsg += "\nError: Could not perform DFT on curve: \"" + cId
      +    "\".\n       Unable to find data in given time domain.";
    return false;
  }

  // Now, if all is ok then perform DFT
  size_t nOut = yReIn.size();
  double freqResolution = 1.0/(delta*nOut);

  std::vector<double> yImIn, yReOut, yImOut;
  if (FFpFourier::FFT(yReIn,yImIn,yReOut,yImOut))
  {
    // Replace the curve data with the transformed data
    x.clear(); x.reserve(nOut/2+1);
    y.clear(); y.reserve(nOut/2+1);
    for (size_t k = 0; k <= nOut/2; k++)
    {
      x.push_back(k*freqResolution);
      if (yImOut.empty())
	y.push_back(yReOut[k]);
      else if (dft.resultType == DFTparams::MAGNITUDE)
	y.push_back(hypot(yReOut[k],yImOut[k]));
      else if (dft.resultType == DFTparams::PHASE)
	y.push_back(atan2(yImOut[k],yReOut[k]));
      else
	y.push_back(0.0);
    }
    dataChanged = true;
    needRainflow = false;
  }
  else
  {
    errMsg += "\nError: FFT transformation failed for curve: \"" + cId + "\".";
    return false;
  }

  return true;
}


/*!
  The DFT function needs to be able to factor the number of samples cleanly
  so that largest factor (i.e. prime) is smaller than its set largest prime.
  This is why we're massaging the number of samples a bit.
*/

int FFpCurve::numberOfSamples (double delta, double start, double stop) const
{
  if (delta <= 0 || start >= stop) return 1;

  int maxPrime = FFpFourier::getMaxPrimeFactor();
  int nOut = (int)floor((stop-start)/delta);
  if (nOut == 0 || nOut == 1)
    nOut = 2;
  else if (nOut < maxPrime)
    nOut++;
  else
  {
    int strike = 0;
    std::vector<int> fac;
    int n = nOut;
    int rad[6] = { 10, 8, 5, 4, 3, 2 };
    while (n > maxPrime)
    {
      for (int i = 0; i < 6; i++)
	if (n%rad[i] == 0 && strike < 6)
	{
	  n = n/rad[i];
	  fac.push_back(rad[i]);
	  strike = 0;
	}
	else
	  strike++;

      if (strike >= 6)
      {
	n = n/2 + 1;
	fac.push_back(2);
	strike = 0;
      }
    }
    fac.push_back(n);
    nOut = 1;
    for (int j : fac) nOut *= j;
  }

  return nOut;
}


/*!
  Between start and stop of domain in x, sample n times from y and put results
  in the yOut vector. Apply the given shift and scale to the sampled values.
  Also subtract sample mean from all sample values if so wanted.
  It is assumed that start is inside the span of the vector,
  and that the X-values are monotonically increasing.
*/

bool FFpCurve::sample (double start, double stop, double shift, double scale,
		       bool subMean, size_t n, std::vector<double>& yOut,
                       std::string& errMsg) const
{
  if (points[X].empty()) return false;
  if (start < points[X].front()) return false;

  // Find sample positions, interpolate, shift/scale and add to yOut vector
  double mean = 0.0;
  double dX = (stop-start)/(double)n;
  size_t i;

  bool ok = true;
  yOut.resize(n);
  for (i = 0; i < n && ok; i++)
  {
    yOut[i] = this->getValue(start+i*dX,ok)*scale + shift;
    if (subMean) mean += yOut[i]/(double)n;
  }

  if (!ok)
    errMsg += "Error: Can not sample curve because "
      " the abscissa values are not monotonically increasing.\n";
  else if (subMean) // Subtract the mean value
    for (i = 0; i < n; i++)
      yOut[i] -= mean;

  return ok;
}


/*!
  Calculates statistical properties based on current curve data.
*/

bool FFpCurve::getCurveStatistics (bool entire, double start, double stop,
				   bool useScaledShifted, const DFTparams& dft,
				   double& rms, double& avg, double& stdDev,
				   double& integral, double& min, double& max,
				   std::string& errMsg) const
{
  rms = avg = stdDev = integral = min = max = 0.0;

  if (!entire && start >= stop)
    return false;

  size_t size = points[X].size();
  if (size < 1 || size > points[Y].size())
    return false;

  double scale = 1.0;
  double offset = 0.0;

  if (useScaledShifted) {
    if (dft.zeroAdjustY)
      offset = -(dft.scaleY * points[Y].front()) + dft.offsetY;
    else
      offset = dft.offsetY;
    scale = dft.scaleY;
  }

  // Do calculation
  double x, y;
  double prevX = 0.0;
  double prevY = 0.0;
  double numPt = 0.0;
  bool integralInit = false;
  bool maxMinInit = false;

  // Loop over all points
  size_t i;
  for (i = 0; i < size; i++) {
    x = points[X][i];
    y = scale * points[Y][i] + offset;

    if (entire || (x >= start && x <= stop)) {
      rms += y*y;
      avg += y;
      numPt++;

      if (integralInit)
	integral += 0.5 * (y + prevY) * (x - prevX);
      else
	integralInit = true;

      if (maxMinInit) {
	if (y > max)
	  max = y;
	else if (y < min)
	  min = y;
      }
      else {
	maxMinInit = true;
	max = min = y;
      }
    }
    else
      integralInit = false;

    prevX = x;
    prevY = y;
  }

  if (numPt == 0.0) {
    errMsg += "\nError: There are no curve data in the specified interval.";
    return false;
  }

  rms = sqrt(rms/numPt);
  avg /= numPt;

  // Do it all over for standard deviation...
  for (i = 0; i < size; i++) {
    x = points[X][i];
    if (entire || (x >= start && x <= stop)) {
      y = scale * points[Y][i] + offset - avg;
      stdDev += y*y;
    }
  }

  stdDev = sqrt(stdDev/numPt);
  return true;
}


bool operator!= (const RFprm& a, const RFprm& b)
{
  return a.start != b.start || a.stop != b.stop || a.gateValue != b.gateValue;
}


/*!
  Calculates the total damage based on the given S-N curve.
  \note The S-N curve is assumed to be given in [MPa], whereas the gateValue and
  the data of the curve itself are assumed to be in the current modelling units.
  \a toMPa defines the scaling factor from the modelling units to [MPa].
*/

double FFpCurve::getDamage (const RFprm& rf, double toMPa, const FFpSNCurve& sn)
{
  if (needRainflow || rf != lastRF)
  {
    // Either the history data, or the range, or gateValue have changed.
    // Need to re-do the rainflow analysis (this is the heavy part).
    if (this->performRainflowCalc(rf))
      needRainflow = false;
    else
      return -1.0;
  }

  // Calculate damage from the computed stress cycles
  FFpCycle::setScaleToMPa(toMPa);
  return FFpFatigue::getDamage(cycles,sn);
}


bool FFpCurve::replaceByRainflow (const RFprm& rf, double toMPa, bool doPVXonly,
                                  const std::string& cId, std::string& errMsg)
{
  // Need to re-do the rainflow analysis only when history data has changed,
  // or if some of the rainflow parameters have been changed
  if (needRainflow || rf != lastRF || doPVXonly)
  {
    if (this->performRainflowCalc(rf,doPVXonly))
      needRainflow = doPVXonly;
    else
    {
      errMsg += "\nError: Rainflow analysis failed for curve: \"" + cId + "\".";
      return false;
    }
  }

  if (doPVXonly)
  {
    if (toMPa != 1.0)
      for (double& y : points[Y]) y *= toMPa;
    return true;
  }

  // Replace the curve data with the cycle ranges
  points[X].clear();
  points[Y].resize(cycles.size());
  FFpCycle::setScaleToMPa(toMPa);
  for (size_t i = 0; i < cycles.size(); i++)
    points[Y][i] = cycles[i].range();

  dataChanged = true;
  return true;
}


/*!
  Performs peak valley extraction and rainflow counting on the curve point data.
*/

bool FFpCurve::performRainflowCalc (const RFprm& rf, bool doPVXonly)
{
  size_t i = 0;
  size_t n = points[X].size();
  if (rf.start < rf.stop)
  {
    while (i < n && points[X][i] < rf.start) i++;
    while (n > i && points[X][n-1] > rf.stop) n--;
    n -= i;
  }
  if (n == 0) return !needRainflow;
  if (n > points[Y].size()) return false;

  lastRF = rf;

  cycles.clear();
  std::vector<FFpPoint> turns;

  // Perform the peak valley extraction
  FFpPVXprocessor pvx(rf.gateValue);
  pvx.process(&points[X][i],&points[Y][i],n,turns,true);

  if (!doPVXonly)
  {
    // Perform the rainflow counting
    FFpRainFlowCycleCounter cyc(rf.gateValue);
    return cyc.process(turns,cycles,true);
  }
  else if (turns.size() >= points[X].size())
    return true;

  // Replace the curve data with the peak-and-valley points
  points[X].resize(turns.size());
  points[Y].resize(turns.size());
  for (i = 0; i < turns.size(); i++)
  {
    points[X][i] = turns[i].first;
    points[Y][i] = turns[i].second;
  }

  dataChanged = true;
  return true;
}


bool FFpCurve::reversePoints ()
{
  size_t n = reader[X].size();
  if (n < 2 || n != reader[Y].size()) return false;
  if (points[X].size() != n || points[Y].size() != n) return false;

  for (int axis = 0; axis < N_AXES; axis++)
  {
    std::reverse(reader[axis].begin(),reader[axis].end());
    std::reverse(points[axis].begin(),points[axis].end());
  }

  return true;
}
