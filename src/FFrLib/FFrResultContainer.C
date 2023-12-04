// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

/*!
  \file FFrResultContainer.C
  \brief Results file data container.
*/

#include "FFrLib/FFrResultContainer.H"
#include "FFrLib/FFrVariable.H"
#include "FFrLib/FFrVariableReference.H"
#include "FFrLib/FFrItemGroup.H"
#include "FFrLib/FFrObjectGroup.H"
#include "FFrLib/FFrExtractor.H"
#include "FFaLib/FFaOS/FFaFilePath.H"
#include "FFaLib/FFaOS/FFaTag.H"
#include <string.h>
#include <float.h>
#include <algorithm>
#include <functional>

#ifdef FT_USE_PROFILER
#include "FFaLib/FFaProfiler/FFaProfiler.H"
#endif


FFrResultContainer::FFrResultContainer(FFrExtractor* extractor,
                                       const std::string& fileName)
{
  myExtractor = extractor;
  myFileName = fileName;

  myPhysTimeRef = NULL;
  myPositionedTimeStep = 0;
  myPreReadTimeStep = -1;
  myLastReadEndPos = 0;
  myStatus = FFR_NO_FILE_FOUND;

  myWantedKey = 0.0;
  myWantedKeyStatus = FFR_NOT_SET;
  iAmLazyPositioned = true;

  swapBytes = false;
  timeStepSize = 0;
  myHeaderSize = (FT_int)0;

  myFile = NULL;
  myDate = 0;
  myDataFile = 0;
  myPreRead = NULL;
  iAmPreReading = false;
}


FFrResultContainer::~FFrResultContainer()
{
  this->close();

  for (FFrEntryBase* entry : myTopLevelEntries)
    if (!entry->getOwner() && !entry->isGlobal())
      delete entry;
}


FFrResultContainer::Status FFrResultContainer::close()
{
  this->clearPreRead();

  if (myFile)
    if (Fclose(myFile))
      perror("FFrResultContainer::close");

  if (myDataFile)
    if (FT_close(myDataFile))
      perror("FFrResultContainer::close");

  myFile = NULL;
  myDataFile = 0;
  myStatus = FFR_DATA_CLOSED;
  return myStatus;
}


void FFrResultContainer::clearPreRead()
{
  if (myPreRead)
  {
#if FFR_DEBUG > 1
    std::cout <<"\nFFrResultContainer::clearPreRead(): Cache size for "
              << FFaFilePath::getFileName(myFileName) <<": "
              << timeStepSize << std::endl;
#endif
    delete[] myPreRead;
    myPreRead = NULL;
  }
  myPreReadTimeStep = -1;
}


/*!
  Tries to open the results file,
  read the header and the time step information in the file.
  Returns an enum telling the new status of the container.
*/

FFrResultContainer::Status FFrResultContainer::updateContainerStatus()
{
#if FFR_DEBUG > 1
  std::cout <<"\nFFrResultContainer::updateContainerStatus()"<< std::endl;
  std::string fileName(FFaFilePath::getFileName(myFileName));
#endif

#ifdef FT_USE_PROFILER
  FFaMemoryProfiler::reportMemoryUsage("> updateContainerStatus");
#endif

  bool stop = false;
  while (!stop)
    switch (myStatus)
      {
      case FFR_CONTAINER_INVALID:
#if FFR_DEBUG > 2
	std::cout <<"FFR_CONTAINER_INVALID: "<< fileName << std::endl;
#endif
	stop = true;
	break;

      case FFR_NO_FILE_FOUND:
#if FFR_DEBUG > 2
	std::cout <<"FFR_NO_FILE_FOUND: "<< fileName << std::endl;
#endif
	myFile = Fopen(myFileName.c_str(),"rb");
	if (myFile)
	  myStatus = FFR_HEADER_INCOMPLETE;
	else
	  stop = true;
	break;

      case FFR_HEADER_INCOMPLETE:
#if FFR_DEBUG > 2
	std::cout <<"FFR_HEADER_INCOMPLETE: "<< fileName << std::endl;
#endif
	if (!FFaFilePath::isExtension(myFileName,"frs"))
	  myStatus = FFR_TEXT_FILE; // this is not an frs-file
	else if (!this->readFileHeader())
	  stop = true;
	else if (!this->buildAndResolveHierarchy())
	  myStatus = FFR_CONTAINER_INVALID;
	else if (myModule != "fedem_modes")
	  myStatus = FFR_DATA_CLOSED;
	else
	{
	  myStatus = FFR_DATA_CLOSED;
	  // Don't reopen mode shape files until we actually need the data
	  Fclose(myFile);
	  myFile = NULL;
	  stop = true;
	}
	break;

      case FFR_DATA_CLOSED:
#if FFR_DEBUG > 2
	std::cout <<"FFR_DATA_CLOSED: "<< fileName << std::endl;
#endif
	if (!this->reopenForDataAccess())
	  myStatus = FFR_CONTAINER_INVALID;
	else if (myPhysicalTimeMap.empty())
	  myStatus = FFR_HEADER_COMPLETE;
	else
	  myStatus = FFR_DATA_PRESENT;
	break;

      case FFR_TEXT_FILE:
#if FFR_DEBUG > 2
	std::cout <<"FFR_TEXT_FILE: "<< fileName << std::endl;
#endif
	myHeaderSize = Ftell(myFile);

      case FFR_TEXT_PRESENT:
      case FFR_NEW_TEXT:
	if (this->readText())
	  myStatus = FFR_NEW_TEXT;
	else
	  myStatus = FFR_TEXT_PRESENT;
	stop = true;
	break;

      case FFR_HEADER_COMPLETE:
#if FFR_DEBUG > 2
	std::cout <<"FFR_HEADER_COMPLETE: "<< fileName << std::endl;
#endif
	if (this->readTimeStepInformation())
	{
	  myStatus = FFR_NEW_DATA;
	  myCurrentIndexIt = myPhysicalTimeMap.begin();
        }
	stop = true;
	break;

      case FFR_DATA_PRESENT:
#if FFR_DEBUG > 2
	std::cout <<"FFR_DATA_PRESENT: "<< fileName << std::endl;
#endif
      case FFR_NEW_DATA:
#if FFR_DEBUG > 2
	std::cout <<"FFR_NEW_DATA: "<< fileName << std::endl;
#endif
	if (this->readTimeStepInformation())
	  myStatus = FFR_NEW_DATA;
	else
	  myStatus = FFR_DATA_PRESENT;
	stop = true;
	break;

      default:
        stop = true;
      }

#ifdef FT_USE_PROFILER
  FFaMemoryProfiler::reportMemoryUsage("  updateContainerStatus >");
#endif
  return myStatus;
}


