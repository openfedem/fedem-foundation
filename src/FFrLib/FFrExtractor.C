// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

/*!
  \file FFrExtractor.C
  \brief Front-end for the result extraction module.
*/

#include "FFrLib/FFrExtractor.H"
#include "FFrLib/FFrResultContainer.H"
#include "FFrLib/FFrVariableReference.H"
#include "FFrLib/FFrSuperObjectGroup.H"
#include "FFrLib/FFrObjectGroup.H"
#include "FFrLib/FFrReadOp.H"
#include "FFrLib/FFrReadOpInit.H"
#include "FFaLib/FFaDefinitions/FFaResultDescription.H"
#include "FFaLib/FFaDefinitions/FFaMsg.H"
#include "FFaLib/FFaOS/FFaFilePath.H"
#ifdef FT_USE_PROFILER
#include "FFaLib/FFaProfiler/FFaProfiler.H"
#endif
#include <cmath>
#include <cfloat>
#include <functional>


FFrExtractor::FFrExtractor(const char* xName)
{
  if (xName) myName = xName;

  myCurrentPhysTime = 0.0;
#ifdef FFR_DEBUG
  std::cout <<"sizeof(FFrSuperObjectGroup) = "<< sizeof(FFrSuperObjectGroup)
	    <<"\nsizeof(FFrObjectGroup) = "<< sizeof(FFrObjectGroup)
	    <<"\nsizeof(FFrItemGroup) = "<< sizeof(FFrItemGroup)
	    <<"\nsizeof(FFrVariable) = "<< sizeof(FFrVariable)
	    <<"\nsizeof(FFrVariableReference) = "<< sizeof(FFrVariableReference)
	    <<"\nsizeof(FFrFieldEntryBase) = "<< sizeof(FFrFieldEntryBase)
	    <<"\nsizeof(FFrEntryBase) = "<< sizeof(FFrEntryBase) << std::endl;
#endif
}


FFrExtractor::~FFrExtractor()
{
  for (const ContainerMap::value_type& c : myContainers)
    delete c.second;

  for (const std::pair<const std::string,FFrSuperObjectGroup*>& sog : myTopLevelSOGs)
    delete sog.second;

#if FFR_DEBUG > 2
  std::cout <<"\nDestroying top-level item groups"<< std::endl;
#endif
  for (FFrItemGroup* ig : myItemGroups)
    if (!ig->getOwner()) delete ig;

#if FFR_DEBUG > 2
  std::cout <<"\nDestroying top-level variable references"<< std::endl;
#endif
  for (const std::pair<const std::string,FFrEntryBase*>& tlv : myTopLevelVars)
    delete tlv.second;

#if FFR_DEBUG > 2
  std::cout <<"\nDestroying top-level variables"<< std::endl;
#endif
  for (FFrVariable* var : myVariables)
    delete var;
}


void FFrExtractor::releaseMemoryBlocks(bool readOps)
{
#ifdef FFR_NEWALLOC
  FFrItemGroup::releaseMemBlocks();
  FFrVariableReference::releaseMemBlocks();
#endif
  if (readOps) FFr::clearReadOps();
}


/*!
  Overloaded method that does the same as the below,
  but takes a set of files instead of a vector.
*/

bool FFrExtractor::addFiles(const std::set<std::string>& fileNames,
                            bool showProgress)
{
  if (fileNames.empty()) return true;

  std::vector<std::string> fileVec;
  fileVec.reserve(fileNames.size());
  for (const std::string& fileName : fileNames)
    fileVec.push_back(fileName);

  return this->addFiles(fileVec,showProgress);
}


/*!
  Adds several files to the result database.
  Returns \e false if one or more of the files caused an error.
*/

bool FFrExtractor::addFiles(const std::vector<std::string>& fileNames,
			    bool showProgress, bool mustExist)
{
#if FFR_DEBUG > 1
  std::cout <<"FFrExtractor::addFiles()"<< std::endl;
#endif
  if (fileNames.empty()) return true;

  if (showProgress)
    FFaMsg::enableSubSteps(fileNames.size());

  int subStep = 0;
  bool retval = true;
  for (const std::string& fileName : fileNames)
  {
    if (showProgress)
    {
      FFaMsg::setSubTask(FFaFilePath::getFileName(fileName));
      FFaMsg::setSubStep(++subStep);
    }
    if (!this->addFile(fileName,mustExist))
      retval = false;
  }

  if (showProgress)
  {
    FFaMsg::disableSubSteps();
    FFaMsg::setSubTask("");
  }

  return retval;
}


