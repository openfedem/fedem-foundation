// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#include "FFaLib/FFaDefinitions/FFaAppInfo.H"
#include "Admin/FedemAdmin.H"
#if defined(win32) || defined(win64)
#include <iostream>
#include <stdio.h>
#include <windows.h>
#include <direct.h>
#include <io.h>
#else
#include <unistd.h>
#endif
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#if _MSC_VER > 1310
#define getcwd _getcwd
#endif


char FFaAppInfo::consoleFlag = 0; // Default is GUI only (no console)
int FFaAppInfo::runInConsole = -1;
bool FFaAppInfo::iAmInCwd = false;
std::string FFaAppInfo::programPath;


/*!
  \brief Checks if the named executable \a fileName exists or not.
*/

static bool fileExists(const std::string& fileName)
{
#if defined(win32) || defined(win64)
  struct _stat buf;
  return (_stat(fileName.c_str(),&buf) == 0 ||
          _stat((fileName+".exe").c_str(),&buf) == 0);
#else
  struct  stat buf;
  return ( stat(fileName.c_str(),&buf) == 0);
#endif
}


void FFaAppInfo::openConsole(bool withGUI)
{
  // Check whether this program is from from a console,
  // or if it is launched as a Window app without its own console
  if (runInConsole < 0)
#if defined(win32) || defined(win64)
    runInConsole = _isatty(_fileno(stdin));
#else
    runInConsole = isatty(fileno(stdin));
#endif

#if defined(win32) || defined(win64)
  if (consoleFlag == 0 && runInConsole)
  {
    AllocConsole();
    freopen("conin$",  "r", stdin);
    freopen("conout$", "w", stdout);
    freopen("conout$", "w", stderr);
  }
#endif

  consoleFlag = withGUI ? 2 : 1;
}


#if defined(win32) || defined(win64)
void FFaAppInfo::closeConsole(bool acknowledge)
#else
void FFaAppInfo::closeConsole(bool)
#endif
{
  if (consoleFlag == 0) return;

  consoleFlag = 0;
  if (runInConsole < 1) return;

#if defined(win32) || defined(win64)
  if (acknowledge)
  {
    fflush(stdout);
    fflush(stderr);
    std::cout <<"\n\nPress ENTER key to close this window ... ";
    std::cin.get();
  }
  FreeConsole();
#endif
}


FFaAppInfo::FFaAppInfo()
{
  version = FedemAdmin::getVersion() + std::string(" ")
          + FedemAdmin::getBuildDate();

#if defined(win32) || defined(win64)
  DWORD cchBuff = BUFSIZ;
  TCHAR tchBuffer[BUFSIZ];
  LPTSTR uname = tchBuffer;
  GetUserName(uname,&cchBuff);
  user = uname;
#else
  char* uname = getenv("USER");
  user = uname ? uname : "(none)";
#endif

  const time_t currentTime = time(NULL);
  date = ctime(&currentTime);
  date.erase(date.size()-1); // Erasing the trailing newline character
}


void FFaAppInfo::init(const char* program)
{
  // Initialize the program path (without the trailing slash)
  std::string fullPath(program);
  size_t prevSPos = fullPath.size();
  size_t slashPos = fullPath.find_last_of("/\\");
  while (prevSPos == slashPos+1 && slashPos > 0)
  {
    prevSPos = slashPos;
    slashPos = fullPath.find_last_of("/\\",slashPos-1);
  }

  if (slashPos < fullPath.size())
  {
    // Check if the program is in current working directory
    programPath = fullPath.substr(0, slashPos > 0 ? slashPos : 1);
    iAmInCwd = programPath == getCWD();
  }
  else if (fileExists(program))
  {
    // The program is in current working directory
    programPath = getCWD();
    iAmInCwd = true;
  }
  else
  {
    programPath.clear();
    char* ppath = strdup(getenv("PATH"));
    if (ppath)
    {
      // Search for the program path among the entries
      // in the $PATH environment variable
#if defined(win32) || defined(win64)
      const char* delim = ";";
      std::string p_sep = "\\";
#else
      const char* delim = ":";
      std::string p_sep = "/";
#endif
      for (char* p = strtok(ppath,delim); p; p = strtok(NULL,delim))
        if (fileExists(p + p_sep + program))
        {
          programPath = p;
          break;
        }
      free(ppath);
    }
  }
}


std::string FFaAppInfo::getCWD()
{
  char* cdir = getcwd(NULL,1024);
  std::string CWD(cdir ? cdir : ".");
  free(cdir);
  return CWD;
}


std::string FFaAppInfo::getProgramPath(const std::string& program, bool fnutts)
{
  std::string fullPath(program);
#if defined(win32) || defined(win64)
  // On windows the solver modules are not necessarily found in default $PATH
  // Assume their executables are located in the same directory as this program
#ifdef win64
  // Check first for 32-bit version on 64-bit
  fullPath = programPath + "\\bin32\\" + program;
  if (!fileExists(fullPath))
#endif
  fullPath = programPath + "\\" + program;
  if (!fileExists(fullPath))
    return "";
#else
  fnutts = false;
#endif
  if (fnutts)
    fullPath = "\"" + fullPath + "\""; // Since the pathname may contain spaces
  return fullPath;
}


std::string FFaAppInfo::checkProgramPath(const std::string& program)
{
  std::string fullPath(program);
#if defined(win32) || defined(win64)
  // On windows the solver modules are not necessarily found in default $PATH
  // Assume their executables are located in the same directory as this program
#ifdef win64
  // Check first for 32-bit version on 64-bit
  fullPath = programPath + "\\bin32\\" + program;
  if (fileExists(fullPath))
    return "bin32\\" + program;
#endif
  fullPath = programPath + "\\" + program;
  if (!fileExists(fullPath))
    return "";
#endif
  return program;
}