/*!
  Reads the file header and stores the obtained binary position in the object.
*/

bool FFrResultContainer::readFileHeader()
{
#if FFR_DEBUG > 1
  std::cout <<"\nFFrResultContainer::readFileHeader() "
	    << FFaFilePath::getFileName(myFileName) << std::endl;
#endif

  // read the first line to learn about endian etc.

  rewind(myFile);
  std::string tag;
  unsigned int chkSum;
  int endianOnFile = FFaTag::read(myFile, tag, chkSum);
  if (endianOnFile < 0)
  {
#ifdef FFR_DEBUG
    std::cerr <<"FFrResultContainer: Error in file "<< myFileName
	      <<"\n                    Read status: "<< endianOnFile
	      << std::endl;
#endif
    return false;
  }

  swapBytes = (endianOnFile != FFaTag::endian());

  // fast forward to find the "DATA:" field - indication of complete header

  char line[BUFSIZ];
  FFrStatus mode = LABEL_SEARCH;
  long_int startHeader = fgets(line,BUFSIZ,myFile) ? Ftell(myFile) : 0;
  while (strncmp(line,"DATA:",5))
    if (feof(myFile) || !fgets(line,BUFSIZ,myFile))
    {
#ifdef FFR_DEBUG
      std::cerr <<"FFrResultContainer: Error in file "<< myFileName
		<<"\n                    Could not find DATA: field"
		<< std::endl;
#endif
      return false;
    }
    else if (!strncmp(line,"DATABLOCKS:",11))
      mode = FOUND_DATABLOCKS;

  if (mode < FOUND_DATABLOCKS)
  {
#ifdef FFR_DEBUG
    std::cerr <<"FFrResultContainer: Error in file "<< myFileName
              <<"\n                    Could not find DATABLOCKS: field"
              << std::endl;
#endif
    return false;
  }

  if (Fseek(myFile, startHeader, SEEK_SET) == EOF)
  {
    perror("FFrResultContainer::readFileHeader");
    return false;
  }

  // Lambda function to parse the Date and Time string
  // in the header and converting it to an unsigned integer value.
  // The syntax should match the output from the Fortran subroutine
  // versionmodule::getcurrentdate (see vpmBaseCommon/versionModule.f90).
  std::function<unsigned int(const char*)> parseDate = [](const char* date)
  {
    char Mon[4];
    unsigned int d, m, y, hour, min, sec, idate = 0;
    if (sscanf(date,"%2d %3s %4d %2d:%2d:%2d;",&d,Mon,&y,&hour,&min,&sec) == 6)
    {
      const char* Months[12] = { "Jan","Feb","Mar","Apr","May","Jun",
                                 "Jul","Aug","Sep","Oct","Nov","Dec" };
      for (m = 0; m < 12 && strcmp(Months[m],Mon); m++);
      idate = sec + 60*(min-1 + 60*(hour-1 + 24*(d-1 + 31*(m + 12*(y-2000)))));
    }
#if FFR_DEBUG > 3
    std::cout <<"\tParsed date: "<< d <<" "<< m+1 <<" "<< y
              <<" "<< hour <<":"<< min <<":"<< sec <<" --> "<< idate << std::endl;
#endif
    return idate;
  };

  FFrCreatorData myCreatorData(myTopLevelEntries,
                               myExtractor->getVariables(),
                               myExtractor->getItemGroups(),
                               myExtractor->getDictionary());

  int c = 0;
  const char cmt = '#';
  std::string label, value;

  mode = LABEL_SEARCH;
  while (!feof(myFile) && mode != FAILED)
    switch (mode)
      {
      case LABEL_SEARCH:
	c = getc(myFile);
	while (!feof(myFile) && isspace(c)) c = getc(myFile);
	if (c == cmt)
	  mode = LABEL_IGNORE;
	else
	  mode = LABEL_READ;
#if FFR_DEBUG > 3
	std::cout <<"LABEL_SEARCH: "<< (char)c << std::endl;
#endif
	break;

      case LABEL_IGNORE:
	while (!feof(myFile) && getc(myFile) != '\n');
	if (label.empty())
	  mode = LABEL_SEARCH;
	else
	  mode = LABEL_VALID;
	break;

      case LABEL_READ:
	for (; isalnum(c) && !feof(myFile); c = getc(myFile))
	  label += (char)toupper(c);
	while (!feof(myFile) && isspace(c)) c = getc(myFile);
#if FFR_DEBUG > 3
	std::cout <<"LABEL_READ: "<< label <<" "<< (char)c << std::endl;
#endif
	if (label.empty())
	  mode = LABEL_ERROR;
	else if (c == cmt)
	  mode = LABEL_IGNORE;
	else if (c == ':')
	  mode = LABEL_VALID;
	else if (c == '=')
	  mode = FOUND_HEADING;
	else
	  mode = LABEL_ERROR;
	break;

      case LABEL_ERROR:
#if FFR_DEBUG > 3
	std::cout <<"LABEL_ERROR: "<< label <<" "<< (char)c << std::endl;
#endif
	label.resize(0);
	mode = LABEL_IGNORE;
	break;

      case LABEL_VALID:
	if (label == "VARIABLES")
	  mode = FOUND_VARIABLES;
	else if (label == "DATABLOCKS")
	  mode = FOUND_DATABLOCKS;
	else if (label == "DATA")
	  mode = FOUND_DATA;
	else
	  mode = LABEL_SEARCH;
#if FFR_DEBUG > 3
	std::cout <<"LABEL_VALID: mode = "<< mode << std::endl;
#endif
	label.resize(0);
	break;

      case FOUND_HEADING:
	for (c = getc(myFile); !feof(myFile) && isspace(c); c = getc(myFile));
	for (; !feof(myFile) && c != ';' && c != '\n'; c = getc(myFile))
	  value += (char)c;
#if FFR_DEBUG > 3
	std::cout <<"FOUND_HEADING: "<< label <<'='<< value << std::endl;
#endif
	if (label == "MODULE")
	  myModule = value;
	else if (label == "DATETIME")
	  myDate = parseDate(value.c_str());
	label.resize(0);
	value.resize(0);
	mode = LABEL_SEARCH;
	break;

      case FOUND_VARIABLES:
#if FFR_DEBUG > 3
	std::cout <<"FOUND_VARIABLES"<< std::endl;
#endif
	mode = this->readVariables(myFile,myCreatorData);
	break;

      case FOUND_DATABLOCKS:
#if FFR_DEBUG > 3
	std::cout <<"FOUND_DATABLOCKS"<< std::endl;
#endif
	mode = this->readVariables(myFile,myCreatorData,true);
	break;

      case FOUND_DATA:
	myHeaderSize = Ftell(myFile);
	mode = DONE;
#if FFR_DEBUG > 3
	std::cout <<"FOUND_DATA: header size = "<< (int)myHeaderSize
		  << std::endl;
#endif
	break;

      case DONE:
#if FFR_DEBUG > 1
        std::cout <<"\nFFrResultContainer::readFileHeader() Done."
                  <<"\n  myTempItemGroups " << myCreatorData.itemGroups.size()
                  <<"\n  myTempVars "       << myCreatorData.variables.size()
                  << std::endl;
#endif
	return true;

      default:
	break;
      }

#ifdef FFR_DEBUG
  std::cerr <<"FFrResultContainer: Error in file "<< myFileName
	    <<"\n                    Parsing aborted before header is complete."
	    << std::endl;
#endif
  return false;
}