void FFrExtractor::closeFiles()
{
  for (const ContainerMap::value_type& c : myContainers)
    c.second->close();
}


std::set<std::string> FFrExtractor::getAllResultContainerFiles() const
{
  std::set<std::string> ret;
  for (const ContainerMap::value_type& c : myContainers)
    ret.insert(c.first);

  return ret;
}


FFrResultContainer* FFrExtractor::getResultContainer(const std::string& fileName) const
{
  ContainerMap::const_iterator it = myContainers.find(fileName);
  return it == myContainers.end() ? NULL : it->second;
}


bool FFrExtractor::addFile(const std::string& fileName, bool mustExist)
{
#if FFR_DEBUG > 1
  std::cout <<"FFrExtractor::addFile()\n\tfilename: "<< fileName << std::endl;
#endif

  // Check if we have added this file earlier
  FFrResultContainer* container = this->getResultContainer(fileName);
  if (container) return true;

  // Create a new result container for the given results file.
  // The header section of if is then parsed if the file is a valid.
  container = new FFrResultContainer(this,fileName);
  switch (this->doSingleResultFileUpdate(container)) {
  case FFrResultContainer::FFR_CONTAINER_INVALID:
    FFaMsg::list("   * Note: Ignoring invalid results database file:\n");
    break;
  case FFrResultContainer::FFR_NO_FILE_FOUND:
    if (mustExist) {
      FFaMsg::list("\n *** Error: Non-existing results database file:\n ");
      break;
    }
  default:
    // This container is valid
    myContainers[fileName] = container;
    return true;
  }

  // Invalid results file, delete the results container again
  FFaMsg::list("           " + fileName + "\n");
  delete container;
  return false;
}


bool FFrExtractor::updateExtractorHeader(FFrResultContainer* container)
{
#if FFR_DEBUG > 1
  std::cout <<"FFrExtractor::updateExtractorHeader()\n\tfilename: "
            << container->getFileName() << std::endl;
#endif
  if (!container->isHeaderComplete()) return false;

#ifdef FT_USE_PROFILER
  FFaProfiler timer("ExtractorTimer");
  timer.startTimer("updateExtractorHeader");
  FFaMemoryProfiler::reportMemoryUsage("> updateExtractorHeader");
#endif

  for (FFrEntryBase* entry : container->topLevel())
    if (entry->isOG())
    {
      // Add to base id -> object group map, unless it is already present there.
      // In that case, merge it with the existing object group.
      FFrObjectGroup* ogptr = static_cast<FFrObjectGroup*>(entry);
      if (ogptr->hasBaseID() || ogptr->hasUserID() || ogptr->hasDescription())
      {
        FFrObjectGroup* ogr = this->getObjectGroup(ogptr->getBaseID());
        if (!ogr)
          myTopLevelOGs.insert(std::make_pair(ogptr->getBaseID(),ogptr));
        else if (ogr->merge(ogptr))
          ogptr = NULL;
      }

      if (ogptr)
      {
        // Add to top level hierarchy
        FFrSuperObjectGroup* sog = this->getSuperGroup(ogptr->getType());
        if (!sog)
        {
          sog = new FFrSuperObjectGroup(ogptr->getType(),myDict);
          ogptr->setOwner(sog);
          sog->dataFields.push_back(ogptr);
          myTopLevelSOGs[ogptr->getType()] = sog;
        }
        else if (ogptr->hasBaseID() || ogptr->hasUserID() || ogptr->hasDescription())
        {
          sog->dataFields.push_back(ogptr);
          ogptr->setOwner(sog);
        }
        else
          sog->dataFields.front()->merge(ogptr);
#if FFR_DEBUG > 2
	std::cout <<"FFrExtractor::UpdateExtractorHeader(): Added "
		  << ogptr->getType() <<" {"<< ogptr->getBaseID() <<"}"
		  << std::endl;
#endif
      }
    }
    else if (entry->isIG() || entry->isVarRef())
    {
      // Add to top level variables/item groups
      const std::string& description = entry->getDescription();
#if FFR_DEBUG > 1
      std::cout <<"FFrExtractor::UpdateExtractorHeader(): Adding ";
#if FFR_DEBUG > 2
      entry->printID(std::cout);
#else
      std::cout << description << std::endl;
#endif
#endif
      FFrEntryBase* oldEntry = this->getTopLevelVar(description);
      if (oldEntry)
        oldEntry->merge(entry);
      else
      {
        myTopLevelVars[description] = entry;
        entry->setGlobal();
      }
    }

#if FFR_DEBUG > 1
  std::cout <<"FFrExtractor::UpdateExtractorHeader(): "<< container->topLevel().size()
            <<" top level entries\n"<< std::endl;
#endif

#ifdef FT_USE_PROFILER
  FFaMemoryProfiler::reportMemoryUsage("  updateExtractorHeader >");
  timer.stopTimer("updateExtractorHeader");
  timer.report();
#endif
  return true;
}


