// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#include "FFpLib/FFpExport/FFpBatchExport.H"
#include "FFrLib/FFrReadOpInit.H"
#include "FFaLib/FFaOperation/FFaBasicOperations.H"
#include "FFaLib/FFaCmdLineArg/FFaCmdLineArg.H"
#include "FFaLib/FFaString/FFaTokenizer.H"
#include "FFaLib/FFaOS/FFaFilePath.H"
#include "FFaLib/FFaOS/FFaFortran.H"
#include "FFaLib/FFaDefinitions/FFaMsg.H"


/*!
  \brief Launches the automatic curve export after the solver has finished.
*/

SUBROUTINE(ffp_crvexp,FFP_CRVEXP) (const char* frsNames,
#ifdef _NCHAR_AFTER_CHARARG
                                   const int nchar1,
                                   const char* expPath, const int nchar2,
                                   const char* modName, const int nchar3,
                                   int& ierr
#else
                                   const char* expPath, const char* modName,
                                   int& ierr, const int nchar1,
                                   const int nchar2, const int nchar3
#endif
){
  FFaTokenizer frsFiles(std::string(frsNames,nchar1),'<','>',',');
  if (frsFiles.empty() || frsFiles.front().empty())
  {
    ierr = -4;
    ListUI <<"\n===> No results database files given, nothing to export.\n";
    return;
  }

  std::string rpcFile, crvFile, expFile(expPath,nchar2);
  FFaCmdLineArg::instance()->getValue ("rpcFile",rpcFile);
  FFaCmdLineArg::instance()->getValue ("curveFile",crvFile);
  if (crvFile.empty())
  {
    ierr = -3;
    ListUI <<"\n===> No curve definition file specified,"
           <<" use the -curveFile option.\n";
    return;
  }

  int format = 0, precision = 0;
  FFaCmdLineArg::instance()->getValue ("curvePlotType",format);
  FFaCmdLineArg::instance()->getValue ("curvePlotPrec",precision);

  // Read number of repeats, etc., from the specified input RPC-file
  FFpRPC3Data rpc;
  if (!rpcFile.empty() && format > 2 && format < 5)
    if (!rpc.readDataFromFile(FFaFilePath::checkName(rpcFile)))
    {
      ierr = -1;
      ListUI <<"\n===> Exporting Curves failed.\n";
      return;
    }

  FFr::initReadOps();
  FFa::initBasicOps();

  // Open frs-files and read the curve definitions
  ListUI <<"\n===> Exporting Curves to "<< expFile
         <<"\n     based on results stored in "<< frsFiles.front();
  for (size_t i = 1; i < frsFiles.size(); i++)
    ListUI <<"\n                                "<< frsFiles[i];
  ListUI <<"\n";

  FFpBatchExport exporter(frsFiles);
  bool success = exporter.readCurves(FFaFilePath::checkName(crvFile));
  if (!success)
  {
    ierr = -1;
    ListUI <<"\n===> Exporting Curves failed.\n";
    return;
  }

  // Now export the curves
  if (format > 2)
    success = exporter.exportGraph(std::string(expPath,nchar2),
				   std::string(modName,nchar3),
				   precision*10+format%5,rpc);
  else
    success = exporter.exportCurves(std::string(expPath,nchar2),
				    std::string(modName,nchar3),
				    precision*10+format);

  ListUI <<"===> Exporting Curves "<< (success ? "done" : "failed") <<".\n";
  ierr = success ? 0 : -2;
}