FFrStatus FFrResultContainer::readVariables(FILE* fs,
                                            FFrCreatorData& myCreatorData,
                                            bool dataBlocks)
{
#if FFR_DEBUG > 1
  std::cout <<"\nFFrResultContainer::readVariables() "
	    << FFaFilePath::getFileName(myFileName)
	    <<" "<< std::boolalpha << dataBlocks << std::endl;
#endif

#ifdef FT_USE_PROFILER
  FFaMemoryProfiler::reportMemoryUsage("> readVariables ");
#endif

  int c = 0;
  const char cmt = '#';
  FFrStatus mode = LABEL_SEARCH;

  while (!feof(fs) && mode != FAILED && mode != DONE)
    switch (mode)
      {
      case LABEL_SEARCH:
	c = getc(fs);
	while (!feof(fs) && isspace(c)) c = getc(fs);
	if (c == cmt)
	  mode = LABEL_IGNORE;
	else
	  mode = LABEL_READ;
	break;

      case LABEL_IGNORE:
	while (!feof(fs) && getc(fs) != '\n');
	mode = LABEL_SEARCH;
	break;

      case LABEL_READ:
	while (!feof(fs) && isspace(c)) c = getc(fs);
	if (c == cmt)
	  mode = LABEL_IGNORE;
	else if (c == '[' || c == '<' || c == '{')
	  mode = LABEL_VALID;
	else
	  mode = LABEL_ERROR;
	break;

      case LABEL_VALID:
	if (c == '<')
	  mode = FFrVariable::create(fs,myCreatorData,dataBlocks);
	else if (c == '[')
	  mode = FFrItemGroup::create(fs,myCreatorData,dataBlocks);
	else if (c == '{')
	  mode = FFrObjectGroup::create(fs,myCreatorData,dataBlocks);
	break;

      case LABEL_ERROR:
	ungetc(c, fs);
	mode = DONE;
	break;

      default:
	break;
      }

#ifdef FT_USE_PROFILER
  FFaMemoryProfiler::reportMemoryUsage(" readVariables >");
#endif
  return mode == DONE ? LABEL_SEARCH : mode;
}


