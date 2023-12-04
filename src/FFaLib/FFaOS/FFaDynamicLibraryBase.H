// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

/*!
  \file FFaDynamicLibraryBase.H
  \brief Management of dynamic loading of shared object libraries.
*/

#ifndef FFA_DYNAMIC_LIBRARY_BASE_H
#define FFA_DYNAMIC_LIBRARY_BASE_H

#include <map>
#include <vector>
#include <string>

#if defined(win32) || defined(win64)
#define NOGDI
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
//! \brief Function pointer on Windows.
typedef void (*DLPROC) ();
#else
//! \brief Function pointer on UNIX.
typedef void* DLPROC;
#endif


/*!
  \brief Base class for dynamic loading of shared object libraries.
*/

class FFaDynamicLibraryBase
{
protected:
  //! \brief Default constructor.
  FFaDynamicLibraryBase() {}
  //! \brief The destructor unloads all dynamically loaded libraries.
  virtual ~FFaDynamicLibraryBase() { this->unloadAll(); }

  //! \brief Enums defining the supported language bindings.
  enum LanguageBinding { Undefined, C, Fortran };

public:
  bool load(const std::string& libName, bool silence = false);
  bool unload(const std::string& libName, bool silence = false);
  void unloadAll();
  //! \brief Returns whether dynamics libraries have been loaded or not.
  bool areLibsLoaded() const { return !myLibHandles.empty(); }
  //! \brief Returns the file name of the dynamic library, if loaded.
  const char* getLibrary(size_t idx = 1) const;

protected:
  DLPROC getProcAddr(const std::string& fName, bool silence = false) const;
  DLPROC getProcAddr(const std::string& cName, const std::string& fName,
                     LanguageBinding& lang, bool silence = false) const;
  DLPROC getProcAddress(const std::string& fName, size_t procID) const;
  DLPROC getProcAddress(const std::string& cName, const std::string& fName,
                        LanguageBinding& lang, size_t procID) const;

private:
  //! Dynamic library handle with associated language binding
#if defined(win32) || defined(win64)
  typedef std::pair<LanguageBinding,HINSTANCE> LibHandle;
#else
  typedef std::pair<LanguageBinding,void*> LibHandle;
#endif
  //! Function pointer with language binding
  typedef std::pair<DLPROC,LanguageBinding> cache_info;

  mutable std::vector<cache_info> myProcCache;  //!< Function pointer cache
  std::map<std::string,LibHandle> myLibHandles; //!< Dynamic library container

  //! \brief Returns the pointer to a named function.
  static DLPROC getAddress(const LibHandle& lib, const std::string& fname);
};

#endif
