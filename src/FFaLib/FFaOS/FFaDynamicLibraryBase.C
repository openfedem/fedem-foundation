// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

/*!
  \file FFaDynamicLibraryBase.C
  \brief Management of dynamic loading of shared object libraries.
*/

#include <cstdlib>
#include <cstring>
#if defined(win32) || defined(win64)
#include <cctype>
#else
#include <dlfcn.h>
#endif

#include "FFaLib/FFaOS/FFaDynamicLibraryBase.H"
#include "FFaLib/FFaOS/FFaFilePath.H"
#include "FFaLib/FFaDefinitions/FFaAppInfo.H"
#include "FFaLib/FFaDefinitions/FFaMsg.H"


/*!
  \brief Loads the library named \a libName.
  \returns \e true if OK and \e false if the library is not available.
*/

bool FFaDynamicLibraryBase::load(const std::string& libName, bool silence)
{
  if (libName.empty()) return false;

  // Append proper file extension to libName, if needed
  std::string lName(libName);
#if defined(win32) || defined(win64)
  for (size_t i = lName.find_last_of('.'); i < lName.size()-1; i++)
    lName[1+i] = tolower(lName[1+i]);
  const char* ext = ".dll";
#elif defined(hpux) || defined(hpux64)
  const char* ext = ".sl";
#else
  const char* ext = ".so";
#endif
  if (lName.rfind(ext) != lName.size()-strlen(ext))
    lName += ext;
  if (myLibHandles.find(lName) != myLibHandles.end())
    return false;

  FFaFilePath::makeItAbsolute(lName,FFaAppInfo::getProgramPath());
#if defined(win32) || defined(win64)
  LibHandle aLib = std::make_pair(Undefined,::LoadLibrary(lName.c_str()));
#else
  LibHandle aLib = std::make_pair(Undefined,::dlopen(lName.c_str(),RTLD_LAZY));
#endif

  if (aLib.second)
  {
    myLibHandles[lName] = aLib;
    if (!silence)
      ListUI <<"\nNote :    Loading dynamic shared library "<< lName <<"\n";
    return true;
  }

  if (!silence)
    ListUI <<"\nError :   Failed to load dynamic shared library "<< lName <<"\n";

  return false;
}


/*!
  \brief Unloads the library named \a libName.
  \return \e true if found and \e false if the library was not loaded.
*/

bool FFaDynamicLibraryBase::unload(const std::string& libName, bool silence)
{
  if (libName.empty()) return false;

  std::map<std::string,LibHandle>::iterator it;
  for (it = myLibHandles.begin(); it != myLibHandles.end(); ++it)
    if (it->first.find(libName) < std::string::npos)
    {
      if (!silence)
        ListUI <<"\nNote :    Unloading dynamic shared library "
               << it->first <<"\n";

#if defined(win32) || defined(win64)
      ::FreeLibrary(it->second.second);
#else
      ::dlclose(it->second.second);
#endif
      myLibHandles.erase(it);
      myProcCache.clear();
      return true;
    }

  return false;
}


/*!
  \brief Unloads all libraries and clears the function cache.
*/

void FFaDynamicLibraryBase::unloadAll()
{
  if (myLibHandles.empty()) return;

  ListUI <<"\nNote :    Unloading "<< (int)myLibHandles.size()
         <<" dynamic shared libraries\n";

  for (std::pair<const std::string,LibHandle>& lib : myLibHandles)
#if defined(win32) || defined(win64)
    ::FreeLibrary(lib.second.second);
#else
    ::dlclose(lib.second.second);
#endif

  myLibHandles.clear();
  myProcCache.clear();
}


const char* FFaDynamicLibraryBase::getLibrary(size_t idx) const
{
  for (const std::pair<const std::string,LibHandle>& lib : myLibHandles)
    if (--idx == 0) return lib.first.c_str();

  return NULL;
}


/*!
  \brief Returns the pointer to a named function.
*/