/*!
  Recursively builds and resolves the file's hierarchy. The return value
  is the length of one binary line (e.g., one time step) in the file.

  Resolving:
   - For FFrFieldEntryBase objects (OG or IG on first level)
     traverse depth-first on all data fields.
   - For IGs, copy the IG information (title) and VRs for that IG
     to a new IG.
   - For VRs create a new FFrVariableReference as a copy of the original
     and insert it into the dataField, adding file info and position.
*/

bool FFrResultContainer::buildAndResolveHierarchy()
{
#if FFR_DEBUG > 1
  std::cout <<"\nFFrResultContainer::buildAndResolveHierarchy() "
            << FFaFilePath::getFileName(myFileName) << std::endl;
#endif

#ifdef FT_USE_PROFILER
  FFaMemoryProfiler::reportMemoryUsage("> buildAndResolveHierarchy"); //70 Mb
#endif

  // KMO: Since binPos is a 4-byte integer and measures the time step size in
  // number of bits, the amount of data per time step is currently limited to
  // 2^31 bits = 2^28 bytes = 256 Mbytes. To remove this limit, either declare
  // binPos and associated variables as FT_int or measure it in bytes instead.
  // In the latter case the limitation will then be raised to 2 Gbytes.
  int binPos = 0;
  // Note: Can not use range-based loop here, since the container will expand
  FFrEntrySet::iterator it;
  for (it = myTopLevelEntries.begin(); it != myTopLevelEntries.end(); ++it)
  {
#if FFR_DEBUG > 2
    std::cout <<"\nTraversing top-level entry "<< (*it)->getType();
    if (!(*it)->isIG()) std::cout <<" "<< (*it)->getDescription();
    std::cout << std::endl;
#endif
    if (!(*it)->isOG() && !(*it)->isGlobal())
      this->collectGarbage(*it);
    binPos = (*it)->traverse(this, NULL, *it, binPos);
  }

#ifdef FT_USE_PROFILER
  FFaMemoryProfiler::reportMemoryUsage("  buildAndResolveHierarchy >");//293 Mb
#endif

  timeStepSize = binPos >> 3;

  // Sort all "Elements" item groups on user ID
  for (FFrEntryBase* entry : myTopLevelEntries)
    if (entry->isOG() && entry->getType() == "Part")
      for (FFrEntryBase* field : static_cast<FFrFieldEntryBase*>(entry)->dataFields)
        if (field->isIG() && field->getType() == "Elements")
          static_cast<FFrFieldEntryBase*>(field)->sortDataFieldsByUserID();

  // After traversal a lot of VRefs and IGs can safely be deleted
#if FFR_DEBUG > 2
  std::cout <<"\nDeleting the left-over variable references"<< std::endl;
#endif
  for (FFrEntryBase* entry : myGarbage) delete entry;
  myGarbage.clear();
  return true;
}


void FFrResultContainer::collectGarbage(FFrEntryBase* entry)
{
  if (std::find(myGarbage.begin(),myGarbage.end(),entry) == myGarbage.end())
  {
    myGarbage.push_back(entry);
    FFrFieldEntryBase* fieldentry = dynamic_cast<FFrFieldEntryBase*>(entry);
    if (fieldentry)
      for (FFrEntryBase* field : fieldentry->dataFields)
        field->setOwner(NULL);
  }
}


/*!
  Reopens the results file using low-lewel IO-routines.
  To handle data files larger than 2GB on Windows.
*/

