// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

/*!
  \file FiDeviceFunctionFactory.C
  \brief External device function factory (functions from file).
*/

#include <algorithm>
#include <numeric>

#include "FiDeviceFunctions/FiDeviceFunctionFactory.H"
#include "FiDeviceFunctions/FiDACFile.H"
#include "FiDeviceFunctions/FiASCFile.H"
#include "FiDeviceFunctions/FiRPC3File.H"
#include "FFaLib/FFaOS/FFaFilePath.H"
#include "FFaLib/FFaString/FFaStringExt.H"
#include "FFaLib/FFaString/FFaTokenizer.H"

int FiDeviceFunctionFactory::myNumExtFun = 0;


FiDeviceFunctionFactory::~FiDeviceFunctionFactory()
{
  for (FiDeviceFunctionBase* ds : myDevices)
    delete ds;

  if (myExtFnFile)
    FT_close(myExtFnFile);
}


int FiDeviceFunctionFactory::open(const std::string& fileName,
				  FiDevFormat format, FiStatus status,
				  int inputChannel, bool littleEndian)
{
  int fileInd = 0;
  if (!fileName.empty() && inputChannel > 0)
  {
    FileChannel fcIndex(fileName,inputChannel);
    FileChnMap::const_iterator it = myFileChannelMap.find(fcIndex);
    if (it == myFileChannelMap.end())
      return this->create(fileName,inputChannel,format,status,littleEndian);
    else
      fileInd = it->second;
  }
  else
  {
    FileMap::const_iterator it = myFileToIndexMap.find(fileName);
    if (it == myFileToIndexMap.end())
      return this->create(fileName,inputChannel,format,status,littleEndian);
    else
      fileInd = it->second;
  }

  FiDeviceFunctionBase* ds = this->getDevice(fileInd);
  if (!ds) return -1;

  switch (ds->getFileStatus())
    {
    case FiDeviceFunctionBase::Not_Open:
      if (status != IO_READ || !ds->open(FiDeviceFunctionBase::Read_Only))
        return -2; // Cannot have multiple instances of output files
    case FiDeviceFunctionBase::Read_Only:
    case FiDeviceFunctionBase::Write_Only:
      ds->ref();
      return fileInd;
    default:
      return -1; // Tried to open earlier, but failed...
    }
}


void FiDeviceFunctionFactory::close(int fileIndex)
{
  FiDeviceFunctionBase* ds = this->getDevice(fileIndex);
  if (!ds) return;

  std::string fileName = ds->getDevicename();
  if (ds->unref() == 0)
  {
    myDevices[fileIndex-1] = NULL;
    myFileToIndexMap.erase(fileName);
    for (const FileChnMap::value_type& fileChn : myFileChannelMap)
      if (static_cast<int>(fileChn.second) == fileIndex)
      {
        myFileChannelMap.erase(fileChn.first);
        break;
      }
  }
}


void FiDeviceFunctionFactory::close(const std::string& fileName)
{
  FileMap::const_iterator it = myFileToIndexMap.find(fileName);
  if (it != myFileToIndexMap.end())
    this->close(it->second);
}


////////////////////////////////////////////////////////////////////////////////
// Open a new file
//

int FiDeviceFunctionFactory::create(const std::string& fileName,
				    int inputChannel, FiDevFormat format,
				    FiStatus status, bool useLittleEndian)
{
  if (format == UNKNOWN_FILE) format = identify(fileName,"",status);
  if (format == NON_EXISTING) return -3;

  FiDeviceFunctionBase* ds;
  switch (format)
  {
    case DAC_FILE:
      ds = new FiDACFile(fileName.c_str(), useLittleEndian ?
                         FiDeviceFunctionBase::LittleEndian :
                         FiDeviceFunctionBase::BigEndian);
      break;
    case RPC_TH_FILE:
      ds = new FiRPC3File(fileName.c_str());
      break;
    case EXT_FUNC:
      // External function values are kept in a separate buffer
      if (inputChannel > myNumExtFun)
      {
        myNumExtFun = inputChannel;
        myExtValues.resize(myNumExtFun,0.0);
      }
      return 0;
    default:
      if (status == IO_READ && inputChannel > 0)
	ds = new FiASCFile(fileName.c_str(),inputChannel);
      else
	ds = new FiASCFile(fileName.c_str());
  }
  if (!ds) return -2;

  bool found;
  switch (status)
  {
    case IO_READ:
      found = ds->open(FiDeviceFunctionBase::Read_Only);
      break;
    case IO_WRITE:
      found = ds->open(FiDeviceFunctionBase::Write_Only);
      break;
    default:
      found = false;
  }
  if (!found)
  {
    delete ds;
    return -3;
  }

  myDevices.push_back(ds);
  if (format == ASC_MC_FILE && status == IO_READ && inputChannel > 0)
  {
    myFileChannelMap[std::make_pair(fileName,inputChannel)] = myDevices.size();
    ds->close(); // prevents loading another channel (and reduces # open files)
  }
  else
    myFileToIndexMap[fileName] = myDevices.size();

#if FI_DEBUG > 1
  std::cout <<"FiDeviceFunctionFactory::create("<< fileName
	    <<"): "<< myDevices.size()
	    <<","<< myFileChannelMap.size()
	    <<","<< myFileToIndexMap.size()
	    << std::endl;
#endif
  return myDevices.size();
}


