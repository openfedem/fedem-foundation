// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

/*!
  \file FFaParse.C
  \brief Utilities for input file parsing.
*/

#include "FFaLib/FFaString/FFaParse.H"
#include <cstring>
#include <cstdio>
#include <cctype>


bool FaParse::parseFMFASCII(char* keyWord, std::istream& s,
			    std::stringstream& statement,
			    const char start, const char stop)
{
  const char commentString1 = '!';
  const char commentString2 = '#';

  char c;

  // Skip white space and comments
  while (s.get(c) && !isalpha(c))
    if (c == commentString1 || c == commentString2)
      while (s.get(c) && c != '\n');

  s.putback(c);

  // Read characters to build keyword
  for (int i = 0; i < BUFSIZ && s.get(c); i++)
    if (isalpha(c) || c == '_' || (i > 0 && isdigit(c)))
      keyWord[i] = toupper(c);
    else
    {
      keyWord[i] = '\0';
      break;
    }

  s.putback(c);

  // Find if the word has a valid statement
  // Read until the start-character or a possible new word is reached

  while (true) // Loop ends in one of the test branches, not while somthing

    if (!s.get(c))
      return false; // EOF

    else if (c == start) // The start character is read
    {
      bool insideFnutts = false;
      while (s.get(c) && (c != stop || insideFnutts))
      {
	// Do a check for "-characters in the statement.
	// The stop sign is allowed inside a pair of "-s.
	if (c == '\"') insideFnutts = !insideFnutts;
	statement << c;
      }
      statement << std::ends;
      return c == stop;
    }

    else if (!isspace(c))
    {
      // The char is not the start character and not a space
      if (c == commentString1 || c == commentString2)
	// Skip comment and resume with do loop
	while (s.get(c) && c != '\n');
      else if (c == stop)
      {
	// The stop character is read before the start character
	// Return an empty statement, but no error
	statement << std::ends;
	return true;
      }
      else
      {
	// A possible new word came before the pivot
	s.putback(c); // putback possible start of new word
	return false; // The recognized word has no sense
      }
    }
}


int FaParse::findIndex(const char** vocabulary, const char* s)
{
  // Test if the string was a keyword, if not return 0
  for (int i = 0; vocabulary[i]; i++)
    if (strcmp(vocabulary[i],s) == 0)
      return i+1;

  return 0;
}


std::string FaParse::extractDescription(std::istream& is,
                                        char startChar, char stopChar)
{
  char c, tmpDescr[BUFSIZ];
  while (is.get(c) && (c != startChar));
  is.get(tmpDescr,BUFSIZ-1,stopChar);

  return std::string(tmpDescr);
}


bool FaParse::skipToWordOrNum(std::istream& s, const char commentChar)
{
  // Commment char must be a non alphanum char.
  char c = 0;
  while (s.get(c) && !(isalnum(c) || c == '-'))
    if (c == commentChar)
      while (s.get(c) && c != 10 && c != 13);

  if (s &&  c != 10 && c != 13)
    s.putback(c);

  return isalpha(c) ? true : false;
}


void FaParse::nextLine(std::istream& s, const char commentChar)
{
  char c = 0;
  while (s.get(c) && c != 10 && c != 13);

  if (commentChar && !s.eof() && s.get(c))
  {
    if (c == commentChar)
      FaParse::nextLine(s,commentChar);
    else if (s)
      s.putback(c);
  }
}


bool FaParse::skipToWord(std::istream& s, const char commentChar)
{
  // Commment char must be a non alphanum char.
  char c = 0;
  while (s.get(c) && !isalpha(c))
    if (c == commentChar)
      while (s.get(c) && c != 10 && c != 13);

  if (s && c != 10 && c != 13)
    s.putback(c);

  return isalpha(c) ? true : false;
}


/*!
  All kinds of text is considered to be comments,
  except any text within "" if \a acceptString is true.
*/

bool FaParse::skipWhiteSpaceAndComments(std::istream& s, bool acceptString)
{
  char c;
  while (!s.eof())
  {
    // Skip whitespace
    while (s.get(c) && isspace(c));
    s.unget();

    // Check if we have numerical data
    if (isdigit(c) || c == '.' || c == '-' || c == '+')
      return s ? true : false;

    // Check if we have string data
    if (acceptString && (c == '\"' || isalpha(c)))
      return s ? true : false;

    // This is a comment line, ignore it and continue
    char text[1024];
    if (!s.getline(text,1024))
      return false;
  }

  return false;
}


bool FaParse::getKeyword(std::istream& s, std::string& keyWord)
{
  char c;
  while (!s.eof())
  {
    // Skip whitespace
    while (s.get(c) && isspace(c));

    // Check if we have a keyword next
    if (isalpha(c))
    {
      keyWord = toupper(c);
      while (s.get(c) && (isalnum(c) || c == '-' || c == '_'))
        keyWord += toupper(c);
      s.unget();
      return s ? true : false;
    }

    // No keyword, check if we have numerical data next
    if (isdigit(c) || c == '.' || c == '-' || c == '+')
    {
      keyWord = "";
      s.unget();
      return s ? true : false;
    }

    // This is a comment line, ignore it and continue
    char text[1024];
    if (!s.getline(text,1024))
      return false;
  }

  return false;
}
