// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

/*!
  \file FFaParse.H
  \brief Utilities for input file parsing.
*/

#ifndef FFA_PARSE_H
#define FFA_PARSE_H

#include <sstream>
#include <string>
#include <vector>


namespace FaParse //! Utilities for input file parsing
{
  //! \brief Reads a model file record from an input stream.
  //! \param[out] keyWord The keyword of the record that was read
  //! \param s The input stream to read from
  //! \param[out] statement String stream containing unparsed data of the record
  //! \param[in] start Separator between the keyword and data section
  //! \param[in] stop End-of-record character
  bool parseFMFASCII(char* keyWord, std::istream& s,
                     std::stringstream& statement,
                     const char start, const char stop);

  //! \brief Searches for the string \a s in the list of strings \a vocabulary.
  int findIndex(const char** vocabulary, const char* s);

  //! \brief Extracts a text string between the given start and stop characters.
  std::string extractDescription(std::istream& is,
                                 const char startChar = '"',
                                 const char stopChar = '"');

  //! \brief Skips until next word or number.
  bool skipToWordOrNum(std::istream& s, const char commentChar);
  //! \brief Skips until next word (no number).
  bool skipToWord(std::istream& s, const char commentChar);
  //! \brief Skips the rest of current line.
  void nextLine(std::istream& s, const char commentChar);
  //! \brief Skips whitespaces and comments.
  bool skipWhiteSpaceAndComments(std::istream& s, bool acceptString = false);
  //! \brief Skips whitespaces and returns the next (non-numerical) keyword.
  bool getKeyword(std::istream& s, std::string& keyWord);
}

#endif
