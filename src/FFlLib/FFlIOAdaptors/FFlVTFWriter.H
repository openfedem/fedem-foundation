// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#ifndef FFL_VTF_WRITER_H
#define FFL_VTF_WRITER_H

#include <string>
#include <vector>
#include <set>
#include <map>

#include "FFlLib/FFlIOAdaptors/FFlWriterBase.H"

class VTFAFile;

#ifdef FF_NAMESPACE
namespace FF_NAMESPACE {
#endif


class FFlVTFWriter : public FFlWriterBase
{
public:
  FFlVTFWriter(const FFlLinkHandler* link);
  FFlVTFWriter(const FFlVTFWriter&) = delete;
  virtual ~FFlVTFWriter();

  FFlVTFWriter& operator=(const FFlVTFWriter&) = delete;

  bool write(const std::string& filename, const std::string& partname,
             int ID = 1, int type = 0);

  bool write(VTFAFile& file, const std::string& partname, int ID = 1,
             std::vector<int>* outputOrder = NULL,
             std::vector<int>* firstOrderNodes = NULL);

  // Mapping from Fedem node order to VTF order
  static const int T6m[6];
  static const int Q8m[8];
  static const int T10m[10];
  static const int P15m[15];
  static const int H20m[20];

protected:
  bool writeNodes(int blockID, bool withID = true);
  bool writeElements(const std::string& partName, int blockID, int nodesID,
                     bool withID = true, bool convTo1stOrder = false);
  bool writeGeometry(const int* parts, int nPart);
  bool writeProperties(int blockID);

  bool showError(const char* msg, int ID = 0);

private:
  VTFAFile*         myFile;
  std::vector<int>  myOrder;
  std::set<int>     myNodes;
  std::map<int,int> myNodMap;
};

#ifdef FF_NAMESPACE
} // namespace
#endif
#endif
