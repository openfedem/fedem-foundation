// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#include "FFrLib/FFrExtractor.H"
#include "FFaLib/FFaDefinitions/FFaResultDescription.H"
#include <iostream>


/*!
  \brief Load FRS-files into an FFrExtractor object and search for a variable.
*/

size_t loadTest (const std::vector<std::string>& files, int npath, char** vpath)
{
  size_t validFiles = 0;
  FFrExtractor* res = new FFrExtractor("RDB reader");
  for (const std::string& frsFile : files)
    if (frsFile.find(".frs") != std::string::npos)
      if (res->addFile(frsFile,true))
      {
        std::cout <<"   * Loaded file "<< frsFile <<" OK" << std::endl;
        validFiles++;
      }
#ifdef FFR_DEBUG
  res->printHierarchy();
#endif
  if (npath > 1 && validFiles > 0)
  {
    std::vector<FFrEntryBase*> entries;
    FFaResultDescription descr(vpath[0]);
    for (int i = 1; i < npath; i++)
      descr.varDescrPath.push_back(vpath[i]);
    std::cout <<"\n   * Searching for "<< descr << std::endl;
    res->search(entries,descr);
    if (entries.empty())
    {
      std::cout <<" *** No match."<< std::endl;
      validFiles--;
    }
    else for (FFrEntryBase* entry : entries)
      entry->printPosition(std::cout);
  }

  delete res;
  FFrExtractor::releaseMemoryBlocks();
  return validFiles;
}