bool FFrExtractor::removeFiles(const std::set<std::string>& fileNames)
{
#if FFR_DEBUG > 1
  std::cout <<"FFrExtractor::removeFiles()";
  for (const std::string& fileName : fileNames) std::cout <<"\n\t"<< fileName;
  std::cout << std::endl;
#endif

  FFrResultContainer* cont;
  ContainerMap allConts;
  std::set<FFrResultContainer*> frsConts;
  for (const std::string& fileName : fileNames)
    if ((cont = this->getResultContainer(fileName)))
    {
      allConts[fileName] = cont;
      if (FFaFilePath::isExtension(fileName,"frs"))
        frsConts.insert(cont);
    }

  if (!frsConts.empty())
  {
    for (std::pair<const std::string,FFrEntryBase*>& tlv : myTopLevelVars)
      tlv.second->removeContainers(frsConts);

    // Need to rebuild the top-level object groups
    FFrObjectGroup* og;
    myTopLevelOGs.clear();
    for (std::pair<const std::string,FFrSuperObjectGroup*>& sog : myTopLevelSOGs)
    {
      sog.second->removeContainers(frsConts);
      for (FFrEntryBase* entry : sog.second->dataFields)
        if ((og = dynamic_cast<FFrObjectGroup*>(entry)))
        {
          myTopLevelOGs.insert(std::make_pair(og->getBaseID(),og));
#if FFR_DEBUG > 1
          std::cout <<"FFrExtractor::removeFiles(): Added "<< og->getType()
                    <<" {"<< og->getBaseID() <<"}"<< std::endl;
#endif
        }
    }
  }

  // Now delete the files and associated container objects from memory
  for (const ContainerMap::value_type& cont : allConts)
  {
    myContainers.erase(cont.first);
    delete cont.second;
  }

  return true;
}


FFrEntryBase* FFrExtractor::search(const FFaResultDescription& descr)
{
  // Lambda function for searching within the variable description paths
  // for a matching description. Note that we only consider the non-empty items
  // here (variables with no result container associated with them are ignored).
  // This catches the event of identical path for two variables but with
  // different data type (or size), and only one of them has result a container.
  std::function<FFrEntryBase*(FFrEntryBase*,size_t)> find =
    [descr](FFrEntryBase* entry, size_t istart) -> FFrEntryBase*
  {
    for (size_t i = istart; i < descr.varDescrPath.size() && entry; i++)
    {
      FFrFieldEntryBase* currEntry = dynamic_cast<FFrFieldEntryBase*>(entry);
      if (!currEntry) return NULL;

      entry = NULL;
      for (FFrEntryBase* next : currEntry->dataFields)
        if (next->getDescription() == descr.varDescrPath[i] && !next->isEmpty())
        {
          entry = next;
          break;
        }
    }

    return entry;
  };

  FFrEntryBase* entry = NULL;
  if (descr.baseId < 0)
  {
    // Super-Object Groups
    entry = this->getSuperGroup(descr.OGType);
  }
  else if (!descr.OGType.empty())
  {
    // Object Groups
    if (descr.baseId > 0)
      entry = find(this->getObjectGroup(descr.baseId),0);
    else
    {
      // For building of possibilities
      FFrSuperObjectGroup* sog = this->getSuperGroup(descr.OGType);
      if (sog && !sog->dataFields.empty())
        entry = find(sog->dataFields.front(),0);
    }
  }
  else if (!descr.varDescrPath.empty())
  {
    // Top level variables
    entry = find(this->getTopLevelVar(descr.varDescrPath.front()),1);
  }

#ifdef FFR_DEBUG
  if (!entry)
    std::cerr <<"FFrExtractor::search: Entry not found: "<< descr << std::endl;
#endif
  return entry;
}


