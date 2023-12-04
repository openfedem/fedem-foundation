// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

/*!
  \file FiASCFile.C
  \brief External device function based on multi-column ASCII file.
*/

#include <cctype>
#include <cstdlib>
#include <cstring>
#include <iostream>

#include "FiDeviceFunctions/FiASCFile.H"
#include "FFaLib/FFaDefinitions/FFaAppInfo.H"

#if _MSC_VER > 1300
#define strncasecmp _strnicmp
#endif


size_t FiASCFile::bufferSize = 0;


FiASCFile::FiASCFile(const char* fname, int nchan) : FiDeviceFunctionBase(fname)
{
  myNumChannels = nchan;
  myChannel = 0;
  outputFormat = 1;
  vit0 = vit1 = myValues.end();
  isCSVt = false;
#if FI_DEBUG > 1
  std::cout <<"FiASCFile: "<< fname <<" myNumChannels="<< myNumChannels
            << std::endl;
#endif
}


char* FiASCFile::readLine(FT_FILE fd, bool commentsOnly)
{
  static std::string lbuf;
  if (lbuf.empty()) lbuf.resize(BUFSIZ);

  // Lambda-function checking for end-of-line character
  auto isEOL = [](int c) -> bool { return c == '\n' || c == '\r' || c == EOF; };

  int c = 0;
  size_t i = 0;
  while (i < lbuf.size() && !FT_eof(fd))
    if (isEOL(c = FT_getc(fd)))
    {
      // eat the '\n' for DOS-formatted files
      if (c == '\r' && (c = FT_getc(fd)) != '\n')
        if (FT_ungetc(c,fd) < 0) return NULL;
      break;
    }
    else if (i > 0)
      lbuf[i++] = c;
    else if (!isspace(c))
    {
      if (commentsOnly && (isdigit(c) || c == '.' || c == '-'))
      {
        // Put back first character of non-comment line
        FT_ungetc(c,fd);
        return NULL;
      }
      lbuf[i++] = c;
    }

  if (i == 0) return NULL; // blank line

  if (i == lbuf.size()) // Need to enlarge the buffer
    for (c = FT_getc(fd); !FT_eof(fd) && !isEOL(c); c = FT_getc(fd), i++)
      lbuf.push_back(c);

  // eat the '\n' for DOS-formatted files
  if (c == '\r' && (c = FT_getc(fd)) != '\n')
    if (FT_ungetc(c,fd) < 0) return NULL;

  lbuf[i] = '\0';
  return const_cast<char*>(lbuf.c_str());
}


int FiASCFile::readChannel(int channel)
{
  if (myNumChannels == 1)
    return 0; // single-channel file, always in core
  else if (channel == myChannel)
    return 0; // the requested channel is the one already in core
  else if (myNumChannels < 1)
    return -1;
  else if (channel < 1 || channel > myNumChannels)
  {
#ifdef FI_DEBUG
    std::cerr <<" *** Error: Invalid channel index "<< channel
              <<" for ASCII-file "<< myDatasetDevice << std::endl;
#endif
    return -2;
  }

  ValuesIter it = myValues.begin();
  if ((int)it->second.size() == myNumChannels)
    return channel-1; // all channels are in core, return channel index
  else if (!this->isReadOnly())
    return -3; // the file has been closed, cannot load another channel

  // A different channel than the one in core is requested, need to reread
  bool okRead = true;
  double prev = 1.0e99;
  FT_seek(myFile,(FT_int)0,SEEK_SET);

  for (int currLine = 1; !FT_eof(myFile) && it != myValues.end(); currLine++)
  {
    char* c = FiASCFile::readLine(myFile);
    if (!c) continue;

    // Find the values on this line, only store value no. channel+1
    int valCount = 0;
    bool searchMore = true;
    double tmpVal = 0.0;

    while (searchMore && *c != '\0' && *c != '\n' && *c != '\r')
    {
      char* end = c;
      tmpVal = strtod(c,&end);

      if (end != c)
      {
        c = end;
        if (++valCount == 1)
        {
          if (isCSVt)
            tmpVal *= 1.0e-6;

          // Don't increment the myValues iterator if repeated first value
          if (tmpVal > prev) ++it;
          // Check first value of this line, which now should equal it->first
          if (it->first == tmpVal)
            prev = tmpVal;
          else
          {
            // Should normally never get here (logic error if we do...)
            std::cerr <<" *** Error: Internal error while reading ASCII-file "
                      << myDatasetDevice <<" (line "<< currLine <<")"
                      <<"\n     "<< it->first <<" != "<< tmpVal
                      << std::endl;
            searchMore = okRead = false;
          }
        }
        else if (valCount == channel+1)
        {
          // Get requested channel value of current line
          it->second.front() = tmpVal;
          searchMore = false;
        }
      }
      else
	switch (*c)
	  {
	  case ',':
	    if (it == myValues.begin() && !valCount)
	    {
	      searchMore = false;
	      break;
	    }
	  case ' ':
	  case '\t':
	    c++;
	    break;
	  case '#':
	    searchMore = false;
	    break;
	  case '_':
	    if (isCSVt)
	    {
	      searchMore = false;
	      break;
	    }
	  default:
	    if (it == myValues.begin() && strchr(c,','))
	    {
	      searchMore = false;
	      break;
	    }
#ifndef FI_DEBUG
	    if (okRead)
#endif
              std::cerr <<" *** Error: Invalid format on line "<< currLine
                        <<" in ASCII-file "<< myDatasetDevice << std::endl;
            searchMore = okRead = false;
	  }
    }
  }

  vit0 = vit1 = myValues.begin();
  if (myValues.size() > 1) ++vit1;
  myChannel = channel;
  return okRead ? 0 : -3;
}


