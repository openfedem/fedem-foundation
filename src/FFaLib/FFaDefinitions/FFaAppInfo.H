// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#ifndef FFA_APPLICATION_INFO_H
#define FFA_APPLICATION_INFO_H

#include <string>


/*!
  \brief Class to store information about the application.
  \details E.g. UI-type (console, GUI), version number, user data, etc.
*/

class FFaAppInfo
{
public:
  //! \brief The constructor gets meta information of this application.
  FFaAppInfo();

  std::string version; //!< Current program version
  std::string user;    //!< User name of running process
  std::string date;    //!< Current date and time

  //! \brief Sets this to be a console application, optionally with GUI too.
  static void openConsole(bool withGUI = false);
  //! \brief Closes the current console window.
  static void closeConsole(bool acknowledge = true);

  //! \brief Returns \e true if this is a console-only application.
  static bool isConsole()   { return consoleFlag == 1; }
  //! \brief Returns \e true if this application has a console windows.
  static bool haveConsole() { return consoleFlag >= 1; }

  //! \brief Initializes the program path.
  static void init(const char* program);
  //! \brief Returns the program path.
  static const std::string& getProgramPath() { return programPath; }
  //! \brief Returns whether this program is in current working dir or not.
  static bool isInCwd() { return iAmInCwd; }
  //! \brief Returns the current working directory of the program
  static std::string getCWD();
  //! \brief Returns full path of a program, or blank if not existing.
  static std::string getProgramPath(const std::string& program,
                                    bool fnutts = true);
  //! \brief Returns path of a program, or blank if not existing.
  static std::string checkProgramPath(const std::string& program);

private:
  static char consoleFlag; //!< 0: no console, 1: only console, 2: console+GUI
  static int runInConsole; //!< If positive, the executable is run from console
  static bool iAmInCwd; //!< If \e true, the executable is in current work dir
  static std::string programPath; //!< Absolute path of this executable
};

#endif