/*!
  New search method used to set up the result menu in animation properties
  based on the actual results present in the RDB. This version returns a
  vector of all entries that match the given description, in which a single
  '*' (wild-card) means all entries on that level.
*/

void FFrExtractor::search(std::vector<FFrEntryBase*>& entries,
                          const FFaResultDescription& descr)
{
  // Recursive lambda function to find all matching entries.
  // It will also include the empty entries, if any.
  std::function<void(FFrEntryBase*,size_t)> find =
    [&find,&entries,descr](FFrEntryBase* entry, size_t istart) -> void
  {
    for (size_t i = istart; i < descr.varDescrPath.size() && entry; i++)
    {
      FFrFieldEntryBase* currEntry = dynamic_cast<FFrFieldEntryBase*>(entry);
      if (!currEntry) return;

      entry = NULL;
      const std::string& vName = descr.varDescrPath[i];
      for (FFrEntryBase* field : currEntry->dataFields)
        if (i+1 < descr.varDescrPath.size())
        {
          if (vName == "*") // wild card
            find(field,i+1); // continue search on next path level
          else if (vName == field->getDescription())
          {
            entry = field;
            break;
          }
        }
        else if (vName == "*" || vName == field->getDescription())
        {
          // Pick all matching entries on the leaf level (normally only one)
#if FFR_DEBUG > 1
          std::cout <<"Found FFr-entry "
                    << field->getEntryDescription() << std::endl;
#endif
          entries.push_back(field);
        }
    }
  };

  entries.clear();
  if (descr.baseId < 0 || descr.OGType.empty())
  {
    // Super-Object Group or Top level variable,
    // find only the first match (no wild-cards allowed)
    FFrEntryBase* entry = this->search(descr);
    if (entry) entries.push_back(entry);
  }
  else if (descr.baseId > 0)
  {
    // Object Groups
    find(this->getObjectGroup(descr.baseId),0);
  }
  else
  {
    // For building of possibilities
    FFrSuperObjectGroup* sog = this->getSuperGroup(descr.OGType);
    if (sog)
      for (FFrEntryBase* entry : sog->dataFields)
        find(entry,0);
  }

#ifdef FFR_DEBUG
  if (entries.empty())
    std::cerr <<"FFrExtractor::search: No match for entry: "<< descr << std::endl;
#endif
}


FFrEntryBase* FFrExtractor::getTopLevelVar(const std::string& key) const
{
  std::map<std::string,FFrEntryBase*>::const_iterator it = myTopLevelVars.find(key);
  return it == myTopLevelVars.end() ? NULL : it->second;
}


FFrObjectGroup* FFrExtractor::getObjectGroup(int id) const
{
  std::map<int,FFrObjectGroup*>::const_iterator it = myTopLevelOGs.find(id);
  return it == myTopLevelOGs.end() ? NULL : it->second;
}


FFrSuperObjectGroup* FFrExtractor::getSuperGroup(const std::string& key) const
{
  std::map<std::string,FFrSuperObjectGroup*>::const_iterator it = myTopLevelSOGs.find(key);
  return it == myTopLevelSOGs.end() ? NULL : it->second;
}


FFrEntryBase* FFrExtractor::findVar(const std::string& oType, int baseId,
                                    const std::string& vName)
{
  FFrObjectGroup* ogEntry = this->getObjectGroup(baseId);
  if (!ogEntry)
    std::cerr <<"Error : No object group with base id "<< baseId << std::endl;

  else if (ogEntry->getType() != oType)
    std::cerr <<"Error : Object group with base id "<< baseId
              <<" should have been a "<< oType
              <<", but it is a "<< ogEntry->getType() << std::endl;

  else for (FFrEntryBase* entry : ogEntry->dataFields)
    if (entry->getDescription() == vName)
      return entry;

#ifdef FFR_DEBUG
  std::cerr <<"FFrExtractor::findVar: Object group "<< oType <<" {"<< baseId
            <<"} has no variable named "<< vName << std::endl;
#endif
  return NULL;
}


void FFrExtractor::doResultFilesUpdate()
{
  for (const ContainerMap::value_type& c : myContainers)
    this->doSingleResultFileUpdate(c.second);
}


