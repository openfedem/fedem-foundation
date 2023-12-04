// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#ifndef FFA_VERSION_NUMBER_H
#define FFA_VERSION_NUMBER_H

#include <string>


/*!
  \brief A class to keep Fedem version numbers.

  \details Used to enable automatic conversion of older model files, etc.
  Version numbers like R2.5m3-i4 are stored as d1=2 d2=5 d3=3 internal=4.
*/

class FFaVersionNumber
{
public:
  //! \brief Default constructor.
  FFaVersionNumber(int n1 = 0, int n2 = 0, int n3 = 0, int n4 = -1);
  //! \brief Constructor initializing the version number from a string.
  FFaVersionNumber(const std::string& s) { this->parseLine(s); }

  //! \brief Sets the version number.
  void setVersion(int n1, int n2 = 0, int n3 = 0, int n4 = -1);
  //! \brief Parses a version number from the provided string.
  void parseLine(const std::string& line, char skipUntil = '\0');

  //! \brief Sets the major-, minor- or patch version or the build number.
  void set(int i, int n);
  //! \brief returns the major-, minor- or patch version or the build number.
  int get(int i) const;

  //! \brief Returns the string representation of this version number.
  const std::string& getString() const { return version; }
  //! \brief Returns a converted string representation of this version number.
  std::string getInterpretedString() const;

  //! \brief Greater than operator.
  bool operator> (const FFaVersionNumber& v) const;
  //! \brief Equality operator.
  bool operator==(const FFaVersionNumber& v) const;
  //! \brief Greater or equal to operator.
  bool operator>=(const FFaVersionNumber& v) const;
  //! \brief Less than operator.
  bool operator< (const FFaVersionNumber& v) const;
  //! \brief Less than or equal to operator.
  bool operator<=(const FFaVersionNumber& v) const;

private:
  int         d1; //!< Major version number
  int         d2; //!< Minor version number
  int         d3; //!< Patch version number
  int         build;   //!< Internal build number
  std::string version; //!< String representation of the version number
};

#endif
