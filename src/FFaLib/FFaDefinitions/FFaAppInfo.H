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
  \brief Namespace with functions returning information about the application.
  \details E.g. UI-type (console, GUI), version number, user data, etc.
*/

namespace FFaAppInfo
{
  //! \brief Sets this to be a console application, optionally with GUI too.
  void openConsole(bool withGUI = false);
  //! \brief Closes the current console window.
  void closeConsole(bool acknowledge = true);
  //! \brief Returns \e true if this is a console-only application.
  bool isConsole();
  //! \brief Returns \e true if this application has a console windows.
  bool haveConsole();

  //! \brief Returns the current program version.
  std::string getVersion();
  //! \brief Returns the user id of the executing process.
  std::string getUser();
  //! \brief Returns the current date and time.
  std::string getDate();

  //! \brief Initializes the program path.
  void init(const char* program);
  //! \brief Returns full path of a program, or blank if not existing.
  std::string getProgramPath(const std::string& program, bool fnutts = true);
  //! \brief Returns path of a program, or blank if not existing.
  std::string checkProgramPath(const std::string& program);
  //! \brief Returns the program path.
  const std::string& getProgramPath();

  //! \brief Returns the current working directory of the program
  std::string getCWD();
  //! \brief Returns whether this program is in current working dir or not.
  bool isInCwd();
}

#endif
