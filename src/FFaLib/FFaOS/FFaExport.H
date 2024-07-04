/* SPDX-FileCopyrightText: 2023 SAP SE
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * This file is part of FEDEM - https://openfedem.org
 */
/*!
  \file FFaExport.H
  \brief Macros to aid export of symbols in shared object libraries.
*/

#ifndef FFA_EXPORT_H
#define FFA_EXPORT_H

#if defined(win32) || defined(win64)
#include <windows.h>
#endif

//! \brief Macro for C++ binding to function in shared object librararies
#if defined(win32) || defined(win64)
#ifdef __cplusplus
#define DLLexport(ret) extern "C" __declspec(dllexport) ret
#else
#define DLLexport(ret) __declspec(dllexport) ret
#endif
#else
#ifdef __cplusplus
#define DLLexport(ret) extern "C" ret
#else
#define DLLexport(ret) ret
#endif
#endif

#endif
