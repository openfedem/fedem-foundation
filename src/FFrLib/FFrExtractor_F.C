// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

/*!
  \file FFrExtractor_F.C
  \brief Fortran wrapper for the FFrExtractor methods.

  \details This file contains some global-scope functions callable from Fortran.
  It only wraps the functionality needed by the solvers for accessing the RDB,
  e.g., during recovery and/or restart operations. This includes the following:

  - ffrextractorinterface::ffr_init
  - ffrextractorinterface::ffr_done
  - ffrextractorinterface::ffr_findptr
  - ffrextractorinterface::ffr_setposition
  - ffrextractorinterface::ffr_increment
  - ffrextractorinterface::ffr_getdata
  - ffrextractorinterface::ffr_finddata

  No further documentation is provided here.
  The above wrappers are documented in the FFrExtractorInterface.f90 file.

  \author Knut Morten Okstad
  \date 9 Oct 2000
*/

#include "FFrLib/FFrExtractor.H"
#include "FFaLib/FFaCmdLineArg/FFaCmdLineArg.H"
#include "FFaLib/FFaString/FFaTokenizer.H"
#include "FFaLib/FFaDefinitions/FFaResultDescription.H"
#include "FFaLib/FFaDefinitions/FFaMsg.H"
#include "FFaLib/FFaOS/FFaFilePath.H"
#include "FFaLib/FFaOS/FFaFortran.H"


namespace
{
  FFrExtractor* ourRdb  = NULL; //!< Pointer to the extractor object
  FFrEntryBase* stepPtr = NULL; //!< Pointer to the step number variable

  //! \brief Helper for finding file position of a result variable.
  FFrEntryBase* findPtr(const std::string& path,
                        const std::string& ogType, int baseId,
#if FFR_DEBUG > 1
                        const char* dbgMsg
#else
                        const char*
#endif
 ){
    FFaResultDescription entry;
    entry.baseId = baseId;
    entry.OGType = ogType;

    // Split up the path description into a vector of strings
    size_t iStart = 0, iEnd, nChr;
    while (iStart != std::string::npos)
    {
      iEnd = path.find('|',iStart);
      nChr = iEnd == std::string::npos ? iEnd : iEnd-iStart;
      entry.varDescrPath.push_back(path.substr(iStart,nChr));
      iStart = iEnd == std::string::npos ? iEnd : iEnd+1;
    }

#if FFR_DEBUG > 1
    std::cout << dbgMsg << entry;
#endif
    return ourRdb->search(entry);
  }
}


////////////////////////////////////////////////////////////////////////////////
//! \brief Opens the results database and reads file headers.

SUBROUTINE(ffr_init,FFR_INIT) (const char* file,
#ifdef _NCHAR_AFTER_CHARARG
                               const int nchar, f90_int& ierr
#else
                               f90_int& ierr, const int nchar
#endif
){
  ierr = 1;
  if (!ourRdb)
    ourRdb = new FFrExtractor();
  if (!ourRdb)
  {
    std::cerr <<"FFr_init: Error allocating extractor object."<< std::endl;
    return;
  }

  // Get rdb file name(s) from the command-line
  std::string fnames;
  FFaCmdLineArg::instance()->getValue(std::string(file,nchar),fnames);

  if (fnames.empty())
    ListUI <<" *** Error: No results database files specified";
  else if (fnames[0] == '<')
  {
    // We have a multi-file list
    FFaTokenizer files(fnames,'<','>',',');
    if (files.empty())
      ListUI <<" *** Error: No results database files specified";
    else
    {
      for (std::string& file : files)
        ListUI <<"\n   * Reading results file "<< FFaFilePath::checkName(file);
      ierr = ourRdb->addFiles(files,false,true,true) ? 0 : 1;
    }
  }
  else
  {
    ListUI <<"\n   * Reading results file "<< FFaFilePath::checkName(fnames);
    ierr = ourRdb->addFile(fnames,true,true) ? 0 : 1;
  }
  ListUI <<"\n\n";
}


////////////////////////////////////////////////////////////////////////////////
//! \brief Releases the FFrExtractor object.

SUBROUTINE(ffr_done,FFR_DONE) ()
{
  delete ourRdb;
  ourRdb = NULL;
  stepPtr = NULL;
}


////////////////////////////////////////////////////////////////////////////////
//! \brief Finds file position of the specified result variable.

SUBROUTINE(ffr_findptr,FFR_FINDPTR) (const char* pathName,
#ifdef _NCHAR_AFTER_CHARARG
                                     const int ncharP,
                                     const char* objectType, const int ncharO,
                                     const f90_int& baseId,  ptr_int& varPtr
#else
                                     const char* objectType,
                                     const f90_int& baseId,  ptr_int& varPtr,
                                     const int ncharP, const int ncharO
#endif
){
  varPtr = (size_t)findPtr(std::string(pathName,ncharP),
			   std::string(objectType,ncharO),
			   baseId, "FFr_findPtr: ");
#if FFR_DEBUG > 1
  std::cout <<" Ptr="<< varPtr << std::endl;
#endif
}