FiDevFormat FiDeviceFunctionFactory::identify(const std::string& fileName,
					      const std::string& path,
					      FiStatus status)
{
  if (fileName.empty() && status == IO_READ)
    return EXT_FUNC;

  // Check if the file name has an extension
  if (fileName.find('.') == std::string::npos)
    return UNKNOWN_FILE;

  // Check if the file exists
  FT_FILE fd = 0;
  std::string fullName(fileName);
  if (status == IO_READ)
  {
    FFaFilePath::makeItAbsolute(fullName,path);
    fd = FT_open(fullName.c_str(),FT_rb);
    if (!fd)
    {
      perror(fullName.c_str());
      return NON_EXISTING;
    }
  }

  FiDevFormat format = UNKNOWN_FILE;
  FFaUpperCaseString ending(FFaFilePath::getExtension(fileName));
  if (ending == "ASC" || ending == "CSV" || ending == "TXT")
  {
    if (status == IO_READ && FiASCFile::isMultiChannel(fd,fullName.c_str()))
      format = ASC_MC_FILE;
    else
      format = ASC_FILE;
  }
  else if (ending == "TIM" || ending == "DRV" || ending == "RSP")
    format = RPC_TH_FILE;
  else if (ending == "DAC")
    format = DAC_FILE;

  if (fd) FT_close(fd);
  return format;
}


////////////////////////////////////////////////////////////////////////////////
// Get values
//

double FiDeviceFunctionFactory::getValue(int fileIndex, double arg, int& stat,
					 int channel, int zeroAdjust,
					 double vertShift, double scale) const
{
  FiDeviceFunctionBase* ds = NULL;
  if (fileIndex == 0)
  {
    // This is an external function, just return its current value
    if (stat > 0)
      std::cerr <<" *** FiDeviceFunctionFactory::getValue(): Integration of"
                <<" external functions is not available."<< std::endl;
    else if (channel < 1 || channel > myNumExtFun)
      std::cerr <<" *** FiDeviceFunctionFactory::getValue(): Channel index "
                << channel <<" is out of range [1,"<< myNumExtFun <<"]."
                << std::endl;
    else if (--channel < static_cast<int>(myExtValues.size()))
      return vertShift + scale*myExtValues[channel];
  }
  else if ((ds = this->getDevice(fileIndex)))
  {
    if (stat < 1)
      return ds->getValue(arg,channel,zeroAdjust!=0,vertShift,scale);
    else
      return ds->integrate(arg,stat,channel,vertShift,scale);
  }

  stat = -2;
  return 0.0;
}


bool FiDeviceFunctionFactory::getValues(int fileIndex, double x0, double x1,
					std::vector<double>& x,
					std::vector<double>& y,
					int channel, int zeroAdjust,
					double vertShift, double scale) const
{
  FiDeviceFunctionBase* ds = this->getDevice(fileIndex);
  if (ds)
    return ds->getValues(x0,x1,x,y,channel,zeroAdjust!=0,vertShift,scale);

  return false;
}


////////////////////////////////////////////////////////////////////////////////
// Set values
//

