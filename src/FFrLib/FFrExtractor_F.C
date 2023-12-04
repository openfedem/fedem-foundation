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
  e.g., during recovery and/or restart operations.

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

static FFrExtractor* rdb     = NULL; //!< Pointer to the extractor object
static FFrEntryBase* stepPtr = NULL; //!< Pointer to the step number variable


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
  if (!rdb) rdb = new FFrExtractor;
  if (!rdb)
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
      ierr = rdb->addFiles(files,false,true) ? 0 : 1;
    }
  }
  else
  {
    ListUI <<"\n   * Reading results file "<< FFaFilePath::checkName(fnames);
    ierr = rdb->addFile(fnames,true) ? 0 : 1;
  }
  ListUI <<"\n\n";
}


////////////////////////////////////////////////////////////////////////////////
//! \brief Releases the FFrExtractor object.

SUBROUTINE(ffr_done,FFR_DONE) ()
{
  if (rdb) delete rdb;
  rdb = NULL;
  stepPtr = NULL;
}


////////////////////////////////////////////////////////////////////////////////
//! \brief Static helper for finding file position of a result variable.
//! \param[in] path Variable description
//! \param[in] ogType Type name of the object we are searching results for
//! \param[in] baseId Unique identifier of the object we are searching for
//!
//! \details This function computes the relative file position pointer within a
//! time step for the data pertaining to the specified variable or item group,
//! within the specified owner group identified by \a ogType and \a baseId.
//!
//! The variable- or item group specification string \a path contains the
//! full path of the wanted object in the result database hierarchy using
//! the '|' character to separate between the different levels.
//!
//! \author Knut Morten Okstad
//! \date 14 Nov 2000
////////////////////////////////////////////////////////////////////////////////

static FFrEntryBase* findPtr(const std::string& path, const std::string& ogType,
#if FFR_DEBUG > 1
                             int baseId, const char* dbgMsg = NULL
#else
                             int baseId, const char* = NULL
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
  if (dbgMsg) std::cout << dbgMsg << entry;
#endif
  return rdb->search(entry);
}


////////////////////////////////////////////////////////////////////////////////
//! \brief Finds file position of the specified result variable.
//! \sa findPtr()
//!
//! \author Knut Morten Okstad
//! \date 14 Nov 2000
////////////////////////////////////////////////////////////////////////////////

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
//! \details Finds the data pertaining to the specified variable or item group
//! within the owner group identified by \a objectType and \a baseId.
//!
//! The variable- or item group specification string \a pathName contains the
//! full path of the wanted object in the result database hierarchy using
//! the '|' character to separate between the different levels.
//!
//! \author Knut Morten Okstad
//! \date 24 Jun 2002
////////////////////////////////////////////////////////////////////////////////

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

  ierr = rdb->getSingleTimeStepData(ptr,data,nw) - nw;
#if FFR_DEBUG > 1
  std::cout <<"        Data:";
  for (int i = 0; i < nw+ierr; i++) std::cout <<" "<< data[i];
  if (ierr) std::cout <<"\n        ierr:"<< ierr <<", ptr: "<< (ptr_int)ptr;
  std::cout << std::endl;
#endif
}

////////////////////////////////////////////////////////////////////////////////
//! \brief Locates results data within current time step for specified variable.
//! \details Finds the data pertaining to the specified integer variable
//! within the owner group identified by \a objectType and \a baseId.
//!
//! The variable specification string \a pathName contains the
//! full path of the wanted object in the result database hierarchy using
//! the '|' character to separate between the different levels.
//!
//! \author Knut Morten Okstad
//! \date 24 Jun 2002
////////////////////////////////////////////////////////////////////////////////

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

  ierr = rdb->getSingleTimeStepData(ptr,data,nw) - nw;
#if FFR_DEBUG > 1
  std::cout <<"       Data:";
  for (int i = 0; i < nw+ierr; i++) std::cout <<" "<< data[i];
  if (ierr) std::cout <<"\n       ierr:"<< ierr <<", ptr: "<< (ptr_int)ptr;
  std::cout << std::endl;
#endif
}


////////////////////////////////////////////////////////////////////////////////
//! \brief Positions the results file(s) for the specified time step.
//! \details Position all result containers of the extractor such that the next
//! ffr_getdata() operations read result data from the time step corresponding
//! to the physical time \a atime (or the step that is closest to this time).
//! The physical time of the time step that is found is returned through
//! the variable \a btime.
//!
//! \note Current implementation finds the next larger position through use of
//! the boolean flag \a getNextHigh.
//!
//! \author Knut Morten Okstad
//! \date 14 Nov 2000
////////////////////////////////////////////////////////////////////////////////

SUBROUTINE(ffr_setposition,FFR_SETPOSITION) (const double& atime,
                                             double& btime, f90_int8& istep)
{
  if (!stepPtr)
    stepPtr = rdb->getTopLevelVar("Time step number");

  int jstep = 0;
  bool getNextHigh = true;
  if (!rdb->positionRDB(atime,btime,getNextHigh))
    istep = -1;
  else if (rdb->getSingleTimeStepData(stepPtr,&jstep,1) != 1)
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
//! \details Increment sll result containers of the extractor such that the next
//! ffr_getdata() operations read result data from the next time step, if any.
//! The physical time of the time step that is found is returned through
//! the variable \a btime.
//!
//! \author Knut Morten Okstad
//! \date 21 May 2001
////////////////////////////////////////////////////////////////////////////////

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
  else if (!rdb->incrementRDB())
    istep = -1;
  else if (rdb->getSingleTimeStepData(stepPtr,&jstep,1) != 1)
    istep = -2;
  else
    istep = jstep; // Temporary cast from int until 64-bit integer is supported

  btime = rdb->getCurrentRDBPhysTime();
#ifdef FFR_DEBUG
  std::cout <<" istep="<< istep <<" time="<< btime << std::endl;
#endif
}


////////////////////////////////////////////////////////////////////////////////
//! \brief Reads the contents of the variable or item group.
//! \details Reads the data pointed to by \a varPtr within current time step,
//! as set by ffr_setposition() from the results database.
//!
//! \author Knut Morten Okstad
//! \date 14 Nov 2000
////////////////////////////////////////////////////////////////////////////////

SUBROUTINE(ffr_getdata,FFR_GETDATA) (const double* data, const f90_int& nw,
                                     const ptr_int& varPtr, f90_int& ierr)
{
  ierr = rdb->getSingleTimeStepData((const FFrEntryBase*)varPtr,data,nw) - nw;
#if FFR_DEBUG > 1
  std::cout <<"FFr_getData: Ptr= "<< varPtr <<" Data:";
  for (int i = 0; i < nw+ierr; i++) std::cout <<" "<< data[i];
  std::cout << std::endl;
#endif
}
