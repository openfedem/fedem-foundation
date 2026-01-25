// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

/*!
  \file FFrResultContainer.H
  \brief Results file data container.
*/

#ifndef FFR_RESULT_CONTAINER_H
#define FFR_RESULT_CONTAINER_H

#include <string>
#include <vector>
#include <map>
#include <set>

#include "FFrLib/FFrVariable.H"
#include "FFrLib/FFrItemGroup.H"
#include "FFaLib/FFaOS/FFaIO.H"

class FFrExtractor;
class FFrVariableReference;


//! Physical time to time step mapping
using FFrTimeMap  = std::map<double,int>;
//! Results entry container
using FFrEntrySet = std::vector<FFrEntryBase*>;

//! \brief Incapsulation of temporary data used during results file parsing.
struct FFrCreatorData
{
  //! Temporary variable container
  std::map<int,FFrVariable*>  variables;
  //! Temporary item group container
  std::map<int,FFrItemGroup*> itemGroups;
  //! Reference to the top-level entries of the results container
  FFrEntrySet&  topLevelEntries;
  //! Pointer to the result variable container of the results extractor
  VariableSet*  extractorVariables;
  //! Pointer to the item group container of the results extractor
  ItemGroupSet* extractorIGs;
  //! Pointer to the text dictionary of the results extractor
  std::set<std::string>* dict;

  //! \brief The constructor initializes the data members.
  FFrCreatorData(FFrEntrySet& tl, VariableSet* vars, ItemGroupSet* itgs,
                 std::set<std::string>* tdic) : topLevelEntries(tl)
  {
    extractorVariables = vars;
    extractorIGs       = itgs;
    dict               = tdic;
  }
};


/*!
  \brief Class holding the data of a Fedem results file.

  \details Instances of this class represents one results file each of a
  Fedem model. The class also contains methods for parsing the file header
  and populating the datastructure for traversing the results hierarchy,
  as well as methods for reading the results themselves (binary data).
*/

class FFrResultContainer
{
public:
  //! \brief The constructor initializes the data members.
  FFrResultContainer(FFrExtractor* extractor, const std::string& fileName);
  //! \brief The destructor closes the file and releases heap-allocated data.
  ~FFrResultContainer();

  //! \brief Disable default copy constructor.
  FFrResultContainer(const FFrResultContainer&) = delete;
  //! \brief Disable default assignment operator.
  FFrResultContainer& operator=(const FFrResultContainer&) = delete;

  //! \brief Enums defining the various container statuses that may occur.
  enum Status {
    FFR_CONTAINER_INVALID = 1,
    FFR_NO_FILE_FOUND,
    FFR_TEXT_FILE,
    FFR_TEXT_PRESENT,
    FFR_NEW_TEXT,
    FFR_HEADER_INCOMPLETE,
    FFR_HEADER_COMPLETE,
    FFR_DATA_CLOSED,
    FFR_DATA_PRESENT,
    FFR_NEW_DATA
  };

  //! \brief Closes the results file of this container, if still open.
  Status close();

  //! \brief Updates the container to a new status (used from the system ticks).
  Status updateStatus(bool memPoll);

  //! \brief Returns the current status of this results container.
  Status getContainerStatus() const { return myStatus; }
  //! \brief Returns \e true, if the file header has been successfully parsed.
  bool isHeaderComplete() const { return myStatus >= FFR_HEADER_COMPLETE; }

  //! \brief Returns the file name of this results container.
  const std::string& getFileName() const { return myFileName; }
  //! \brief Returns the time stamp of this results container.
  unsigned int getDate() const { return myDate; }

  //! \brief Returns the number of time steps in this results container
  int getStepsInFile() const { return myPhysicalTimeMap.size(); }
  //! \brief Returns the size (in bytes) of the results data of each time step.
  int getStepSize() const { return timeStepSize; }
  //! \brief Returns the size (in bytes) of the results file header.
  FT_int getHeaderSize() const { return myHeaderSize; }

  //! \brief Prints out some size parameters for this results container.
  void printInfo() const;

  //! \brief Returns a const reference to the top-level entries container.
  const FFrEntrySet& topLevel() const { return myTopLevelEntries; }
  //! \brief Returns a const reference to the time step mapping.
  const FFrTimeMap& getPhysicalTime() const { return myPhysicalTimeMap; }

  //! \brief Positions the file to the specified key (physical time).
  int positionAtKey(double key, bool getNextHigher = false);
  //! \brief Returns whether the file has been positioned.
  bool isPositioned() const { return myPositionedTimeStep > 0; }
  //! \brief Resets the file position.
  void resetPositioning();