/*!
  \brief Helper splitting the \a heading string into a vector of channel names.
*/

static int splitHeader (std::vector<std::string>& channels,
                        const std::string& heading, unsigned int maxChn = 0)
{
  size_t nextTab = heading.find_first_of(", \t");
  if (nextTab == std::string::npos) return 0; // No column separator found

  if (maxChn > 0) channels.reserve(maxChn);

  int numChannels = 0;
  const char* sep = heading[nextTab] == ',' ? "," : " \t";
  while (nextTab < heading.size() && (maxChn == 0 || channels.size() < maxChn))
  {
    while (++nextTab < heading.size() && isspace(heading[nextTab]));
    size_t secondTab = heading.find_first_of(sep,nextTab);
    if (secondTab < heading.size())
      channels.push_back(heading.substr(nextTab,secondTab-nextTab));
    else
      channels.push_back(heading.substr(nextTab));
    nextTab = secondTab;
    while (!channels.back().empty() && isspace(channels.back().back()))
      channels.back().pop_back();
    if (!channels.back().empty())
      ++numChannels;
  }

  return numChannels;
}


bool FiASCFile::initialDeviceRead()
{
  std::string heading;
  bool okRead = true;
  int dataLine = 0;
  int wantChannel = myNumChannels;
#if FI_DEBUG > 1
  std::cout <<"FiASCFile::initialDeviceRead: wantChannel="<< wantChannel
            << std::endl;
#endif
  for (int currentLine = 1; !FT_eof(myFile); currentLine++)
  {
    char* c = FiASCFile::readLine(myFile);
    if (!c) continue;

    // Find the first two values on this line
    int valCount = 0;
    bool searchMore = true;
    double oTmpVal, tmpVal = 0.0;

    while (searchMore && *c != '\0' && *c != '\n' && *c != '\r')
    {
      if (valCount == 1)
        oTmpVal = tmpVal;
      char* end = c;
      tmpVal = strtod(c,&end);

      if (end != c)
      {
	c = end;
	if (++valCount > 1) // We have a valid line with numerical data
	  if ((valCount == 2 && ++dataLine == 1) || valCount == 1+wantChannel)
	  {
	    if (dataLine > 1)
	    {
              searchMore = false;
              if (oTmpVal == myValues.rbegin()->first)
                std::cerr <<"  ** Warning: ASCII-file "<< myDatasetDevice
                          <<"\n              The first column value "<< oTmpVal
                          <<" at line "<< currentLine <<" equals that of previo"
                          <<"us line.\n              Previous line is ignored."
                          << std::endl;
              else if (oTmpVal < myValues.rbegin()->first)
              {
                std::cerr <<" *** Error: Invalid ASCII-file "<< myDatasetDevice
                          <<"\n            The first column must be monotonical"
                          <<"ly increasing.\n            Line = "<< currentLine
                          <<": "<< oTmpVal <<" (previous value "
                          << myValues.rbegin()->first <<")."<< std::endl;
                myValues.clear();
                return false;
              }
	    }
	    myValues[oTmpVal] = { tmpVal };
	  }
      }
      else
	switch (*c)
	  {
	  case ',':
	    if (myValues.empty() && !valCount)
	    {
	      // First line starts with a comma,
	      // assume it is a csv-file with column headers
	      heading = "#Description" + std::string(c);
	      searchMore = false;
	      break;
	    }
	  case ' ':
	  case '\t':
	    c++;
	    if (valCount == 1) // Bugfix #347: tmpVal will be zero at this point
	      tmpVal = oTmpVal; // Restore its value read from the first column
	    break;
	  case '#': // comment line
	    if (!valCount && !strncasecmp(c,"#DESC",5))
	      heading = c;
	    searchMore = false;
	    break;
	  case '_':
	    if (currentLine == 1 && !valCount && !strncmp(c,"_t,",3))
	    {
	      // This is a csv-file with its first column being time in microsec
	      isCSVt = true;
	      heading = "#Description" + std::string(c+2);
	      searchMore = false;
	      break;
	    }
	  default:
	    if (myValues.empty() && strchr(c,','))
	    {
	      // First line starts with something else. If it contains a comma,
	      // assume it is a csv-file with comma-separated column headers.
	      heading = c;
	      searchMore = false;
	      break;
	    }
#ifndef FI_DEBUG
	    if (okRead)
#endif
              std::cerr <<" *** Error: Invalid format on line "<< currentLine
                        <<" in ASCII-file "<< myDatasetDevice << std::endl;
            searchMore = okRead = false;
	  }
    }

    if (dataLine == 1 && okRead)
    {
      // Let the first line with numerical data determine the number of channels
      myNumChannels = valCount-1;
      if (wantChannel > myNumChannels)
	wantChannel = 1;
    }
  }

  // Get channel names from the description line, if present
  if (myNumChannels > 0 && !heading.empty())
    if (splitHeader(chn,heading,myNumChannels) != myNumChannels)
    {
      std::cerr <<" *** Error: Invalid header in ASCII-file "
                << myDatasetDevice <<"\n     "<< heading << std::endl;
      okRead = false;
    }

  myChannel = wantChannel; // the wanted channel is now in core

  if (myValues.empty())
  {
#ifdef FI_DEBUG
    std::cerr <<" *** Error: Empty ASCII-file "<< myDatasetDevice << std::endl;
#endif
    okRead = false;
  }
  else
  {
    vit0 = vit1 = myValues.begin();
    if (myValues.size() > 1) ++vit1;

    if (isCSVt)
    {
      // Convert first column (time) from microsec to sec
      ValuesMap newValues;
      for (ValuesIter it = vit0; it != myValues.end(); ++it)
        newValues[it->first*1.0e-6] = it->second;
      myValues.swap(newValues);
    }
  }
#if FI_DEBUG > 1
  std::cout <<"FiASCFile::initialDeviceRead: "<< myChannel
            <<" "<< myNumChannels <<" "<< myValues.size() << std::endl;
#endif
  return okRead;
}


