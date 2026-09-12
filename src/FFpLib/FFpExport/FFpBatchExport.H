// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

/*!
  \file FFpBatchExport.H
  \brief Auto-export of curve data for batch executions.
*/

#ifndef FFP_BATCH_EXPORT_H
#define FFP_BATCH_EXPORT_H

#include <vector>
#include <string>

class FFrExtractor;
class FFpCurveDef;
class FFpGraph;


/*!
  \brief Struct holding some meta data parameters of a RPC3-file.
*/

struct FFpRPC3Data
{
  //! \brief Parses some meta data from the specified \a rpcFile.
  bool readDataFromFile(const std::string& rpcFile);

  int repeats  = 0; //!< Number of repeats
  int averages = 0; //!< Number of averages
  int framePts = 0; //!< Size of data frame
  int groupPts = 0; //!< Size of data group
};


/*!
  \brief Class managing auto-export of curve data for batch executions.
*/

class FFpBatchExport
{
public:
  //! \brief The constructor initializes \ref myExtractor.
  //! \param[in] frsFiles List of frs-files to read curve data from
  FFpBatchExport(const std::vector<std::string>& frsFiles);
  //! \brief The destructor releases the heap-allocated data objects.
  ~FFpBatchExport();

  //! \brief Reads a list of frs-files with results from specified model file.
  //! \param[out] frsFiles List of frs-files associated with the model file.
  //! \param[in] modelFile The Fedem model file to extract frs-file names from
  static bool readFrsFiles(std::vector<std::string>& frsFiles,
                           const std::string& modelFile);

  //! \brief Reads the curve definitions from a specified model file.
  //! \param[in] defFile The Fedem model file to read curve definitions from
  bool readCurves(const std::string& defFile);
  //! \brief Exports curves as separate two-column files.
  //! \param[in] path Prefix of the exported curve data files.
  //! \param[in] modelFile Fedem model file (for curve data file header)
  //! \param[in] format Format of the exported curve files.
  bool exportCurves(const std::string& path,
                    const std::string& modelFile, int format);
  //! \brief Exports curves as a single multi-column file.
  //! \param[in] fName Name of the exported curve data file.
  //! \param[in] modelFile Fedem model file (for curve data file header)
  //! \param[in] format Format of the exported curve file.
  //! \param[in] rpc RPC3-file meta data
  bool exportGraph(const std::string& fName,
                   const std::string& modelFile, int format,
                   const FFpRPC3Data& rpc);
  //! \brief Prints position data for the curve data variables (for debugging).
  //! \param[in] fName Optional file to print to, if empty use std::cout.
  bool printPosition(const std::string& fName = "");

private:
  //! \brief Reads the curve data to be exported.
  //! \param[out] rdbCurves Container holding all curve data to be exporte
  void readPlottingData(FFpGraph& rdbCurves);

  FFrExtractor* myExtractor; //!< Extractor object holding the results data

  std::vector<FFpCurveDef*> myCurves; //!< Curve definition container
};

#endif
