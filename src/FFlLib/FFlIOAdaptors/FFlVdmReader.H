// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#ifndef FFL_VDM_READER_H
#define FFL_VDM_READER_H

#include <string>
#include <map>

#include "FFaLib/FFaAlgebra/FFaMat34.H"
#include "FFlLib/FFlIOAdaptors/FFlReaderBase.H"

struct vdm_DataFun;
struct vis_Model;
struct vis_EProp;

#ifdef FF_NAMESPACE
namespace FF_NAMESPACE {
#endif

class FFlAttributeBase;


class FFlVdmReader : public FFlReaderBase
{
public:
  FFlVdmReader(FFlLinkHandler* readIntoLink);
  virtual ~FFlVdmReader();

protected:
  bool loadModel();
  bool convert();

private:
  FFlAttributeBase* getRGDAttribute(vis_EProp* eprop,
				    int pid, int nelnod,
				    bool& ignoreElm);
  FFlAttributeBase* getWAVGMAttribute(vis_EProp* eprop,
				      int pid, int nelnod,
				      bool& ignoreElm);
  FFlAttributeBase* getMassAttribute(vis_EProp* eprop, int pid, int cid);

  static int getDofFlag(int bitFlag);

protected:
  vdm_DataFun* datafun;
  vis_Model*   model;

private:
  int                   nWarnings;
  std::map<int,FaMat34> coordSys;
};


class FFlAnsysReader : public FFlVdmReader
{
public:
  FFlAnsysReader(FFlLinkHandler* link) : FFlVdmReader(link) {}
  virtual ~FFlAnsysReader() {}

  static void init();
  static void identifierCB(const std::string& fileName, int& isAnsysFile);
  static void readerCB(const std::string& fileName, FFlLinkHandler* link);

protected:
  bool read(const char* fileName);
};


class FFlAbaqusReader : public FFlVdmReader
{
public:
  FFlAbaqusReader(FFlLinkHandler* link) : FFlVdmReader(link) {}
  virtual ~FFlAbaqusReader() {}

  static void init();
  static void identifierCB(const std::string& fileName, int& isAbaqusFile);
  static void readerCB(const std::string& fileName, FFlLinkHandler* link);

protected:
  bool read(const char* fileName);
};

#ifdef FF_NAMESPACE
} // namespace
#endif
#endif