bool FFrResultContainer::reopenForDataAccess()
{
#if FFR_DEBUG > 1
  std::cout <<"\nFFrResultContainer::reopenForDataAccess() "
	    << FFaFilePath::getFileName(myFileName) << std::endl;
#endif

#ifdef FT_USE_LOWLEVEL_IO
  if (myFile)
    if (Fclose(myFile))
      perror("FFrResultContainer::reopenForDataAccess 1");

  myFile = NULL;
  myDataFile = FT_open(myFileName.c_str(),FT_rb);
  if (myDataFile < 0)
  {
    perror("FFrResultContainer::reopenForDataAccess 2");
    return false;
  }
  if (FT_seek(myDataFile, myHeaderSize, SEEK_SET) == EOF)
  {
    perror("FFrResultContainer::reopenForDataAccess 3");
    return false;
  }
#else
  myDataFile = myFile;
  myFile = NULL;
  if (myDataFile) return true;

  myDataFile = Fopen(myFileName.c_str(),"rb");
  if (!myDataFile)
  {
    perror("FFrResultContainer::reopenForDataAccess 4");
    return false;
  }
  if (Fseek(myDataFile, myHeaderSize, SEEK_SET) == EOF)
  {
    perror("FFrResultContainer::reopenForDataAccess 5");
    return false;
  }
#endif
  myPositionedTimeStep = 0;
  myLastReadEndPos = 0;
  iAmLazyPositioned = false;

  return true;
}


void FFrResultContainer::printInfo() const
{
  FT_int curPos = FT_tell(myDataFile);
  if (FT_seek(myDataFile, (FT_int)0, SEEK_END) != EOF)
  {
    this->printSizeParameters(FT_tell(myDataFile));
    if (FT_seek(myDataFile, curPos, SEEK_SET) != EOF) return;
  }
  perror("FFrResultContainer::printInfo");
}


void FFrResultContainer::printSizeParameters(FT_int fileSize) const
{
  FT_int dataSegmentSizeBytes = fileSize - myHeaderSize;
  std::cout <<"Data info from "   << FFaFilePath::getFileName(myFileName)
            <<"\n  FileSize: "    << fileSize;
  std::cout <<"\n  HeaderSize: "  << myHeaderSize;
  std::cout <<"\n  DataSegmSize: "<< dataSegmentSizeBytes
            <<"\n  TimeStepSize: "<< timeStepSize;
  if (timeStepSize > 0)
    std::cout <<"\n  TimeSteps: " << dataSegmentSizeBytes/timeStepSize;
#if FFR_DEBUG > 1
  std::cout <<"\n  myTopLevelEntries "<< myTopLevelEntries.size();
#endif
  std::cout << std::endl;
}


/*!
  Reads the file from current file position to check if the size has increased.
  Should be called on initial read and for each new "beep" from other processes.
*/

bool FFrResultContainer::readText()
{
#if FFR_DEBUG > 1
  std::cout <<"\nFFrResultContainer::readText() "
	    << FFaFilePath::getFileName(myFileName) << std::endl;
#endif

  // Figure out if the file size has increased
  FT_int oldSize = myHeaderSize;
  if (Fseek(myFile, 0, SEEK_END) == EOF)
  {
#ifdef FFR_DEBUG
    perror("FFrResultContainer::readText");
#endif
    return false;
  }
  myHeaderSize = Ftell(myFile);
  return myHeaderSize > oldSize ? true : false;
}


/*!
  Should be called on initial read and for each new "beep" from other processes.
*/

