// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#ifndef FFP_GRAPH_H
#define FFP_GRAPH_H

#include "FFpCurve.H"


class FFpGraph
{
public:
  //! \brief Enums for available curve export formats.
  enum Format
  {
    ASCII,
    DAC_LITTLE_ENDIAN,
    DAC_BIG_ENDIAN,
    RPC_LITTLE_ENDIAN,
    RPC_BIG_ENDIAN,
    NFORMATS
  };

  FFpGraph(FFpCurve* curve = NULL);
  FFpGraph(size_t nCurves, bool populateGraph = true);

  ~FFpGraph();

  void addCurve (FFpCurve* curve) { curves.push_back(curve); }
  void clear() { curves.clear(); }

  bool empty() const { return curves.empty(); }
  size_t numCurves() const { return curves.size(); }
  FFpCurve& operator[] (const size_t i) { return *curves[i]; }

  void getTimeInterval (double& t0, double& t1) const { t0 = tmin; t1 = tmax; }
  void setTimeInterval (double t0, double t1);
  void setNoHeaderState (bool state = true) { noHeader = state; }
  void setNoXaxisValues (bool state = true) { noXvalues = state; }
  bool getNoXaxisValues () const { return noXvalues; }
  bool loadTemporalData (FFrExtractor* extractor, std::string& errMsg);
  bool loadSpatialData (FFrExtractor* extractor, std::string& errMsg);

  bool writeCurve (const std::string& fileName, int format,
		   const std::string& curveId, const std::string& descr,
		   const std::string& xTitle, const std::string& yTitle,
		   const std::string& modelName, std::string& errMsg,
		   int curveNo = 1);

  bool writeGraph (const std::string& fileName, int format,
		   const std::vector<std::string>& curveId,
		   const std::vector<std::string>& cDescr,
		   const std::string& modelName, std::string& errMsg,
		   int repeats = 0, int averages = 0,
		   int frmPts = 0, int grpPts = 0);

private:
  static double minIncrement (const std::vector<double>& xVals);

  std::vector<FFpCurve*> curves;

  bool   noHeader;   //!< Toggles writing of ASCII file header
  bool   noXvalues;  //!< Should only the Y-axis values be read?
  bool   internal;   //!< Are the FFpCurves allocated internally or not?
  double tmin, tmax; //!< Time interval for RDB data loading
};

#endif
