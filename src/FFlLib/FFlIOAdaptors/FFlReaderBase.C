// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#include <cstring>
#include <fstream>

#include "FFlReaderBase.H"


/*!
  Searches for \a keyWord in the given text file named \a fileName,
  among the first \a maxLines lines in the file. Returns the line
  number of the first line containing \a keyWord or 0 if no match.
*/

int FFlReaderBase::searchKeyword(const char* fileName,
				 const char* keyWord,
				 int maxLines)
{
  const char* keyWords[2] = { keyWord, 0 };
  return searchKeyword(fileName,keyWords,maxLines);
}


int FFlReaderBase::searchKeyword(const char* fileName,
				 const char** keyWord,
				 int maxLines)
{
  std::ifstream fileStream(fileName);
  if (!fileStream) return -1;

  char line[256];
  for (int lCount = 1; !fileStream.eof() && lCount <= maxLines; lCount++)
  {
    fileStream.getline(line,255);
    for (const char** kw = keyWord; *kw; kw++)
      if (strncmp(line,*kw,strlen(*kw)) == 0)
	return lCount;
  }

  return 0;
}