  //! \brief Reads a double variable from the specified position.
  int readPositionedTimestepData(const double* values, int nvals,
				 int bitPos, int cellBits, int repeats)
  {
    return this->actualRead((void*)values,nvals,bitPos,cellBits,repeats);
  }

  //! \brief Reads a float variable from the specified position.
  int readPositionedTimestepData(const float* values, int nvals,
				 int bitPos, int cellBits, int repeats)
  {
    return this->actualRead((void*)values,nvals,bitPos,cellBits,repeats);
  }

  //! \brief Reads an integer variable from the specified position.
  int readPositionedTimestepData(const int* values, int nvals,
				 int bitPos, int cellBits, int repeats)
  {
    return this->actualRead((void*)values,nvals,bitPos,cellBits,repeats);
  }


  //! \brief Enums defining the file position statuses that may occur.
  enum PositionStatus {
    FFR_NOT_SET,
    FFR_BEFORE_START,
    FFR_INSIDE,
    FFR_AFTER_END
  };

  //! \brief Returns current file position status flag.
  PositionStatus getPositioningStatus() const { return myWantedKeyStatus; }
  //! \brief Returns current physical time of this container.
  double getCurrentKey() const { return this->getKey(0); }
  //! \brief Returns first physical time of this container.
  double getFirstKey() const { return this->getKey(1); }
  //! \brief Returns last physical time of this container.
  double getLastKey() const { return this->getKey(2); }

  //! \brief Returns the difference between current position and the wanted key.
  double getDistanceFromPosKey(bool usePositionedKey = false) const;
  //! \brief Calculates the distance to the next key for this results container.
  bool getDistanceToNextKey(double& dist);

  //! \brief Collects temporary entries to be deleted when parsing finished.
  void collectGarbage(FFrEntryBase* entry);

  //! \brief Enables time step pre-read cache.
  void enablePreRead(bool f) { if (!(iAmPreReading = f)) this->clearPreRead(); }
  //! \brief Checks if the time step cache is enabled.
  bool isPreReadEnabled() const { return iAmPreReading; }
  //! \brief Clears the time step cache.
  void clearPreRead();

protected:
  //! \brief Prints out sime size parameters for the results file.
  void printSizeParameters(FT_int fileSize) const;

  //! \brief Reads the header of the file.
  bool readFileHeader(bool memPoll);
  //! \brief Reads variable definitions and references.
  FFrStatus readVariables(FILE* fp, FFrCreatorData& cd,
                          bool dataBlocks, bool memPoll);

  //! \brief Builds and resolves the results hierarchy of the file.
  bool buildAndResolveHierarchy(bool memPoll);
  //! \brief Reads time step information from the file.
  bool readTimeStepInformation();
  //! \brief Reads the file squentially from current file position.
  bool readText();
  //! \brief Reopens the results file for direct access of binary data.
  bool reopenForDataAccess();
  //! \brief Reads all data associated with current time step into a cache.
  void fillPreRead();
  //! \brief Returns current, first or last physical time of this container.
  double getKey(int flag) const;
  //! \brief Reads a certain number of bytes from the binary results file.
  int actualRead(void* var, int nvals, int bitPos, int cellBits, int repeats);

private:
  FFrVariableReference* myPhysTimeRef; //!< Pointer to physical time variable

  // data info
  bool    swapBytes;    //!< Endian mismatch?
  int     timeStepSize; //!< Time step size in bytes (max 2GB)
  FT_int  myHeaderSize; //!< Binary start position in bytes
  FILE*   myFile;       //!< File descriptor used in header parsing
  FT_FILE myDataFile;   //!< File handle used for data access
  Status  myStatus;     //!< Current status of the file reading

  // header info
  std::string  myFileName; //!< File name associated with this result container
  std::string  myModule;   //!< Program module that wrote the results file
  unsigned int myDate;     //!< Time stamp of the results file

  // extractor info
  FFrExtractor* myExtractor;     //!< Result extractor that own this container
  FFrEntrySet myTopLevelEntries; //!< List of all top-level entries
  FFrEntrySet myGarbage;         //!< Garbage collection container

  // information for pre-positioning of the container
  int myPositionedTimeStep; //!< Current file position
  int myLastReadEndPos;     //!< Position after the last data read

  FFrTimeMap           myPhysicalTimeMap; //!< Physical time to results mapping
  FFrTimeMap::iterator myCurrentIndexIt;  //!< Pointer to what we should read

  double         myWantedKey;       //!< The physical time to read results for
  PositionStatus myWantedKeyStatus; //!< Is the key inside, exact, front or rear

  bool iAmLazyPositioned; //!< Only move file pointer first time reading data
  bool iAmPreReading;     //!< Read all results for current time step into core

  char* myPreRead;         //!< Pre-read cache
  int   myPreReadTimeStep; //!< Current time step in pre-read cache
};

#endif
