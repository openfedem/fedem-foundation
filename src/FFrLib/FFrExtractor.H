// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

/*!
  \file FFrExtractor.H
  \brief Front-end for the result extraction module.
  \author Jacob Storen
*/

#ifndef FFR_EXTRACTOR_H
#define FFR_EXTRACTOR_H

#include "FFrLib/FFrVariable.H"
#include "FFrLib/FFrItemGroup.H"
#include <string>
#include <vector>
#include <set>
#include <map>

class FFrEntryBase;
class FFrObjectGroup;
class FFrSuperObjectGroup;
class FFrResultContainer;
class FFaResultDescription;


/*!
  \brief Front-end for the result extraction module.

  \details This class defines the front-end for accessing result data bases in
  a Fedem model. All data access is performed through the methods of this class.
  There is typically only one instance of this class for a given Fedem model,
  but it may contain an arbitrary number of result file containers, represented
  by the FFrResultContainer class. Note that a given result quantity may exist
  in more than one result container. When requested for, the value in the most
  recent result container is then returned.
*/

class FFrExtractor
{
public:
  //! \brief Default constructor.
  //! \param[in] xname Optional extractor name
  FFrExtractor(const char* xname = NULL);
  //! \brief The destructor frees the dynamically allocated members.
  virtual ~FFrExtractor();

  //! \brief Releases the dynamically allocated memory blocks in global scope.
  static void releaseMemoryBlocks(bool readOps = false);

  //! \brief Returns the name of this extractor.
  const std::string& getName() const { return myName; }

  //! \brief Adds a set of files to the RDB.
  bool addFiles(const std::set<std::string>& fileNames,
                bool showProgress = false);
  //! \brief Adds a list of files to the RDB.
  virtual bool addFiles(const std::vector<std::string>& fileNames,
                        bool showProgress = false, bool mustExist = false);
  //! \brief Adds a single file to the RDB.
  bool addFile(const std::string& fileName, bool mustExist = false);

  //! \brief Closes all result container files.
  void closeFiles();

  //! \brief Returns all result container files currently in this extractor.
  std::set<std::string> getAllResultContainerFiles() const;

  //! \brief Returns the result container associated with \a fileName.
  FFrResultContainer* getResultContainer(const std::string& fileName) const;

  //! \brief Removes the specified files from the RDB.
  virtual bool removeFiles(const std::set<std::string>& fileNames);

  //! \brief Checks if there is new data on disk.
  virtual void doResultFilesUpdate();

  //! \brief Returns a pointer to the variables container of this extractor.
  VariableSet*  getVariables()  { return &myVariables; }
  //! \brief Returns a pointer to the item group container of this extractor.
  ItemGroupSet* getItemGroups() { return &myItemGroups; }

  //! \brief Returns whether the extractor contains any results or not.
  bool empty() const { return myTopLevelOGs.empty(); }

  //! \brief Returns a specified top level variable or item group.
  FFrEntryBase* getTopLevelVar(const std::string& key) const;
  //! \brief Returns a specified object group.
  FFrObjectGroup* getObjectGroup(int id) const;
  //! \brief Returns a specified super object group.
  FFrSuperObjectGroup* getSuperGroup(const std::string& key) const;

  //! \brief Prints out some information for the result containers to std::cout.
  void printContainerInfo() const;
  //! \brief Prints out the RDB hierarchy of this extractor to std::cout.
  void printHierarchy() const;

  //! \brief Finds the first entry that matches a search criterion.
  FFrEntryBase* search(const FFaResultDescription& descr);
  //! \brief Wild-card version finding all entries matching a search criterion.
  void search(std::vector<FFrEntryBase*>& entries,
              const FFaResultDescription& descr);
  //! \brief Finds a variable reference of an object group.
  //! \param[in] oType Object group type name
  //! \param[in] baseId Base id of the object to find the variable within
  //! \param[in] vName Name of the variable to find
  FFrEntryBase* findVar(const std::string& oType, int baseId,
                        const std::string& vName);


  // RDB Positioning:

  //! \brief Resets the RDB to point to the first of all time steps recorded.
  bool resetRDBPositioning();

  //! \brief Positions the result containers in the RDB.
  //! \param[in] wantedTime The wanted physical time
  //! \param[out] foundTime The actual found physical time
  //! \param[in] getNextHigher If \e true, use the closest higher time step
  //! \return \e true if one or more of the containers are positioned
  bool positionRDB(double wantedTime, double& foundTime,
                   bool getNextHigher = false);

  //! \brief Increments the RDB to the closest time step defined.
  //! \details Used when mapping against key.
  bool incrementRDB();


  // RDB Time info access:

  //! \brief Get a set of keys for which valid data exist in the provided files.
  void getValidKeys(std::set<double>& resKeys,
                    const std::set<std::string>& files) const;

  //! \brief Enables file-based pre-read of time steps.
  void enableTimeStepPreRead(const std::set<std::string>& files);
  //! \brief Disables file-based pre-read of time steps.
  void disableTimeStepPreRead();
  //! \brief Clears the pre-read time step cache.
  void clearPreReadTimeStep();

  //! \brief Returns the physical time of the last time step in RDB.
  double getLastTimeStep() const;
  //! \brief Returns the physical time of the first time step in RDB.
  double getFirstTimeStep() const;
  //! \brief Returns the physical time of the last changed time step in RDB.
  double getLastWrittenTime() const;

  //! \brief Returns RDB position information.
  double getCurrentRDBPhysTime() const { return myCurrentPhysTime; }

  //! \brief Special single time step interface.
  int getSingleTimeStepData(const FFrEntryBase* entryRef,
                            const double* values, int nval);
  //! \brief Special single time step interface.
  int getSingleTimeStepData(const FFrEntryBase* entryRef,
                            const int* values, int nval);

  //! \brief Returns the text dictionary of this extractor.
  std::set<std::string>* getDictionary() { return &myDict; }

protected:
  //! \brief Checks if there is new data on disk for the given \a container.
  virtual int doSingleResultFileUpdate(FFrResultContainer* container);
  //! \brief Updates the top-level containers with items from \a container.
  bool updateExtractorHeader(FFrResultContainer* container);

private:
  std::string myName;       //!< Name of this extractor
  double myCurrentPhysTime; //!< Physical time of last time step read

  //! Text dictionary used to minimize multiple string storage
  std::set<std::string> myDict;

  //! File name to result container mapping
  typedef std::map<std::string,FFrResultContainer*> ContainerMap;

  ContainerMap myContainers; //!< All result containers in this extractor
  VariableSet  myVariables;  //!< All variables in this extractor
  ItemGroupSet myItemGroups; //!< All item groups in this extractor

protected:
  //! Top level super object group mapping
  std::map<std::string,FFrSuperObjectGroup*> myTopLevelSOGs;
  //! Top level object group mapping
  std::map<int,FFrObjectGroup*>              myTopLevelOGs;
  //! Top level variable mapping
  std::map<std::string,FFrEntryBase*>        myTopLevelVars;
};

#endif