bool FFrResultContainer::readTimeStepInformation()
{
#if FFR_DEBUG > 1
  std::cout <<"\nFFrResultContainer::readTimeStepInformation() "
	    << FFaFilePath::getFileName(myFileName) << std::endl;
#endif

  // Figure out file size and entities to read
  FT_int curPos = FT_tell(myDataFile);
  if (FT_seek(myDataFile, (FT_int)0, SEEK_END) == EOF)
  {
#ifdef FFR_DEBUG
    perror("FFrResultContainer::readTimeStepInformation 1");
#endif
    return false;
  }
  else if (timeStepSize < 1)
  {
#ifdef FFR_DEBUG
    std::cerr <<"FFrResultContainer::readTimeStepInformation: Logic error, "
              <<" timeStepSize is zero."<< std::endl;
#endif
    return false; // to avoid division by zero on logical errors
  }

  FT_int fileSize = FT_tell(myDataFile);
  FT_int dataSegmentSizeBytes = fileSize - myHeaderSize;
  int stepsInFile = dataSegmentSizeBytes / (FT_int)timeStepSize;

#if FFR_DEBUG > 1
  this->printSizeParameters(fileSize);
#endif

  // Find out if we have read earlier
  int startStep = 0;
  if (!myPhysicalTimeMap.empty())
    startStep = myPhysicalTimeMap.rbegin()->second+1;

#if FFR_DEBUG > 3
  std::cout <<"startStep = "<< startStep << std::endl;
#endif

  // Find out if we have new data in the file
  if (startStep == stepsInFile)
  {
    if (FT_seek(myDataFile, curPos, SEEK_SET) == EOF)
      perror("FFrResultContainer::readTimeStepInformation 2");

#if FFR_DEBUG > 1
    std::cout <<"FFrResultContainer::readTimeStepInformation() "
	      <<"No new time steps, nStep = "<< stepsInFile << std::endl;
#endif
    return false;
  }

  // Locate the Physical Time variable, if not found earlier
  if (!myPhysTimeRef)
  {
    for (FFrEntryBase* entry : myTopLevelEntries)
      if (entry->getDescription() == "Physical time")
        if ((myPhysTimeRef = dynamic_cast<FFrVariableReference*>(entry))) break;

    if (!myPhysTimeRef)
    {
      std::cerr <<"FFrResultContainer: Error in file "<< myFileName
		<<"\n                    No time step data found."<< std::endl;
      return false;
    }
  }

  int physTimeOffsetByte = myPhysTimeRef->containers.front().second >> 3;
  FT_int newPos          = myHeaderSize + startStep*(FT_int)timeStepSize
                         + physTimeOffsetByte;
#if FFR_DEBUG > 3
  std::cout <<"startPos = "<< newPos
            <<"\nstepSize = "<< timeStepSize << std::endl;
#endif

  if (FT_seek(myDataFile, newPos, SEEK_SET) == EOF)
  {
    perror("FFrResultContainer::readTimeStepInformation 3");
    std::cerr <<"   startPos  = "<< newPos
              <<"\n   stepSize  = "<< timeStepSize
              <<"\n   startStep = "<< startStep << std::endl;
    return false;
  }

#if FFR_DEBUG > 1
  std::cout <<"Reading time steps:";
#endif
  int i, nRead;
  double readVal;
  for (i = startStep; i < stepsInFile; i++)
  {
#if FFR_DEBUG > 3
    std::cout <<"\ncurr Pos = "<< FT_tell(myDataFile);
#endif

    // read the Physical time
    if (!swapBytes)
      nRead = FT_read(&readVal, 8, 1, myDataFile);
    else
    {
      // Swap bytes
      char* p = (char*)&readVal;
      char b[8];
      nRead = FT_read(b, 8, 1, myDataFile);
      *p++ = b[7]; *p++ = b[6]; *p++ = b[5]; *p++ = b[4];
      *p++ = b[3]; *p++ = b[2]; *p++ = b[1]; *p   = b[0];
    }
    if (nRead != 1)
      std::cerr <<"FFrResultContainer: Error reading Physical Time for Step "
                << i << std::endl;
#if FFR_DEBUG > 1
    else
#if FFR_DEBUG > 3
      std::cout <<"\nStep "<< i <<" : Time = "<< readVal;
#else
      std::cout << ((i-startStep)%10 ? " " : "\n") << readVal;
#endif
#endif
    myPhysicalTimeMap.insert(myPhysicalTimeMap.end(),std::make_pair(readVal,i));

    newPos = timeStepSize - 8;
    if (FT_seek(myDataFile, newPos, SEEK_CUR) == EOF)
    {
#ifdef FFR_DEBUG
      perror("FFrResultContainer::readTimeStepInformation 4");
#endif
      return false;
    }
  }

  if (FT_seek(myDataFile, curPos, SEEK_SET) == EOF)
  {
#ifdef FFR_DEBUG
    perror("FFrResultContainer::readTimeStepInformation 5");
#endif
    return false;
  }

#if FFR_DEBUG > 1
  std::cout <<"\nSuccessfully read "<< stepsInFile-startStep
            <<" new time steps\n"<< std::endl;
#endif
  return true;
}


double FFrResultContainer::getKey(int flag) const
{
#if FFR_DEBUG > 3
  std::cout <<"FFrResultContainer::getKey("<< flag <<") "
            << FFaFilePath::getFileName(myFileName) << std::endl;
#endif
  if (myPhysicalTimeMap.empty())
    return DBL_MAX;
  else if (flag == 1) // first key
    return myPhysicalTimeMap.begin()->first;
  else if (flag == 2) // last key
    return myPhysicalTimeMap.rbegin()->first;

  return myCurrentIndexIt->first; // current key
}


double FFrResultContainer::getDistanceFromPosKey(bool usePositionedKey) const
{
#if FFR_DEBUG > 3
  std::cout <<"FFrResultContainer::getDistanceFromPosKey()"<< std::endl;
#endif

  if (myPhysicalTimeMap.empty()) return DBL_MAX;

  double wantedKey = myWantedKey;
  if (usePositionedKey)
    wantedKey = myExtractor->getCurrentRDBPhysTime();

#if FFR_DEBUG > 3
  std::cout <<"\tfile: "<< FFaFilePath::getFileName(myFileName)
	    <<" size="<< myPhysicalTimeMap.size()
	    <<" current="<< myCurrentIndexIt->first
	    <<" wanted="<< wantedKey
	    <<" => "<< myCurrentIndexIt->first - wantedKey << std::endl;
#endif

  return myCurrentIndexIt->first - wantedKey;
}


