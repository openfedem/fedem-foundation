// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

/*!
  \file FFaTag.C
  \brief Utilities for reading and writing of file tags.
*/

#include <cstring>

#ifdef FFA_DEBUG
#include <cerrno>
#include <iostream>
#endif

#include "FFaLib/FFaOS/FFaTag.H"

static short EndianField = 0x1234; //!< 16-bit value used to determine endian
static unsigned char EF1 = 0x12;   //!< Little endian value
static unsigned char EF2 = 0x34;   //!< Big endian value


/*!
  \brief Interface class for stream-based reading/writing of file tags.
  \details This is a base class used in FFaTag_read() and FFaTag_write()
  such that those implementations can be made independent on whether we
  are using the low-level IO-functions or not.
*/

class FFa_stream
{
protected:
  //! \brief Default constructor.
  FFa_stream() {}

public:
  //! \brief Empty destructor.
  virtual ~FFa_stream() {}
  //! \brief Returns the next character from the stream.
  virtual int getC() = 0;
  //! \brief Reads the specified number of data items from the stream.
  virtual int read(void*, size_t, size_t) = 0;
  //! \brief Writes the specified number of data items to the stream.
  virtual int write(void*, size_t, size_t) = 0;
};


#ifdef FT_USE_LOWLEVEL_IO

/*!
  \brief Class for file stream based on low-level IO.
*/

class FT_stream : public FFa_stream
{
  FT_FILE fd; //!< File descriptor

public:
  //! \brief The constructor sets the file descriptor.
  FT_stream(FT_FILE _fd) { fd = _fd; }
  //! \brief Empty destructor.
  virtual ~FT_stream() {}
  //! \brief Returns the next character from the file.
  virtual int getC() { return FT_getc(fd); }

  //! \brief Reads the specified number of items from the file.
  //! \param[out] buf Memory buffer containing the bytes that was read
  //! \param[in] m Size (in bytes) of each data item
  //! \param[in] n Number of data items to read
  //! \return Number of data items read
  virtual int read(void* buf, size_t m, size_t n)
  {
    return FT_read(buf,m,n,fd);
  }

  //! \brief Writes the specified number of data items to the file.
  //! \param[in] buf Memory buffer containing the bytes to be written
  //! \param[in] m Size (in bytes) of each data item
  //! \param[in] n Number of data items to write
  //! \return Number of data items written
  virtual int write(void* buf, size_t m, size_t n)
  {
    return FT_write(buf,m,n,fd);
  }
};

#endif


/*!
  \brief Class for file stream based on standard IO.
*/

class F_stream : public FFa_stream
{
  FILE* fd; //!< File pointer

public:
  //! \brief The constructor sets the file pointer.
  F_stream(FILE* _fd) { fd = _fd; }
  //! \brief Empty destructor.
  virtual ~F_stream() {}

  //! \brief Returns the next character from the file.
  virtual int getC() { return fgetc(fd); }

  //! \brief Reads the specified number of bytes from the file.
  virtual int read(void* buf, size_t m, size_t n)
  {
    return fread(buf,m,n,fd);
  }

  //! \brief Writes the specified number of bytes to the file.
  virtual int write(void* buf, size_t m, size_t n)
  {
    return fwrite(buf,m,n,fd);
  }
};


#ifdef FFA_DEBUG
//! \brief Prints a system-generated error to std::cerr.
static int FFaTag_error (const char* msg, int status)
{
  std::cerr <<"FFaTag: "<< msg;
  if (errno) std::cerr <<": "<< strerror(errno);
  std::cerr << std::endl;
  errno = 0;
  return status;
}
#else
//! \brief Dummy function for silent runs.
static int FFaTag_error (const char*, int status) { return status; }
#endif


/*!
  \brief Checks the current system endian.
*/

static int FFaTag_checkEndian (short myEndian)
{
  const int FFA_BIG_ENDIAN    = 1;
  const int FFA_LITTLE_ENDIAN = 2;

  unsigned char* p = (unsigned char*)(&myEndian); // 16-bit integer

  if (*p == EF1)
    return FFA_BIG_ENDIAN;
  else if (*p == EF2)
    return FFA_LITTLE_ENDIAN;

  return FFaTag_error("Invalid endian field",-3);
}


int FFaTag::endian ()
{
  return FFaTag_checkEndian(EndianField);
}


/*!
  \brief Returns the current system endian.
*/

extern "C" int FFa_endian ()
{
  return FFaTag_checkEndian(EndianField);
}


/*!
  \brief Reads the file tag and checksum from the provided file stream.
  \param fs File stream to read from
  \param[out] tag File tag
  \param[out] cs Checksum value
  \param[in] tagLength Number of characters in the tag string on file
  \return  0 : This is an ASCII file
  \return  1 : This is a big endian binary file
  \return  2 : This is a little endian binary file
  \return -1 : Wrong file start, first read character should be a #
  \return -2 : Error reading file tag
  \return -3 : Invalid or error reading endian field
  \return -4 : Error reading checksum field
*/