bool FiASCFile::concludingDeviceWrite(bool noHeader)
{
  const char* fmts[3] =
  {
    "\t%.4e",
    "\t%.8e",
    "\t%.16e"
  };

  int nLines = 0;
  bool success = true;
  char cline[64];

  if (FT_setbuf(bufferSize*1024))
    std::cout <<" ==> FiASCFile: Using output buffer "<< bufferSize
              <<"KB"<< std::endl;

  if (!myParent.empty() && !noHeader)
  {
    FFaAppInfo current;
    this->writeString("#FEDEM\t",current.version);
    this->writeString("\n#PARENT\t",myParent);
    this->writeString("\n#FILE \t",myDatasetDevice);
    this->writeString("\n#USER \t",current.user);
    this->writeString("\n#DATE \t",current.date);
    this->writeString("\n#\n");
  }

  if (!chn.empty() && !noHeader)
  {
    cline[0] = '\t';
    cline[63] = '\0';
    success = this->writeString("#DESCRIPTION");
    for (size_t i = 0; i < chn.size() && success; i++)
    {
      strncpy(cline+1,chn[i].c_str(),62);
      success = this->writeString(cline);
    }
    if (success)
      success = this->writeString("\n");
  }

  for (ValuesIter v = myValues.begin(); v != myValues.end() && success; ++v)
  {
    nLines++;
    snprintf(cline,64,"%.8e",v->first);
    success = this->writeString(cline);
    for (double y : v->second)
    {
      snprintf(cline,64,fmts[outputFormat],y);
      success &= this->writeString(cline);
    }
    if (success)
      success = this->writeString("\n");
  }

  if (!success)
    std::cerr <<" *** Error: Failed to write ASCII-file "<< myDatasetDevice
              <<"\n            Failure occured writing line # "<< nLines
              << std::endl;
  return success;
}