int FFrExtractor::doSingleResultFileUpdate(FFrResultContainer* container)
{
#if FFR_DEBUG > 2
  std::cout <<"FFrExtractor::doSingleResultFileUpdate()\n\tfilename: "
	    << container->getFileName() << std::endl;
#endif

  if (container->getContainerStatus() == FFrResultContainer::FFR_DATA_CLOSED)
    return FFrResultContainer::FFR_DATA_CLOSED;

  bool wasComplete = container->isHeaderComplete();
  int status = container->updateContainerStatus();
  if (!wasComplete && container->isHeaderComplete())
    this->updateExtractorHeader(container);

  return status;
}


void FFrExtractor::printContainerInfo() const
{
  for (const ContainerMap::value_type& c : myContainers)
    c.second->printInfo();

#ifdef FFR_DEBUG
 std::cout <<"Sizes:"
           <<"\n   ResContainers: "<< myContainers.size()
           <<"\n   Variables:     "<< myVariables.size()
           <<"\n   ItemGroups:    "<< myItemGroups.size()
           <<"\n   TopLevelSOGs:  "<< myTopLevelSOGs.size()
           <<"\n   TopLevelOGs:   "<< myTopLevelOGs.size()
           <<"\n   TopLevelVars:  "<< myTopLevelVars.size()
           <<"\n   Dictionary:    "<< myDict.size() << std::endl;
#endif
}


void FFrExtractor::printHierarchy() const
{
  unsigned int varrefCounter = 0;

  // Lambda function for printing an FFrVariableReference object
  std::function<void(FFrVariableReference*)> printVarRef =
    [&varrefCounter](FFrVariableReference* vRef)
  {
    if (!vRef) return;

#if FFR_DEBUG > 2
    if (vRef->containers.empty())
      std::cout <<" (empty";
    else for (size_t i = 0; i < vRef->containers.size(); i++)
      std::cout << (i == 0 ? " (" : " ")
                << FFaFilePath::getFileName(vRef->containers[i].first->getFileName());
#else
    std::cout <<" ("<< vRef->containers.size();
#endif
    std::cout <<")";

    FFrReadOp<double>* roD;
    FFrReadOp<float>*  roF;
    FFrReadOp<int>*    roI;
    FFaOperationBase*  rop = vRef->getReadOperation();
    if ((roD = dynamic_cast<FFrReadOp<double>*>(rop)))
    {
      double val = 0.0;
      roD->evaluate(val);
      std::cout <<"\t"<< val <<" (double)";
    }
    else if ((roF = dynamic_cast<FFrReadOp<float>*>(rop)))
    {
      float val = 0.0f;
      roF->evaluate(val);
      std::cout <<"\t"<< val <<" (float)";
    }
    else if ((roI = dynamic_cast<FFrReadOp<int>*>(rop)))
    {
      int val = 0;
      roI->evaluate(val);
      std::cout <<"\t" << val <<" (int)";
    }
    rop->unref();

    ++varrefCounter;
  };

  // Recursive lambda function for printing an FFrEntryBase object
  std::function<void(FFrFieldEntryBase*,int)> printEntry =
    [&printEntry,&printVarRef](FFrFieldEntryBase* eb, int indent)
  {
    if (!eb) return;

    for (FFrEntryBase* entry : eb->dataFields)
    {
      std::cout <<"\n"<< std::string(indent*4,' ');
#if FFR_DEBUG > 2
      entry->printID(std::cout,false);
#else
      std::cout << entry->getDescription();
#endif
      FFrFieldEntryBase* fieldEntry = dynamic_cast<FFrFieldEntryBase*>(entry);
      if (fieldEntry)
        printEntry(fieldEntry,indent+1);
      else
        printVarRef(dynamic_cast<FFrVariableReference*>(entry));

      if (entry->getOwner() == NULL)
        std::cout <<"\n *** Owner not set!";
      else if (entry->getOwner() != dynamic_cast<FFrEntryBase*>(eb))
        std::cout <<"\n *** Owner wrong: "<< eb->getDescription();
    }
  };

  std::cout <<"\n   * TopLevel item groups ("<< myItemGroups.size() <<")";
#if FFR_DEBUG > 2
  varrefCounter = 0;
  for (FFrItemGroup* ig : myItemGroups)
  {
    std::cout <<"\n";
    ig->printID(std::cout,false);
    printEntry(ig,1);
    std::cout << std::endl;
  }
#endif

  std::cout <<"\n   * TopLevel variables ("<< myTopLevelVars.size() <<")\n";
  for (const std::pair<const std::string,FFrEntryBase*>& tlv : myTopLevelVars)
  {
    std::cout <<"\n"<< tlv.second->getDescription();
    FFrItemGroup* igRef = dynamic_cast<FFrItemGroup*>(tlv.second);
    if (igRef)
      printEntry(igRef,2);
    else
      printVarRef(dynamic_cast<FFrVariableReference*>(tlv.second));
  }

  std::cout <<"\n\n   * TopLevel Hierarchy ("<< myTopLevelSOGs.size() <<")";
  for (const std::pair<const std::string,FFrSuperObjectGroup*>& sog : myTopLevelSOGs)
  {
    std::cout <<"\n\n"<< sog.second->getDescription() <<" ("<< sog.second->dataFields.size() <<")";
    for (FFrEntryBase* entry : sog.second->dataFields)
    {
      std::cout <<"\n    \""<< entry->getDescription() <<"\"";
      if (entry->getOwner() == NULL)
        std::cout <<"\n *** Owner not set!";
      else if (entry->getOwner() != dynamic_cast<FFrEntryBase*>(sog.second))
        std::cout <<"\n *** Owner wrong: "<< sog.second->getDescription();
      printEntry(dynamic_cast<FFrFieldEntryBase*>(entry),2);
    }
  }

  std::cout <<"\n\n   * Number of variable references: "
            << varrefCounter << std::endl;
}