int FiDeviceFunctionFactory::setValue(int fileIndex, double x, double y)
{
  if (fileIndex >= 0)
  {
    FiDeviceFunctionBase* ds = this->getDevice(fileIndex);
    if (!ds) return -1;

    ds->setValue(x,y);
    return 0;
  }

  // Negative fileIndex indicates an external function index
  if (myExtValues.empty())
  {
    std::cerr <<" *** FiDeviceFunctionFactory::setValue():"
              <<" No external functions."<< std::endl;
    return -2;
  }

  size_t funcIdx = -fileIndex;
  if (funcIdx > myExtValues.size())
  {
    std::cerr <<" *** FiDeviceFunctionFactory::setValue(): Function index"
              << funcIdx <<" is out of range [1,"<< myExtValues.size()
              <<"]."<< std::endl;
    return -1;
  }

  if (myExtFnFile)
  {
    // An external function value file was provided.
    // Need to close it such that the function values provided here
    // will override the values from that file.
    if (myExtFnStep > 0)
    {
      std::cerr <<" *** FiDeviceFunctionFactory::setValue(): Trying to"
                <<" assign external function values when "<< myExtFnStep
                <<" steps already have been read from file."<< std::endl;
      return -3;
    }
    else
    {
      std::cout <<"   * Closing the external function value file,\n"
                <<"     assuming values will be provided by"
                <<" FiDeviceFunctionFactory::setValue()."<< std::endl;
      FT_close(myExtFnFile);
      myExtFnFile = 0;
    }
  }

  myExtValues[--funcIdx] = y;
  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Set and get axis labels
//

void FiDeviceFunctionFactory::setAxisTitle(int fileIndex, int axis,
					   const char* axisText) const
{
  FiDeviceFunctionBase* ds = this->getDevice(fileIndex);
  if (ds)
    ds->setAxisTitle(axis,axisText);
}

void FiDeviceFunctionFactory::setAxisUnit(int fileIndex, int axis,
					  const char* unitText) const
{
  FiDeviceFunctionBase* ds = this->getDevice(fileIndex);
  if (ds)
    ds->setAxisUnit(axis,unitText);
}


void FiDeviceFunctionFactory::getAxisTitle(int fileIndex, int axis,
					   char* axisText, size_t n) const
{
  FiDeviceFunctionBase* ds = this->getDevice(fileIndex);
  if (ds)
    ds->getAxisTitle(axis,axisText,n);
}

void FiDeviceFunctionFactory::getAxisUnit(int fileIndex, int axis,
					  char* unitText, size_t n) const
{
  FiDeviceFunctionBase* ds = this->getDevice(fileIndex);
  if (ds)
    ds->getAxisUnit(axis,unitText,n);
}


////////////////////////////////////////////////////////////////////////////////
// Set frequency and step
//

void FiDeviceFunctionFactory::setFrequency(int fileIndex, double freq) const
{
  FiDeviceFunctionBase* ds = this->getDevice(fileIndex);
  if (ds)
    ds->setFrequency(freq);
}

void FiDeviceFunctionFactory::setStep(int fileIndex, double step) const
{
  FiDeviceFunctionBase* ds = this->getDevice(fileIndex);
  if (ds)
    ds->setStep(step);
}


////////////////////////////////////////////////////////////////////////////////
// Get step and frequency
//

double FiDeviceFunctionFactory::getStep(int fileIndex) const
{
  FiDeviceFunctionBase* ds = this->getDevice(fileIndex);
  return ds ? ds->getStep() : 0.0;
}

double FiDeviceFunctionFactory::getFrequency(int fileIndex) const
{
  FiDeviceFunctionBase* ds = this->getDevice(fileIndex);
  return ds ? ds->getFrequency() : 0.0;
}


////////////////////////////////////////////////////////////////////////////////

int FiDeviceFunctionFactory::channelIndex(int fileIndex,
					  const std::string& channel) const
{
  FiDeviceFunctionBase* ds = this->getDevice(fileIndex);
  return ds ? ds->isChannelPresentInFile(channel) : 0;
}


bool FiDeviceFunctionFactory::getChannelList(int fileIndex,
					     std::vector<std::string>& ch) const
{
  ch.clear();
  FiDeviceFunctionBase* ds = this->getDevice(fileIndex);
  return ds ? ds->getChannelList(ch) : false;
}


bool FiDeviceFunctionFactory::getChannelList(const std::string& fileName,
					     std::vector<std::string>& channels)
{
  channels.clear();
  FiDeviceFunctionBase* fileReader;
  switch (identify(fileName))
  {
    case RPC_TH_FILE:
      fileReader = new FiRPC3File(fileName.c_str());
      break;
    case ASC_MC_FILE:
      fileReader = new FiASCFile(fileName.c_str());
      break;
    case ASC_FILE:
      channels.push_back("1");
      return true;
    default:
      return false;
  }

  bool success = false;
  if (fileReader->open(FiDeviceFunctionBase::Read_Only))
  {
    success = fileReader->getChannelList(channels);
    fileReader->close();
  }

  delete fileReader;
  return success;
}


////////////////////////////////////////////////////////////////////////////////

void FiDeviceFunctionFactory::dump() const
{
  std::cout <<"Registered data files [idx, name, references]"<< std::endl;
  for (size_t i = 0; i < myDevices.size(); i++)
    if (myDevices[i])
      std::cout << i+1 <<"\t"
                << myDevices[i]->getDevicename() <<"\t"
                << myDevices[i]->getRefCount() << std::endl;
}

////////////////////////////////////////////////////////////////////////////////

FiDeviceFunctionBase* FiDeviceFunctionFactory::getDevice(size_t fileIndex) const
{
  if (fileIndex < 1 || fileIndex > myDevices.size())
  {
    std::cerr <<" *** FiDeviceFunctionFactory::getDevice(): File index "
              << fileIndex <<" is out of range [1,"<< myDevices.size()
              <<"]."<< std::endl;
    return NULL;
  }

  FiDeviceFunctionBase* ds = myDevices[fileIndex-1];
#if FI_DEBUG > 1
  std::cout <<"FiDeviceFunctionFactory::getDevice("<< fileIndex <<"):";
  if (ds) std::cout <<" "<< ds->getDevicename();
  std::cout << std::endl;
#endif
  return ds;
}

////////////////////////////////////////////////////////////////////////////////

bool FiDeviceFunctionFactory::initExtFuncFromFile(const std::string& fileName,
                                                  const std::string& labels)
{
  if (myExtFnFile)
    FT_close(myExtFnFile);

  myExtFnStep = 0;
  myExtFnFile = FT_open(fileName.c_str(),FT_rb);
  if (!myExtFnFile)
  {
    perror(fileName.c_str());
    return false;
  }

  int nerr = 0;
  std::vector<std::string> head;
  int nlab = FiASCFile::readHeader(myExtFnFile,head);
  if (nlab < 0)
  {
    std::cerr <<" *** Failed to read header from "<< fileName << std::endl;
    return false;
  }

  myExtIndex.resize(myExtValues.size(),0);
  if (!labels.empty())
  {
    FFaTokenizer chName(labels,'<','>');
    std::vector<std::string>::const_iterator it;
    for (size_t i = 0; i < chName.size() && i < myExtIndex.size(); i++)
      if ((it = std::find(head.begin(),head.end(),chName[i])) != head.end())
        myExtIndex[i] = 1 + (it - head.begin());
      else if (chName[i].empty() && (i < head.size() || head.size() > 1))
        myExtIndex[i] = 1 + i;
      else
      {
        std::cerr <<" *** The tag \""<< chName[i]
                  <<"\" is not found among the column labels { ";
        for (const std::string& h : head) std::cout <<"\""<< h <<"\" ";
        std::cerr <<"}."<< std::endl;
        nerr++;
      }
  }
  else if (head.empty() || head.size() > myExtIndex.size())
    std::iota(myExtIndex.begin(),myExtIndex.end(),1);
  else
    std::iota(myExtIndex.begin(),myExtIndex.begin()+head.size(),1);

  std::cout <<"   * External function values are read from file "
            << fileName <<"\n     using the columns";
  for (int i : myExtIndex) std::cout <<" "<< i;
  if (!head.empty())
  {
    std::cout <<"\n     which are tagged";
    int nhead = head.size();
    for (int i : myExtIndex)
      std::cout << (i > 0 && i <= nhead ? " \""+head[i-1]+"\"" : " (none)");
  }
  std::cout << std::endl;

  return nerr == 0;
}

////////////////////////////////////////////////////////////////////////////////

bool FiDeviceFunctionFactory::updateExtFuncFromFile(int nstep, bool doCount)
{
  if (!myExtFnFile)
    return false;

  for (int i = 0; i < nstep; i++)
    if (!FiASCFile::readNext(myExtFnFile,myExtIndex,myExtValues))
      return false;
    else if (doCount)
      ++myExtFnStep;

  return true;
}

////////////////////////////////////////////////////////////////////////////////

void FiDeviceFunctionFactory::storeExtFuncValues(double* data,
                                                 unsigned int ndat,
                                                 int iop, int& offs)
{
  if (iop == 0)
    offs = myNumExtFun;
  else if (ndat < offs+myExtValues.size())
    offs = ndat - (offs+myExtValues.size());
  else
  {
    data += offs;
    offs += myExtValues.size();
    if (iop == 1)
      std::copy(myExtValues.begin(),myExtValues.end(),data);
    else
      std::copy(data,data+myExtValues.size(),myExtValues.begin());
  }
}