int FiASCFile::countColumns(FT_FILE fd)
{
  int   ncol = 0;
  char* line = NULL;
  while (!FT_eof(fd) && (!line || line[0] == '#'))
    line = FiASCFile::readLine(fd);

  if (line)
    for (char* s = strtok(line,", \t"); s; ncol++)
      s = strtok(NULL,", \t\n\r");

  return ncol;
}


#if FI_DEBUG > 1
bool FiASCFile::isMultiChannel(FT_FILE fd, const char* fname, bool rewind)
#else
bool FiASCFile::isMultiChannel(FT_FILE fd, const char*, bool rewind)
#endif
{
  if (!fd) return false;

  int ncol = FiASCFile::countColumns(fd);
  if (rewind) FT_seek(fd,(FT_int)0,SEEK_SET);

#if FI_DEBUG > 1
  if (fname)
    std::cout <<"FiASCFile::isMultiChannel: "<< fname <<" ncol="<< ncol
              <<" "<< std::boolalpha << (ncol > 2) << std::endl;
#endif
  return ncol > 2;
}


int FiASCFile::getNoChannels(const char* fname)
{
  FT_FILE fd = FT_open(fname,FT_rb);
  if (!fd) return 0;

  int ncol = FiASCFile::countColumns(fd);
  FT_close(fd);

#if FI_DEBUG > 1
  std::cout <<"FiASCFile::getNoChannels: "<< fname <<" "<< ncol-1 << std::endl;
#endif
  return ncol-1;
}


bool FiASCFile::getChannelList(Strings& list)
{
  if (myNumChannels < 1) return false;

  list = chn;
  for (int i = chn.size()+1; i <= myNumChannels; i++)
    list.push_back(std::to_string(i));

  return true;
}


int FiASCFile::isChannelPresentInFile(const std::string& channel)
{
  if (myNumChannels == 1) return 1;

  for (size_t i = 0; i < chn.size() && (int)i < myNumChannels; i++)
    if (channel == chn[i])
      return i+1;

  int chnum = atoi(channel.c_str());
  return this->isChannelPresentInFile(chnum) ? chnum : 0;
}


bool FiASCFile::isChannelPresentInFile(int channel)
{
  if (myNumChannels == 1) return true;

  return (channel > 0 && channel <= myNumChannels);
}