bool FFrExtractor::positionRDB(double wantedTime,
	 		       double& foundTime, bool getNextHigher)
{
#if FFR_DEBUG > 1
  std::cout <<"FFrExtractor::positionRDB() Wanted time: "<< wantedTime << std::endl;
#endif
  if (myContainers.empty()) return false;

  // Position all containers at the closest time step.
  // Check whether all are starting after wanted time.

  bool isBeforeAll = true;
  bool isAfterAll = true;
  for (const ContainerMap::value_type& c : myContainers)
  {
    int status = c.second->positionAtKey(wantedTime,getNextHigher);
    if (status != FFrResultContainer::FFR_BEFORE_START)
      isBeforeAll = false;

    if (status != FFrResultContainer::FFR_AFTER_END)
      isAfterAll = false;
  }

  // Find the container with the actual closest time step, and use that as
  // the "foundTime". Containers that start after wanted time are not counted,
  // unless all containers are after the wanted time. That is to always find the
  // closest time step at the lower side.

  double minDiff = DBL_MAX;
  double closestTime = DBL_MAX;
  for (const ContainerMap::value_type& c : myContainers)
  {
    int status = c.second->getPositioningStatus();
    if (getNextHigher)
    {
      if (status == FFrResultContainer::FFR_AFTER_END && !isAfterAll)
        continue;
    }
    else
    {
      if (status == FFrResultContainer::FFR_BEFORE_START && !isBeforeAll)
        continue;
    }

    double time = c.second->getCurrentKey();
    double diff = fabs(wantedTime-time);
    if (diff < minDiff) {
      closestTime = time;
      minDiff = diff;
    }
  }

  foundTime = myCurrentPhysTime = closestTime;

  // If the found time deviates significantly from the wanted time,
  // we need to reposition the containers to the found time,
  // such that incrementRDB works later (Bugfix #367).
  if (fabs(foundTime-wantedTime) > 1.0e-12)
    for (const ContainerMap::value_type& c : myContainers)
      c.second->positionAtKey(myCurrentPhysTime);

#if FFR_DEBUG > 1
  std::cout <<"FFrExtractor::positionRDB() Found time: "<< foundTime << std::endl;
#endif
  return true;
}


int FFrExtractor::getSingleTimeStepData(const FFrEntryBase* entryRef,
					const double* values, int nvals)
{
  if (!entryRef) return 0;

  if (!entryRef->isVariableFloat()) // variable is double
    return entryRef->readPositionedTimestepData(values,nvals);

  // Variable is float ==> must type-cast from float to double
  float* tmpVals = new float[nvals];
  double* dpVals = const_cast<double*>(values);
  int valuesRead = entryRef->readPositionedTimestepData(tmpVals,nvals);
  for (int i = 0; i < valuesRead; i++) dpVals[i] = tmpVals[i];
  delete[] tmpVals;
  return valuesRead;
}


int FFrExtractor::getSingleTimeStepData(const FFrEntryBase* entryRef,
					const int* values, int nvals)
{
  if (!entryRef) return 0;

  return entryRef->readPositionedTimestepData(values, nvals);
}