DLPROC FFaDynamicLibraryBase::getAddress(const LibHandle& lib,
                                         const std::string& fName)
{
#if defined(win32) || defined(win64)
  return (DLPROC)::GetProcAddress(lib.second,fName.c_str());
#else
  return (DLPROC)::dlsym(lib.second,fName.c_str());
#endif
}


/*!
  \brief Returns the function pointer for the named function.
  \details Does not store the pointer in the cache, so this method should
  only be used for non-critical functions invoked only a few times.
*/

DLPROC FFaDynamicLibraryBase::getProcAddr(const std::string& fName,
                                          bool silence) const
{
  DLPROC p = NULL;
  for (const std::pair<const std::string,LibHandle>& lib : myLibHandles)
    if ((p = getAddress(lib.second,fName)))
      break;

  if (silence || p) return p;

  ListUI <<"Error :   Failed to obtain address of function "<< fName;
  if (!myLibHandles.empty())
  {
    std::map<std::string,LibHandle>::const_iterator it = myLibHandles.begin();
    ListUI <<"\n          ["<< it->first;
    for (++it; it != myLibHandles.end(); ++it) ListUI <<", "<< it->first;
    ListUI <<"]";
  }
  ListUI <<"\n";

  return p;
}


/*!
  \brief Returns the function pointer for the named function.
  \details The language binding is set depending on whether the provided
  \a cName (C/C++) or \a fName (Fortran) is found.
*/

DLPROC FFaDynamicLibraryBase::getProcAddr(const std::string& cName,
                                          const std::string& fName,
                                          LanguageBinding& lang,
                                          bool silence) const
{
  DLPROC p = NULL;
  for (const std::pair<const std::string,LibHandle>& lib : myLibHandles)
  {
    switch (lib.second.first) {
    case C:
      p = getAddress(lib.second,cName);
      lang = C;
      break;
    case Fortran:
      p = getAddress(lib.second,fName);
      lang = Fortran;
      break;
    default:
      if ((p = getAddress(lib.second,cName)))
        const_cast<LibHandle&>(lib.second).first = lang = C;
      else if ((p = getAddress(lib.second,fName)))
        const_cast<LibHandle&>(lib.second).first = lang = Fortran;
      break;
    }
    if (p) break;
  }

  if (silence || p) return p;

  ListUI <<"Error :   Failed to obtain address of function "<< cName
         <<" and "<< fName;
  if (!myLibHandles.empty())
  {
    std::map<std::string,LibHandle>::const_iterator it = myLibHandles.begin();
    ListUI <<"\n          ["<< it->first;
    for (++it; it != myLibHandles.end(); ++it) ListUI <<", "<< it->first;
    ListUI <<"]";
  }
  ListUI <<"\n";

  return p;
}


/*!
  \brief Returns the function pointer for the given function.
  \details The pointer is stored in a local cache to speed up repeated calls.
*/

DLPROC FFaDynamicLibraryBase::getProcAddress(const std::string& fName,
                                             size_t procID) const
{
  if (myLibHandles.empty()) return NULL;

  if (procID >= myProcCache.size())
    myProcCache.resize(procID+1,cache_info(NULL,Undefined));

  cache_info& ci = myProcCache[procID];
  if (!ci.first && !ci.second)
    ci = cache_info(this->getProcAddr(fName),C);

  return ci.first;
}


/*!
  \brief Returns the function pointer for the given function.
  \details The pointer is stored in a local cache to speed up repeated calls.
*/

DLPROC FFaDynamicLibraryBase::getProcAddress(const std::string& cName,
                                             const std::string& fName,
                                             LanguageBinding& lang,
                                             size_t procID) const
{
  if (myLibHandles.empty()) return NULL;

  if (procID >= myProcCache.size())
    myProcCache.resize(procID+1,cache_info(NULL,Undefined));

  cache_info& ci = myProcCache[procID];
  if (!ci.first && !ci.second)
    ci.first = this->getProcAddr(cName,fName,ci.second);

  lang = ci.second;
  return ci.first;
}