double FiASCFile::getValue(double x, int channel, bool zeroAdjust,
                           double vertShift, double scaleFactor)
{
  if (myValues.empty()) return 0.0;

  int index = this->readChannel(channel);
  if (index < 0) return 0.0;

#if FI_DEBUG > 2
  std::cout <<"FiASCFile::getValue: "<< x <<" "<< channel
            <<" prev=["<< vit0->first <<","<< vit1->first <<"]";
#endif

  double retval;

  // First check if x is within the same interval as in the previous call.
  // This will save search time for very large files.
  if (vit0->first <= x && x <= vit1->first)
    retval = this->interpolate(x, vit0->first, vit0->second[index],
			       vit1->first, vit1->second[index]);

  else if (myValues.size() == 1)
    // Special case: single point (constant value function)
    retval = myValues.begin()->second[index];

  else if (x <= myValues.begin()->first)
  {
    // x is before the very first point in the file
    vit0 = myValues.begin();
    vit1 = vit0++;
    retval = this->extrapolate(x, vit1->first, vit1->second[index],
			       vit0->first, vit0->second[index]);
  }
  else if (x >= myValues.rbegin()->first)
  {
    // x is past the very last point in the file
    ValuesMap::reverse_iterator vIterPrev = myValues.rbegin();
    ValuesMap::reverse_iterator vIter = vIterPrev++;
    retval = this->extrapolate(x, vIterPrev->first, vIterPrev->second[index],
			       vIter->first, vIter->second[index]);
  }
  else
  {
    ++vit0; ++vit1; // check if x is in the next interval
    if (vit1 == myValues.end() || x < vit0->first || vit1->first < x)
    {
      vit0 = myValues.upper_bound(x); // first element with key > x
      vit1 = vit0--;
    }
    retval = this->interpolate(x, vit0->first, vit0->second[index],
			       vit1->first, vit1->second[index]);
  }

#if FI_DEBUG > 2
  std::cout <<" --> "<< retval << std::endl;
#endif

  // First scale the function value
  retval *= scaleFactor;

  // Then zero-adjust, shift or both
  double shiftVal = vertShift;
  if (zeroAdjust) shiftVal -= myValues.begin()->second[index]*scaleFactor;

  retval += shiftVal;
  return retval;
}


void FiASCFile::setDescription(const std::string& desc)
{
  if (myChannel == (int)chn.size() && myChannel < myNumChannels)
    chn.push_back(desc);
}


void FiASCFile::setValue(double x, double y)
{
  if (myNumChannels > 1) return; // for one-channel files only

  myValues[x] = { y };
  myChannel = 1;
}


bool FiASCFile::setData(const Doubles& x, const Doubles& y)
{
  // Increment the channel counter
  if (++myChannel > myNumChannels)
  {
    myChannel = 1;
    myValues.clear();
  }

  if (x.size() < 1 || x.size() != y.size()) return false;

  size_t i = 0;
  if (myChannel == 1)
    // Let the first channel define the x-values for all channels
    for (i = 0; i < x.size(); i++)
    {
      myValues[x[i]] = Doubles(myNumChannels,0.0);
      myValues[x[i]].front() = y[i];
    }

  else if (x.front() < x.back()) // x-values are monotonically increasing
    for (ValuesIter it = myValues.begin(); it != myValues.end(); ++it)
    {
      // Find next x-value of current channel, account for finer resolution
      while (i+1 < x.size() && it->first > x[i]) i++;

      double yVal = y[i];
      if (it->first == x[i])
      {
        // The x-values of current channel match those of the first channel
        if (i+1 < x.size()) i++;
      }
      else if (it->first < x[i])
      {
        // The current channel have a coarser resolution
        if (i > 0)
          yVal = this->interpolate(it->first,x[i-1],y[i-1],x[i],y[i]);
        else if (x.size() > 1)
          yVal = this->extrapolate(it->first,x[0],y[0],x[1],y[1]);
      }
      else if (i > 0) // it->first > x[i]
      {
        // The current channel does not have enough data, must extrapolate
        yVal = this->extrapolate(it->first,x[i-1],y[i-1],x[i],y[i]);
      }

      it->second[myChannel-1] = yVal;
    }

  else // curve data is reversed (monotonically decreasing x-axis values)
    for (ValuesIter it = myValues.begin(); it != myValues.end(); ++it)
    {
      // Find next x-value of current channel, account for finer resolution
      i = x.size()-1;
      while (i > 0 && it->first > x[i]) i--;

      double yVal = y[i];
      if (it->first == x[i])
      {
        // The x-values of current channel match those of the first channel
        if (i > 0) i--;
      }
      else if (it->first < x[i])
      {
        // The current channel have a coarser resolution
        if (i+1 < x.size())
          yVal = this->interpolate(it->first,x[i+1],y[i+1],x[i],y[i]);
        else if (x.size() > 1)
          yVal = this->extrapolate(it->first,x[1],y[1],x[0],y[0]);
      }
      else if (i+1 < x.size()) // it->first > x[i]
      {
        // The current channel does not have enough data, must extrapolate
        yVal = this->extrapolate(it->first,x[i+1],y[i+1],x[i],y[i]);
      }

      it->second[myChannel-1] = yVal;
    }

  return true;
}

