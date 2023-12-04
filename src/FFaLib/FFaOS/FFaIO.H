/* SPDX-FileCopyrightText: 2023 SAP SE
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * This file is part of FEDEM - https://openfedem.org
 */
/*!
  \file FFaIO.H
  \brief Functions and data type for direct access of large binary files.
  \details The symbols in this file are provided to easily enable switching
  between standard and low-level file IO on windows platforms,
  through the FT_USE_LOWLEVEL_IO preprocessor macro
*/

#ifndef FFA_IO_H
#define FFA_IO_H

# if defined(irix)
#   define Fopen    fopen
#   define Fclose   fclose
#   define Ftell    ftell64
#   define Fseek    fseek64
#   define long_int long long int
# elif defined(hpux)
#   define Fopen    fopen64
#   define Fclose   fclose
#   define Ftell    ftello64
#   define Fseek    fseeko64
#   define long_int off64_t
# elif defined(win64)
#   define Fopen    fopen
#   define Fclose   fclose
#   define Ftell    _ftelli64
#   define Fseek    _fseeki64
#   define long_int __int64
# else
   /*! \brief Opens a binary file. */
#   define Fopen    fopen
   /*! \brief Closes a binary file. */
#   define Fclose   fclose
   /*! \brief Returns current file position. */
#   define Ftell    ftell
   /*! \brief Sets the file position. */
#   define Fseek    fseek
   /*! \brief 64-bit integer type. */
#   define long_int long int
# endif

/* Special treatment required for Windows, use of low-level IO-functions */

# ifdef FT_USE_LOWLEVEL_IO

#   include <io.h>
#   include <fcntl.h>
#   include <sys/stat.h>

#   ifdef __cplusplus
extern "C" {
#   endif
  int    _ft_open(const char* name, int flag);
  int    _ft_close(int fp);
  int    _ft_getc(int fp);
  int    _ft_ungetc(int c, int fp);
  char*  _ft_gets(char* buf, int n, int fp);
  size_t _ft_read(char* buf, size_t n, size_t m, int fp);
  size_t _ft_write(const char* buf, size_t n, size_t m, int fp);
  size_t _ft_setbuf(size_t bufsiz);
#   ifdef __cplusplus
}
#   endif

#   define FT_rb               _O_BINARY | _O_RDONLY
#   define FT_wb               _O_BINARY | _O_WRONLY | _O_CREAT | _O_TRUNC
#   define FT_ab               _O_BINARY | _O_WRONLY | _O_APPEND
#   define FT_rbp              _O_BINARY | _O_RDWR
#   define FT_wbp              _O_BINARY | _O_RDWR | _O_CREAT | _O_TRUNC
#   define FT_tmp              _O_BINARY | _O_RDWR | _O_CREAT | _O_TEMPORARY
#   define FT_open             _ft_open
#   define FT_close            _ft_close
#   define FT_tell             _telli64
#   define FT_seek             _lseeki64
#   define FT_read(buf,n,m,f)  _ft_read((char*)buf,n,m,f)
#   define FT_write(buf,n,m,f) _ft_write((char*)buf,n,m,f)
#   define FT_setbuf(n)        _ft_setbuf(n)
#   define FT_flush            _commit
#   define FT_getc             _ft_getc
#   define FT_ungetc           _ft_ungetc
#   define FT_gets             _ft_gets
#   define FT_eof              _eof
#   define FT_int              __int64
#   define FT_FILE             int

# else

#   include <stdio.h>

   /*! \brief File open mode for reading. */
#   define FT_rb               "rb"
   /*! \brief File open mode for writing. */
#   define FT_wb               "wb"
   /*! \brief File open mode for appending. */
#   define FT_ab               "ab"
   /*! \brief File open mode for reading and writing. */
#   define FT_rbp              "rb+"
   /*! \brief File open mode for reading and writing, truncate existing file. */
#   define FT_wbp              "wb+"
   /*! \brief File open mode for temporary file. */
#   define FT_tmp              "wb+"
   /*! \brief Opens a binary file. */
#   define FT_open             Fopen
   /*! \brief Closes a binary file. */
#   define FT_close            Fclose
   /*! \brief Returns current file position. */
#   define FT_tell             Ftell
   /*! \brief Sets the file position. */
#   define FT_seek             Fseek
   /*! \brief Reads an array from file. */
#   define FT_read(buf,n,m,f)  fread((void*)buf,n,m,f)
   /*! \brief Writes an array to file. */
#   define FT_write(buf,n,m,f) fwrite((void*)buf,n,m,f)
   /*! \brief Dummy doing nothing. */
#   define FT_setbuf(n)        false
   /*! \brief Flushes the file buffer(s) to disk. */
#   define FT_flush            fflush
   /*! \brief Reads one character from file. */
#   define FT_getc             fgetc
   /*! \brief Puts back one character to the file stream. */
#   define FT_ungetc           ungetc
   /*! \brief Reads a string from file. */
#   define FT_gets             fgets
   /*! \brief Checks for end-of-file. */
#   define FT_eof              feof
   /*! \brief 64-bit integer type. */
#   define FT_int              long_int
   /*! \brief File pointer. */
#   define FT_FILE             FILE*

# endif

#endif
