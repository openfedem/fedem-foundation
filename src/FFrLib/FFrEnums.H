// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

/*!
  \file FFrEnums.H
  \brief Enums that may be used by several FFrLib classes.
*/

#ifndef FFR_ENUMS_H
#define FFR_ENUMS_H

//! \brief Enum for different states while parsing a result database header.
enum FFrStatus
{
  FAILED = -1,      //!< Parsing failure, abort the execution
  DONE = 0,         //!< Parsing file header successfully completed
  LABEL_SEARCH,     //!< Search for the next label
  LABEL_IGNORE,     //!< Comment line detected, ignore the rest of the line
  LABEL_READ,       //!< New label detected, read it
  LABEL_VALID,      //!< A valid label was found
  LABEL_ERROR,      //!< An invalid label was detected
  FOUND_HEADING,    //!< A metadata line was detected
  FOUND_VARIABLES,  //!< The VARIABLES: section was detected
  FOUND_DATABLOCKS, //!< The DATABLOCKS: section was detected
  FOUND_DATA        //!< The start of binary content has been reached
};

#endif