////////////////////////////////////////////////////////////////////////////////
//! \brief Locates results data within current time step for specified variable.

SUBROUTINE(ffr_realdata,FFR_REALDATA) (const double* data, const f90_int& nw,
#ifdef _NCHAR_AFTER_CHARARG
                                       const char* pathName,   const int ncharP,
                                       const char* objectType, const int ncharO,
                                       const f90_int& baseId,  f90_int&  ierr
#else
                                       const char* pathName,
                                       const char* objectType,
                                       const f90_int& baseId,  f90_int&  ierr,
                                       const int   ncharP,     const int ncharO
#endif
){
  FFrEntryBase* ptr = findPtr(std::string(pathName,ncharP),
			      std::string(objectType,ncharO),
			      baseId, "FFr_realData: ");

  ierr = ourRdb->getSingleTimeStepData(ptr,data,nw) - nw;
#if FFR_DEBUG > 1
  std::cout <<"        Data:";
  for (int i = 0; i < nw+ierr; i++) std::cout <<" "<< data[i];
  if (ierr) std::cout <<"\n        ierr:"<< ierr <<", ptr: "<< (ptr_int)ptr;
  std::cout << std::endl;
#endif
}

////////////////////////////////////////////////////////////////////////////////
//! \brief Locates results data within current time step for specified variable.

SUBROUTINE(ffr_intdata,FFR_INTDATA) (const f90_int* data, const f90_int& nw,
#ifdef _NCHAR_AFTER_CHARARG
                                     const char* pathName,   const int ncharP,
                                     const char* objectType, const int ncharO,
                                     const f90_int& baseId,  f90_int&  ierr
#else
                                     const char* pathName,
                                     const char* objectType,
                                     const f90_int& baseId,  f90_int&  ierr,
                                     const int   ncharP,     const int ncharO
#endif
){
  FFrEntryBase* ptr = findPtr(std::string(pathName,ncharP),
			      std::string(objectType,ncharO),
			      baseId, "FFr_intData: ");

  ierr = ourRdb->getSingleTimeStepData(ptr,data,nw) - nw;
#if FFR_DEBUG > 1
  std::cout <<"       Data:";
  for (int i = 0; i < nw+ierr; i++) std::cout <<" "<< data[i];
  if (ierr) std::cout <<"\n       ierr:"<< ierr <<", ptr: "<< (ptr_int)ptr;
  std::cout << std::endl;
#endif
}


////////////////////////////////////////////////////////////////////////////////
//! \brief Positions the results file(s) for the specified time step.

SUBROUTINE(ffr_setposition,FFR_SETPOSITION) (const double& atime,
                                             double& btime, f90_int8& istep)
{
  if (!stepPtr)
    stepPtr = ourRdb->getTopLevelVar("Time step number");

  int jstep = 0;
  bool getNextHigh = true;
  if (!ourRdb->positionRDB(atime,btime,getNextHigh))
    istep = -1;
  else if (ourRdb->getSingleTimeStepData(stepPtr,&jstep,1) != 1)
    istep = -2;
  else
    istep = jstep; // Temporary cast from int until 64-bit integer is supported

#ifdef FFR_DEBUG
  std::cout <<" istep="<< istep <<" atime="<< atime <<" btime="<< btime
            << std::endl;
#endif
}


////////////////////////////////////////////////////////////////////////////////
//! \brief Positions the results file(s) for the next time step.

SUBROUTINE(ffr_increment,FFR_INCREMENT) (double& btime, f90_int8& istep)
{
  int jstep = 0;
  if (!stepPtr)
  {
    std::cerr <<"FFr_increment: Internal error: Must invoke "
              <<"FFr_setPosition first."<< std::endl;
    istep = -999;
    return;
  }
  else if (!ourRdb->incrementRDB())
    istep = -1;
  else if (ourRdb->getSingleTimeStepData(stepPtr,&jstep,1) != 1)
    istep = -2;
  else
    istep = jstep; // Temporary cast from int until 64-bit integer is supported

  btime = ourRdb->getCurrentRDBPhysTime();
#ifdef FFR_DEBUG
  std::cout <<" istep="<< istep <<" time="<< btime << std::endl;
#endif
}


////////////////////////////////////////////////////////////////////////////////
//! \brief Reads the contents of the variable or item group.

SUBROUTINE(ffr_getdata,FFR_GETDATA) (const double* data, const f90_int& nw,
                                     const ptr_int& varPtr, f90_int& ierr)
{
  ierr = ourRdb->getSingleTimeStepData((const FFrEntryBase*)varPtr,data,nw)-nw;
#if FFR_DEBUG > 1
  std::cout <<"FFr_getData: Ptr= "<< varPtr <<" Data:";
  for (int i = 0; i < nw+ierr; i++) std::cout <<" "<< data[i];
  std::cout << std::endl;
#endif
}
