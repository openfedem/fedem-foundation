/* SPDX-FileCopyrightText: 2023 SAP SE
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * This file is part of FEDEM - https://openfedem.org
 */
/*!
  \file FFaIO.c
  \brief Functions for direct access of large binary files using low-level IO.
  \details The functions in this file may be used in place of the standard IO
  functions on Windows platforms where binary files larger than 2GB used to be
  problematic. Probably they are not needed any longer, but are retained for
  reference and future testing. Not used at all on Linux, therefore
  no documentation is generated unless doxygen is configured with
  ENABLED_SECTIONS = FULL_DOC.
  \cond FULL_DOC
*/

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#if defined(win32) || defined(win64)
#include <io.h>
#else
#include <fcntl.h>
#include <unistd.h>
#define _open open
#define _close close
#define _write write
#define _read read
#endif

static char*  buffer  = 0;
static size_t bufSize = 0;
static size_t bufPtr  = 0;

#ifdef __cplusplus
extern "C" {
#endif

int _ft_open(const char* name, int flag)
{
  int fp = -2;
  if (buffer)
  {
    fprintf(stderr,"FT_open: Cannot open a new file while buffer is active.\n");
    return fp;
  }

#if defined(win32) || defined(win64)
  fp = _open(name,flag,_S_IREAD|_S_IWRITE);
#else
  fp = _open(name,flag, S_IROTH| S_IWOTH);
#endif
  if (fp < 0)
    perror("FT_open");

  return fp;
}


int _ft_close(int fp)
{
  if (buffer)
  {
    /* Empty the internal buffer */
    if (bufPtr > 0)
      if (_write(fp,buffer,bufPtr) < 0)
      {
	perror("FT_close");
	return -1;
      }
      else
	bufPtr = 0;

    free(buffer);
    buffer = 0;
    bufSize = 0;
  }

  return _close(fp);
}


/*! Re-implementation of fgetc and fgets using the low-level _read function. */

int _ft_getc(int fp)
{
  char byte;

  return _read(fp,&byte,1) < 1 ? -1 : (int)byte;
}


int _ft_ungetc(int c, int fp)
{
  fprintf(stderr," *** _ft_ungetc not implemented.");
  return -1;
}


char* _ft_gets(char* buf, int n, int fp)
{
  register char* p;

  for (p = buf; n > 1; n--)
    if (_read(fp,p,1) < 1)
    {
      /* Read failure */
      if (p == buf)
      {
	/* Nothing at all was read */
	*p = '\0';
	return (char*)0;
      }
      else
	break;
    }
    else if (*p++ == '\n')
      break;

  *p = '\0';
  return buf;
}


/*! Re-implementation of fread and fwrite using the low-level IO functions. */

size_t _ft_read(char* buf, size_t n, size_t m, int fp)
{
  int nBytes;

  nBytes = _read(fp,buf,n*m);
  if (nBytes > 0)
    return (size_t)(nBytes/n);
  else if (nBytes < 0)
    perror("FT_read");
  return 0;
}


size_t _ft_write(const char* buf, size_t n, size_t m, int fp)
{
  int nBytes;

  if (buffer)
  {
    /* Use internal buffer */
    if (bufPtr+n*m <= bufSize)
    {
      memcpy(buffer+bufPtr,buf,n*m);
      bufPtr += n*m;
      return m;
    }

    if (bufPtr > 0)
      if (_write(fp,buffer,bufPtr) < 0)
      {
	perror("FT_wite");
	return 0;
      }
      else
	bufPtr = 0;

    if (n*m <= bufSize)
    {
      memcpy(buffer,buf,n*m);
      bufPtr = n*m;
      return m;
    }
  }

  nBytes = _write(fp,buf,n*m);
  if (nBytes > 0)
    return (size_t)(nBytes/n);
  else if (nBytes < 0)
    perror("FT_write");
  return 0;
}


size_t _ft_setbuf(size_t newSize)
{
  if (buffer && newSize == bufSize)
    return 0;

  if (buffer)
    free(buffer);

  if (newSize > 0)
    buffer = (char*)malloc(newSize);
  else
    buffer = 0;

  bufSize = newSize;
  return newSize;
}

#ifdef __cplusplus
}
#endif

/*! \endcond */