/*!
  Calculates the distance to the next key for this container.
  Returns \e true if the wanted key (as set with positionAtKey())
  is either within or in front of the container values.

  Returns \e false if the wanted key is after the values in the
  container or if the container is empty.
*/

bool FFrResultContainer::getDistanceToNextKey(double& dist)
{
#if FFR_DEBUG > 3
  std::cout <<"FFrResultContainer::getDistanceToNextKey() "
	    << FFaFilePath::getFileName(myFileName) << std::endl;
#endif
  if (myPhysicalTimeMap.empty())
    return false;

  if (myCurrentIndexIt == myPhysicalTimeMap.end())
    return false;
  else if (myWantedKeyStatus == FFR_AFTER_END)
    return false;

  // Find the first time step whose key is larger than the wanted key.
  // This may also be the current time step, in case the wanted key
  // did not match any of the time steps (TT #2903).
  FFrTimeMap::const_iterator it = myPhysicalTimeMap.begin();
  if (myWantedKeyStatus == FFR_INSIDE)
  {
    // Time step tolerance (TT #3011).
    // Changed to relative tolerance 28/7-2017 (Bugfix #498).
    double Trange = myPhysicalTimeMap.rbegin()->first - it->first;
    double eps = (Trange > 1.0 ? Trange : 1.0)*1.0e-12;
    for (it = myCurrentIndexIt; it->first < myWantedKey+eps;)
      if (++it == myPhysicalTimeMap.end())
        return false;
  }

  dist = it->first - myWantedKey;

#if FFR_DEBUG > 3
  std::cout <<"-->NearestNext: "<< myWantedKey <<"\t"<< myCurrentIndexIt->first
	    <<"\t"<< it->first <<"\t("<< dist <<")\t"
	    << FFaFilePath::getFileName(myFileName) << std::endl;
#endif
  return true;
}


int FFrResultContainer::positionAtKey(double key, bool getNextHigher)
{
#if FFR_DEBUG > 3
  std::cout <<"FFrResultContainer::positionAtKey() "
	    << FFaFilePath::getFileName(myFileName)
	    <<" key = "<< key << std::endl;
#endif
  if (myStatus == FFR_DATA_CLOSED)
    this->updateContainerStatus();

  if (myPhysicalTimeMap.empty())
  {
#if FFR_DEBUG > 3
    std::cout <<"FFrResultContainer::positionAtKey() FFR_NOT_SET"<< std::endl;
#endif
    return FFR_NOT_SET;
  }

  myWantedKey = key;

  // find the step corresponding to the key:

  // check the ranges - wanted time should be inside max and min time
  if (myPhysicalTimeMap.begin()->first > key)
  {
    myWantedKeyStatus = FFR_BEFORE_START;
    myCurrentIndexIt = myPhysicalTimeMap.begin();
  }
  else if (key > myPhysicalTimeMap.rbegin()->first)
  {
    myWantedKeyStatus = FFR_AFTER_END;
    myCurrentIndexIt = myPhysicalTimeMap.end();
    myCurrentIndexIt--;
  }
  else
  {
    myWantedKeyStatus = FFR_INSIDE;
    if (getNextHigher)
      myCurrentIndexIt = myPhysicalTimeMap.upper_bound(key - FLT_EPSILON);
    else
    {
      myCurrentIndexIt = myPhysicalTimeMap.upper_bound(key + FLT_EPSILON);
      if (myCurrentIndexIt != myPhysicalTimeMap.begin())
	myCurrentIndexIt--;
    }
#if FFR_DEBUG > 3
    std::cout <<"FFrResultContainer::positionAtKey() FFR_INSIDE\t"
	      << myCurrentIndexIt->first <<"\t"
	      << myCurrentIndexIt->second << std::endl;
#endif
  }

  // if set to the same time step - do not change the file pointer
  if (myPositionedTimeStep != myCurrentIndexIt->second)
    iAmLazyPositioned = true;

  return myWantedKeyStatus;
}


void FFrResultContainer::resetPositioning()
{
#if FFR_DEBUG > 2
  std::cout <<"FFrResultContainer::resetPositioning()"<< std::endl;
#endif
  this->updateContainerStatus();

  if (myStatus == FFR_DATA_PRESENT || myStatus == FFR_NEW_DATA)
  {
    if (FT_seek(myDataFile, myHeaderSize, SEEK_SET) == EOF)
      perror("FFrResultContainer::resetPositioning");
    else
    {
      myPositionedTimeStep = 0;
      myLastReadEndPos = 0;
      iAmLazyPositioned = false;
    }
  }
}


