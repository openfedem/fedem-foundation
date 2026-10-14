// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

/*!
  \file FFrReadOpInit.H
  \brief Initialization of the read operations.
*/

#ifndef FFR_READ_OP_INIT_H
#define FFR_READ_OP_INIT_H


namespace FFr //! Utilities for initializing the read operations
{
  //! \brief Initializes the read operation factory.
  void initReadOps();
  //! \brief Releases the read operation factory.
  void clearReadOps();
}

#endif