static int FFaTag_read (FFa_stream& fs, std::string& tag,
                        unsigned int& cs, int tagLength)
{
  int c = fs.getC();
  if (c == '#')
    tag += '#';
  else
    return FFaTag_error("The first character should be a #",-1);

  // Read the file tag, character by character
  bool isBinary = false;
  for (int i = 1; i < tagLength; i++)
  {
    c = fs.getC();
    if (c < 0)
      return FFaTag_error("Error reading file tag",-2);
    else if (!isBinary && (c == '\n' || c == '\r'))
    {
      // If newline is encountered within the first 'tagLength' characters
      // we assume this is an ASCII file
      return 0;
    }
    else if (c < 32 || c > 126)
      isBinary = true;
    tag += c;
  }

  // Check endian field
  short myEndian;
  if (fs.read(&myEndian,sizeof(myEndian),1) < 1)
    return FFaTag_error("Error reading endian field",-3);

  int endianStat = FFaTag_checkEndian(myEndian);

  // Read checksum field
  int readStat = fs.read(&cs,sizeof(unsigned int),1);
  if (readStat < 1)
    return FFaTag_error("Error reading checksum field",-4);

  if (endianStat == FFaTag::endian())
    // same endian on file as on machine
    readStat = fs.read(&cs,sizeof(unsigned int),1);
  else
  {
    // swap bytes
    char b[4];
    readStat = fs.read(b,sizeof(unsigned int),1);
    char* p = (char*)(&cs);
    p[0] = b[3];
    p[1] = b[2];
    p[2] = b[1];
    p[3] = b[0];
  }

  if (readStat > 0)
    return endianStat;
  else
    return FFaTag_error("Error reading checksum field",-4);
}


/*!
  \brief Writes the file tag and checksum to the provided file stream.
  \param fs File stream to write to
  \param[in] tag File tag
  \param[in] nchar Number of characters in the provided tag string
  \param[in] cs Checksum value
  \param[in] tagLength Number of characters in the tag string on file
  \return &nbsp; 0 : OK
  \return -1 : Error writing file tag
  \return -2 : Error writing endian field
  \return -3 : Error writing checksum field
*/

int FFaTag_write (FFa_stream& fs, const char* tag, int nchar,
                  unsigned int cs, int tagLength)
{
  // Tag should be exactly 'tagLength' characters long.
  // Fill in with trailing blanks if necessary.

  char* fullTag = new char[tagLength];
  int myLength = nchar;
  if (myLength > tagLength) myLength = tagLength;
  strncpy(fullTag,tag,myLength);
  if (myLength < tagLength) memset(&fullTag[myLength],' ',tagLength-myLength);

  // Write file tag
  int nWrote = fs.write(fullTag,sizeof(char),tagLength);
  delete[] fullTag;
  if (nWrote < tagLength)
    return FFaTag_error("Error writing file tag",nWrote-tagLength);

  // Write endian field
  if (fs.write(&EndianField,sizeof(EndianField),1) < 1)
    return FFaTag_error("Error writing endian field",-2);

  // Write checksum field (should be 8 bytes, first 4 are zero)
  unsigned int chksum[2] = { 0, cs };
  if (fs.write(chksum,sizeof(unsigned int),2) < 2)
    return FFaTag_error("Error writing checksum field",-3);

  return 0;
}


#ifdef FT_USE_LOWLEVEL_IO

int FFaTag::read (FT_FILE fd, std::string& tag, unsigned int& cs,
                  int tagLength)
{
  FT_stream fs(fd);
  return FFaTag_read(fs,tag,cs,tagLength);
}


int FFaTag::write (FT_FILE fd, const char* tag, int nchar, unsigned int cs,
                   int tagLength)
{
  FT_stream fs(fd);
  return FFaTag_write(fs,tag,nchar,cs,tagLength);
}

#endif


int FFaTag::read (FILE* fd, std::string& tag, unsigned int& cs,
                  int tagLength)
{
  F_stream fs(fd);
  return FFaTag_read(fs,tag,cs,tagLength);
}


int FFaTag::write (FILE* fd, const char* tag, int nchar, unsigned int cs,
                   int tagLength)
{
  F_stream fs(fd);
  return FFaTag_write(fs,tag,nchar,cs,tagLength);
}


/*!
  \brief Reads the file tag, endian field and checksum from a specified file.
  \param fd File descriptor/pointer to the investigated file
  \param[out] tag File tag
  \param[in] nchar Max number of characters in the tag string
  \param[out] cs Checksum value
  \return  0 : This is an ASCII file
  \return  1 : This is a big endian binary file
  \return  2 : This is a little endian binary file
  \return -1 : Wrong file start, first read character should be a #
  \return -2 : Error reading file tag
  \return -3 : Invalid or error reading endian field
  \return -4 : Error reading checksum field
*/

extern "C" int FFa_readTag (FT_FILE fd, char* tag, int nchar, unsigned int* cs)
{
  std::string myTag;
  int status = FFaTag::read(fd,myTag,*cs,LEN_TAG);
  if (status < 0) return status;

  strncpy(tag,myTag.c_str(),nchar);
  if (LEN_TAG < nchar-1) tag[LEN_TAG] = '\0';
  return status;
}


/*!
  \brief Writes the file tag, endian field and checksum to a specified file.
  \param fd File descriptor/pointer to the written file
  \param[in] tag File tag
  \param[in] nchar Number of characters in the tag string
  \param[in] cs Checksum value
  \return &nbsp; 0 : OK
  \return -1 : Error writing file tag
  \return -2 : Error writing endian field
  \return -3 : Error writing checksum field
*/

extern "C" int FFa_writeTag (FT_FILE fd, const char* tag, int nchar,
                             unsigned int cs)
{
  return FFaTag::write(fd,tag,nchar,cs,LEN_TAG);
}
