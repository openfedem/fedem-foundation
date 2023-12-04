// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#include "FFlLib/FFlIOAdaptors/FFlReaders.H"
#include "FFlLib/FFlLinkHandler.H"
#include "FFlLib/FFlMemPool.H"

#include "FFaLib/FFaOS/FFaFilePath.H"
#include "FFaLib/FFaDefinitions/FFaMsg.H"


char FFlReaders::convertToLinear = 0;


bool FFlReaders::registerReader(const std::string& name, const std::string& ext,
                                const FFaDynCB2<const std::string&,FFlLinkHandler*>& readerCB,
                                const FFaDynCB2<const std::string&,int&>& identifierCB,
                                const std::string& info1, const std::string& info2)
{
  // Check that each reader is defined only once...
  for (const FFlReaderData& reader : myReaders)
    if (reader.info.name == name) return false;

  myReaders.push_back(FFlReaderData());
  FFlReaderData& data = myReaders.back();

  data.info.name    = name;
  data.info.extensions.insert(ext);
  data.readerCB     = readerCB;
  data.identifierCB = identifierCB;
  data.info.info1   = info1;
  data.info.info2   = info2;
  return true;
}


void FFlReaders::addExtension(const std::string& name, const std::string& newExt)
{
  for (FFlReaderData& reader : myReaders)
    if (reader.info.name == name) {
      reader.info.extensions.insert(newExt);
      return;
    }
}


void FFlReaders::getRegisteredReaders(std::vector<FFlReaderInfo>& infoData,
                                      bool includeDefaultReader) const
{
  unsigned int i = 0;
  for (const FFlReaderData& reader : myReaders)
    if ((i++) != defaultReader || includeDefaultReader)
      infoData.push_back(reader.info);
}


const FFlReaderInfo& FFlReaders::getDefaultReader() const
{
  if (defaultReader < myReaders.size())
    return myReaders[defaultReader].info;

  static FFlReaderInfo tempInfo;
  return tempInfo;
}


void FFlReaders::setDefaultReader(const std::string& name)
{
  for (size_t i = 0; i < myReaders.size(); i++)
    if (myReaders[i].info.name == name) {
      defaultReader = i;
      return;
    }

  defaultReader = 0;
}


int FFlReaders::read(const std::string& fileName, FFlLinkHandler* link)
{
  int identified = 0;
  std::vector<FFlReaderData>::iterator it = myReaders.end();
  std::string extension = FFaFilePath::getExtension(fileName);
  if (!extension.empty())
  {
    // Try to identify using the extension identify CB
    for (it = myReaders.begin(); it != myReaders.end(); ++it)
      if (it->info.extensions.find(extension) != it->info.extensions.end())
        break;

    if (it != myReaders.end())
      it->identifierCB.invoke(fileName,identified);
    else
      ListUI <<"\n  -> No readers registered for ."<< extension <<" files";
  }

  if (!identified)
  {
    // Throw file at all identifyCBs and see if things match
    // (kind of auto-detection)
    ListUI <<"\n  -> Trying to auto-detect ...\n";
    for (it = myReaders.begin(); it != myReaders.end(); ++it)
    {
      it->identifierCB.invoke(fileName,identified);
      if (identified > 0)
      {
        ListUI <<"  -> Identified as "<< it->info.name <<"\n";
        break;
      }
    }
  }

  if (!identified)
    ListUI <<"  -> Sorry, could not identify the FE data file format.\n";
  else if (identified < 0)
    ListUI <<"\n *** Error: Can not open FE data file "<< fileName <<"\n";
  else
  {
    it->readerCB.invoke(fileName,link);
    it->identifierCB.invoke(std::string(),identified);
    if (link->isTooLarge())
    {
      identified = 0;
      ListUI <<"\n *** Parsing FE data file \""<< fileName <<"\" aborted.\n\n";
    }
    else if (!link->hasGeometry())
    {
      identified = 0;
      ListUI <<"\n *** Parsing FE data file \""<< fileName <<"\" failed.\n"
             <<"     The FE model is probably not consistent and has not been"
             <<" resolved completely.\n     Delete this part, fix the FE data"
             <<" file and then try to import it once again.\n\n";
    }
    else if (!link->resolve(convertToLinear == 2))
    {
      identified = 0;
      ListUI <<"\n *** Resolving FE data in \""<< fileName <<"\" failed.\n"
             <<"     The FE model is not consistent and should be deleted.\n"
             <<"     Delete this part, fix the FE data file"
             <<" and then try to import it once again.\n\n";
    }
#ifdef FFL_DEBUG
    else link->dump();
#endif
  }

  FFlMemPool::resetMemPoolPart();
  FFaMsg::setSubTask("");
  return identified;
}
