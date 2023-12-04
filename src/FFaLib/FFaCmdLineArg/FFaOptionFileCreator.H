// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

/*!
  \file FFaOptionFileCreator.H
  \brief Command-line option file creator.
*/

#ifndef FFA_OPTION_FILE_CREATOR_H
#define FFA_OPTION_FILE_CREATOR_H

#include <vector>
#include <string>

class FaVec3;


/*!
  \brief Class for writing command-line option files for the solver modules.
*/

class FFaOptionFileCreator
{
public:
  //! \brief Default constructor.
  FFaOptionFileCreator(const std::string& fname = "") : myFilename(fname) {}

  //! \brief Adds a bool-valued command-line option.
  void add(const std::string& optionName, bool val);
  //! \brief Adds an integer-valued command-line option.
  void add(const std::string& optionName, int val);
  //! \brief Adds a real-valued (double) command-line option.
  void add(const std::string& optionName, double val);
  //! \brief Adds a two-valued real (double) command-line option.
  void add(const std::string& optionName, const std::pair<double,double>& val);
  //! \brief Adds a FaVec3-valued command-line option.
  void add(const std::string& optionName, const FaVec3& val);
  //! \brief Adds a vector-valued (double) command-line option.
  void add(const std::string& optionName, const std::vector<double>& val);
  //! \brief Adds a string-valued command-line option.
  void add(const std::string& optionName, const std::string& val,
           bool addQuotes = true);

  //! \brief Adds a comment line.
  void addComment(const std::string& comment, bool lineInFront = false);

  //! \brief Returns all options with values as a vector of strings.
  std::vector<std::string> getOptVector() const;

  //! \brief Writes the command-line options to file.
  bool writeOptFile() const;

private:
  //! Convenience type definition
  typedef std::pair<std::string,std::string> Option;

  std::vector<Option> myOptions;  //!< List of command-line options with value
  std::string         myFilename; //!< Name of the file to be written
};

#endif