void FiASCFile::setEmptyChannel(const std::string& desc)
{
  if (myChannel == 1 || myChannel == myNumChannels) return;

  this->setDescription(desc);

  // Increment the channel counter
  if (++myChannel > myNumChannels)
  {
    myChannel = 1;
    myValues.clear();
  }
}


void FiASCFile::getData(Doubles& x, Doubles& y,
                        const std::string& channel, double minX, double maxX)
{
  this->getRawData(x,y,minX,maxX,this->isChannelPresentInFile(channel));
}


void FiASCFile::getRawData(Doubles& x, Doubles& y,
                           double minX, double maxX, int channel)
{
  x.clear();
  y.clear();

  if (myNumChannels > 1 && (channel < 1 || channel > myNumChannels))
  {
#ifdef FI_DEBUG
    std::cerr <<" *** Error: Invalid channel "<< channel
              <<" for ASCII-file "<< myDatasetDevice << std::endl;
#endif
    return;
  }

  int index = this->readChannel(channel);
  if (index < 0) return;

  x.reserve(myValues.size());
  y.reserve(myValues.size());

  for (const ValuesMap::value_type& v : myValues)
    if (minX > maxX || (v.first >= minX && v.first <= maxX))
    {
      x.push_back(v.first);
      y.push_back(v.second[index]);
    }
}


/*!
  This method is equivalent to the getRawData() method if invoked with
  \a zeroAdj = \e false, \a shift = 0.0 and \a scale = 1.0, however, the former
  method will be more efficient in that case.
*/

bool FiASCFile::getValues(double x0, double x1, Doubles& x, Doubles& y,
                          int channel, bool zeroAdj, double shift, double scale)
{
  x.clear();
  y.clear();

  if (myNumChannels > 1 && (channel < 1 || channel > myNumChannels))
  {
#ifdef FI_DEBUG
    std::cerr <<" *** Error: Invalid channel "<< channel
              <<" for ASCII-file "<< myDatasetDevice << std::endl;
#endif
    return false;
  }

  int index = this->readChannel(channel);
  if (index < 0) return false;

  if (myValues.size() == 1)
  {
    // Special case, single point file
    x.resize(1, myValues.begin()->first);
    y.resize(1, zeroAdj ? 0.0 : myValues.begin()->second[index]*scale);
    return true;
  }

  if (x0 < myValues.begin()->first)  x0 = myValues.begin()->first;
  if (x1 > myValues.rbegin()->first) x1 = myValues.rbegin()->first;

  // Estimate the number of points assuming constant time step
  double yVal = myValues.rbegin()->first - myValues.begin()->first;
  size_t nPoints = myValues.size()*(size_t)((x1-x0)/yVal);
  x.reserve(nPoints);
  y.reserve(nPoints);

  for (const ValuesMap::value_type& v : myValues)
    if (v.first > x1)
      break;
    else if (v.first >= x0)
    {
      yVal = shift + v.second[index]*scale;
      if (zeroAdj) yVal -= myValues.begin()->second[index]*scale;
      x.push_back(v.first);
      y.push_back(yVal);
    }

  return true;
}


int FiASCFile::readHeader(FT_FILE fd, Strings& header)
{
  int ierr = 0;
  char* cv = NULL;
  while (!FT_eof(fd) && (cv = FiASCFile::readLine(fd,true)))
    if (!strncasecmp(cv,"#DESC",5) || strchr(cv,','))
      return splitHeader(header,cv);
    else
      --ierr;

  return ierr; // No header found
}


bool FiASCFile::readNext(FT_FILE fd, const std::vector<int>& columns,
                         Doubles& values)
{
  values.resize(columns.size(),0.0);

  char* cv = NULL;
  while (!FT_eof(fd) && (!cv || *cv == '#'))
    cv = FiASCFile::readLine(fd);
  if (!cv) return false; // nothing (more) to read

  int cCount = 0;
  while (*cv != '\0' && *cv != '\n' && *cv != '\r')
  {
    char* end = cv;
    double tv = strtod(cv,&end);
    if (end != cv)
    {
      for (size_t i = 0; i < columns.size(); i++)
        if (columns[i] == cCount)
          values[i] = tv;
      ++cCount;
      cv = end;
    }
    else if (*cv == ',' || isspace(*cv))
      ++cv;
  }

  return true;
}