void FFrResultContainer::fillPreRead()
{
#if FFR_DEBUG > 2
  std::cout <<"FFrResultContainer::fillPreRead()"<< std::endl;
#endif

  // Calculate the File pointer move :
  FT_int moveDistance = 0;
  if (iAmLazyPositioned)
  {
    int deltaSteps = myCurrentIndexIt->second - myPositionedTimeStep;
    moveDistance = deltaSteps*(FT_int)timeStepSize - myLastReadEndPos;
    myPositionedTimeStep = myCurrentIndexIt->second;
    myLastReadEndPos = 0;
    iAmLazyPositioned = false;
  }

  // Move file pointer
  if (moveDistance != 0)
    if (FT_seek(myDataFile, moveDistance, SEEK_CUR) == EOF)
      perror("FFrResultContainer::fillPreRead");

  // Check if the pre-read buffer is empty
  if (!myPreRead)
  {
    myPreRead = new char[timeStepSize];
    if (!myPreRead) throw std::bad_alloc();
#if FFR_DEBUG > 1
    std::cout <<"\nFFrResultContainer::fillPreRead(): Cache size for "
              << FFaFilePath::getFileName(myFileName) <<": "<< timeStepSize << std::endl;
#endif
  }

  // Read the time step data
  int readBytes = FT_read(myPreRead, 1, timeStepSize, myDataFile);
  if (readBytes != timeStepSize)
    std::cerr <<"FFrResultContainer: Error during data prefetch."
              <<"\n\tRead "<< readBytes <<" bytes of "<< timeStepSize
              <<"\n\tFile "<< FFaFilePath::getFileName(myFileName) << std::endl;

  myLastReadEndPos  = timeStepSize;
  myPreReadTimeStep = myPositionedTimeStep;
}


/*!
  \param[out] var The memory segment to read into
  \param[in] nvals Number of values (cell) that we want to read
  \param[in] bitPos File position to start reading from
  \param[in] cellBits Number of bits in each cell
  \param[in] repeats Number of values present on file for the variable to read

  The data values associated with a variable is read from the physical file.
  Note: nvals < repeats is not an error condition (it used to be earlier).
  It should be allowed to read only a part of a variable, if the user wants to.
*/

int FFrResultContainer::actualRead(void* var, int nvals,
				   int bitPos, int cellBits, int repeats)
{
  int nRead = nvals < repeats ? nvals : repeats;
  if (nRead < 1) return 0; // nothing to read (silently ignore)

#if FFR_DEBUG > 3
  std::cout <<"FFrResultContainer::actualRead() n="<< nvals <<","<< repeats;
#endif

  // Check that we actually have some binary data before trying to read
  if (myStatus < FFR_DATA_PRESENT)
  {
#if FFR_DEBUG > 3
    std::cout <<": No data yet"<< std::endl;
#endif
    return 0;
  }

  int bytePos   = bitPos >> 3;
  int cellBytes = cellBits >> 3;

  if (iAmPreReading)
  {
    if (!myPreRead)
      this->fillPreRead();
    else if (myPositionedTimeStep != myCurrentIndexIt->second)
      this->fillPreRead();
    else if (myPreReadTimeStep != myPositionedTimeStep)
      this->fillPreRead();

    memcpy(var, &myPreRead[bytePos], nRead*cellBytes);
  }
  else
  {
    FT_int moveDistance = 0;
    if (iAmLazyPositioned)
    {
      iAmLazyPositioned = false;
      moveDistance = (myCurrentIndexIt->second - myPositionedTimeStep)*(FT_int)timeStepSize - myLastReadEndPos;
      myPositionedTimeStep = myCurrentIndexIt->second;
      myLastReadEndPos = 0;
    }

    moveDistance += bytePos - myLastReadEndPos;
    if (moveDistance != 0)
    {
#if FFR_DEBUG > 3
      std::cout <<" moveDistance = "<< moveDistance << std::endl;
#endif
      if (FT_seek(myDataFile, moveDistance, SEEK_CUR) == EOF)
      {
	perror("FFrResultContainer::actualRead");
	return 0;
      }
    }

    int readBytes = FT_read(var, cellBytes, nRead, myDataFile);
    if (readBytes != nRead)
      std::cerr <<"FFrResultContainer: Could not read specified amount of data "
		<<"(read "<< readBytes <<" of wanted "<< nRead <<", time step "
		<< myPositionedTimeStep <<")"<< std::endl;

    myLastReadEndPos = (bitPos + cellBits*nRead) >> 3;
  }
#if FFR_DEBUG > 3
  std::cout <<" nRead = "<< nRead << std::endl;
#endif

  if (swapBytes)
  {
    char* swapBuf = new char[cellBytes*nRead];
    char* chVar   = (char*)var;
    size_t k = 0;

    memcpy(swapBuf, var, nRead*cellBytes);
    for (int i = 1; i <= nRead; i++)
      for (int j = 1; j <= cellBytes; j++, k++)
        chVar[k] = swapBuf[i*cellBytes-j];

    delete[] swapBuf;
  }

  return nRead;
}