bool FFrExtractor::resetRDBPositioning()
{
  double smallestKey = DBL_MAX;
  bool validContainers = false;

  for (const ContainerMap::value_type& c : myContainers)
  {
    if (c.second->getStepsInFile())
      validContainers = true;
    double key = c.second->getFirstKey();
    if (key < smallestKey)
      smallestKey = key;
  }

  if (!validContainers) return false;

  for (const ContainerMap::value_type& c : myContainers)
  {
    c.second->positionAtKey(smallestKey);
    c.second->resetPositioning();
  }

  myCurrentPhysTime = smallestKey;

  return true;
}


/*!
  The default key (and currently hard-coded) is physical time.
  If we are at the end of all time series, return \e false.
*/

bool FFrExtractor::incrementRDB()
{
  // Find the container(s) whith the nearest data
  double dist, nearestNext = DBL_MAX;

  for (const ContainerMap::value_type& c : myContainers)
    if (c.second->getDistanceToNextKey(dist))
      if (dist < nearestNext)
        nearestNext = dist;

  // Return false if we have reached the end of all time steps
  if (nearestNext == DBL_MAX) return false;

#if FFR_DEBUG > 1
  std::cout <<"FFrExtractor::incrementRDB() "<< myCurrentPhysTime
	    <<" --> "<< myCurrentPhysTime+nearestNext << std::endl;
#endif

  myCurrentPhysTime += nearestNext;

  // Calculate the distance from the current time to the containers time
  for (const ContainerMap::value_type& c : myContainers)
    c.second->positionAtKey(myCurrentPhysTime);

  return true;
}


/*!
  Get the times with valid data from the extractor, looking only on the
  extractor files matching the file names provided.
*/

void FFrExtractor::getValidKeys(std::set<double>& validValues,
				const std::set<std::string>& files) const
{
  FFrResultContainer* cont = NULL;
  for (const std::string& fileName : files)
    if ((cont = this->getResultContainer(fileName)))
    {
      std::set<double>::iterator lastInserted = validValues.begin();
      for (const std::pair<const double,int>& timeStep : cont->getPhysicalTime())
        lastInserted = validValues.insert(lastInserted,timeStep.first);
    }
}


double FFrExtractor::getLastTimeStep() const
{
  double maxTimeStep = -HUGE_VAL;

  for (const ContainerMap::value_type& c : myContainers)
  {
    const std::map<double,int>& times = c.second->getPhysicalTime();
    std::map<double,int>::const_reverse_iterator tit = times.rbegin();
    if (tit != times.rend() && maxTimeStep < tit->first)
      maxTimeStep = tit->first;
  }

  return maxTimeStep;
}


double FFrExtractor::getFirstTimeStep() const
{
  double minTimeStep = HUGE_VAL;

  for (const ContainerMap::value_type& c : myContainers)
  {
    const std::map<double,int>& times = c.second->getPhysicalTime();
    std::map<double,int>::const_iterator tit = times.begin();
    if (tit != times.end() && minTimeStep > tit->first)
      minTimeStep = tit->first;
  }

  return minTimeStep;
}


/*!
  The largest end time among the result containers with new or existing data
  is returned. If no data exist, -HUGE_VAL is returned.
*/

double FFrExtractor::getLastWrittenTime() const
{
  double lastWrittenTime = -HUGE_VAL;

  for (const ContainerMap::value_type& c : myContainers)
    if (c.second->getContainerStatus() >= FFrResultContainer::FFR_DATA_PRESENT)
    {
      const std::map<double,int>& times = c.second->getPhysicalTime();
      std::map<double,int>::const_reverse_iterator tit = times.rbegin();
      if (tit != times.rend() && lastWrittenTime < tit->first)
        lastWrittenTime = tit->first;
    }

  return lastWrittenTime;
}


/*!
  Enables single read of the current time step in the selected files.
*/

void FFrExtractor::enableTimeStepPreRead(const std::set<std::string>& files)
{
  FFrResultContainer* cont = NULL;
  for (const std::string& fileName : files)
    if ((cont = this->getResultContainer(fileName)))
      cont->enablePreRead(true);
}


void FFrExtractor::disableTimeStepPreRead()
{
  for (const ContainerMap::value_type& c : myContainers)
    c.second->enablePreRead(false);
}


void FFrExtractor::clearPreReadTimeStep()
{
  for (const ContainerMap::value_type& c : myContainers)
    c.second->clearPreRead();
}
