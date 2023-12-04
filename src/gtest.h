/* SPDX-FileCopyrightText: 2023 SAP SE
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * This file is part of FEDEM - https://openfedem.org
 */
/*!
  \file gtest.h
  \brief Google test wrapper.
  \details This file just includes the gtest.h file from the google test package
  and then redefines the INSTATIATE_TEST_CASE_P macro in case of newer versions.
  We do this to avoid the generated warning on use of the depreciated API.
  Since googletest does not have any versioning information, the way we check
  which version we have is to check if GTEST_INCLUDE_GTEST_GTEST_MATCHERS_H_
  is defined. If so, we have a newer version.
*/

#ifndef FT_GTEST
#define FT_GTEST

#include "gtest/gtest.h"

#ifdef GTEST_INCLUDE_GTEST_GTEST_MATCHERS_H_
//! \cond DO_NOT_DOCUMENT
#undef INSTANTIATE_TEST_CASE_P
#define INSTANTIATE_TEST_CASE_P INSTANTIATE_TEST_SUITE_P
//! \endcond
#endif

#endif
