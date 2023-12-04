// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

/*!
  \file FFaTag.H
  \brief Utilities for reading and writing of file tags.
*/

#ifndef FFA_TAG_H
#define FFA_TAG_H

#include <string>
#include <cstdio>

#include "FFaLib/FFaOS/FFaIO.H"

//! \brief Default length of a file tag.
#define LEN_TAG 30


namespace FFaTag //! Utilities for reading and writing of file tags.
{
#ifdef FT_USE_LOWLEVEL_IO
  //! \brief Reads the file tag and checksum from the provided file.
  //! \sa FFaTag_read(FFa_stream&,std::string&,unsigned int&,int)
  int read(FT_FILE fd, std::string& tag, unsigned int& cs,
           int tagLength = LEN_TAG);

  //! \brief Writes the file tag and checksum to the provided file.
  //! \sa FFaTag_write(FFa_stream&,const char*,int,unsigned int,int)
  int write(FT_FILE fd, const char* tag, int nchar, unsigned int cs,
            int tagLength = LEN_TAG);
#endif

  //! \brief Reads the file tag and checksum from the provided file.
  //! \sa FFaTag_read(FFa_stream&,std::string&,unsigned int&,int)
  int read(FILE* fd, std::string& tag, unsigned int& cs,
           int tagLength = LEN_TAG);

  //! \brief Writes the file tag and checksum to the provided file.
  //! \sa FFaTag_write(FFa_stream&,const char*,int,unsigned int,int)
  int write(FILE* fd, const char* tag, int nchar, unsigned int cs,
            int tagLength = LEN_TAG);

  //! \brief Returns current system endian.
  int endian();
}

#endif
