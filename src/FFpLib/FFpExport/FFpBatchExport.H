// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#ifndef FFP_BATCH_EXPORT_H
#define FFP_BATCH_EXPORT_H

#include <vector>
#include <string>

class FFrExtractor;
class FFpCurveDef;
class FFpGraph;


class FFpRPC3Data
{
public:
  FFpRPC3Data() { repeats =  averages =  framePts = groupPts = 0; }

  bool readDataFromFile(const std::string& rpcFile);

  int repeats, averages, framePts, groupPts;
};


class FFpBatchExport
{
public:
  FFpBatchExport(const std::vector<std::string>& frsFiles);
  ~FFpBatchExport();

  //! \brief Reads a list of frs-files with results from specified model file.
  static bool readFrsFiles(std::vector<std::string>& frsFiles,
                           const std::string& modelFile);

  bool readCurves   (const std::string& defFile);
  bool exportCurves (const std::string& path,
                     const std::string& modelFile, int format);
  bool exportGraph  (const std::string& fName,
                     const std::string& modelFile, int format,
		     const FFpRPC3Data& rpc);
  bool printPosition(const std::string& fName = "");

private:
  void readPlottingData(FFpGraph& rdbCurves);

  FFrExtractor*             myExtractor;
  std::vector<FFpCurveDef*> myCurves;
};

#endif
