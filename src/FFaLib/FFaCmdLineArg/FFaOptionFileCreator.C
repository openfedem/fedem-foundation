// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

/*!
  \file FFaOptionFileCreator.C
  \brief Command-line option file creator.
*/

#include <cstdio>
#include <cerrno>

#include "FFaLib/FFaCmdLineArg/FFaOptionFileCreator.H"
#include "FFaLib/FFaAlgebra/FFaVec3.H"
#include "FFaLib/FFaOS/FFaFilePath.H"


void FFaOptionFileCreator::addComment(const std::string& comment,
                                      bool whiteInFront)
{
  if (whiteInFront)
    myOptions.push_back(Option("\n",""));

  std::string newComment(comment);
  for (size_t i = 0; i < newComment.size(); i++)
    if (newComment[i] == '\n')
      newComment.insert(++i,"\n# ");

  myOptions.push_back(Option("# " + newComment,""));
}


void FFaOptionFileCreator::add(const std::string& optionName, bool val)
{
  myOptions.push_back(Option(optionName + (val ? "+" : "-"),""));
}


void FFaOptionFileCreator::add(const std::string& optionName, int val)
{
  char cval[16];
  snprintf(cval,16,"%d",val);
  myOptions.push_back(Option(optionName,cval));
}


void FFaOptionFileCreator::add(const std::string& optionName, double val)
{
  char cval[32];
  snprintf(cval,32,"%.16g",val);
  myOptions.push_back(Option(optionName,cval));
}


void FFaOptionFileCreator::add(const std::string& optionName,
                               const std::pair<double,double>& val)
{
  char cval[64];
  snprintf(cval,64,"%.16g %.16g",val.first,val.second);
  myOptions.push_back(Option(optionName,cval));
}


void FFaOptionFileCreator::add(const std::string& optionName, const FaVec3& val)
{
  char cval[96];
  snprintf(cval,96,"%.16g %.16g %.16g",val.x(),val.y(),val.z());
  myOptions.push_back(Option(optionName,cval));
}


void FFaOptionFileCreator::add(const std::string& optionName,
                               const std::vector<double>& val)
{
  if (val.empty()) return;

  char cval[32];
  snprintf(cval,32,"%.16g",val.front());
  myOptions.push_back(Option(optionName,cval));
  for (size_t i = 1; i < val.size(); i++)
  {
    snprintf(cval,32," %.16g",val[i]);
    myOptions.back().second.append(cval);
  }
}


void FFaOptionFileCreator::add(const std::string& optionName,
                               const std::string& val, bool addQuotes)
{
  myOptions.push_back(Option(optionName,val));
  if (addQuotes && !val.empty())
    myOptions.back().second = "\"" + myOptions.back().second + "\"";
}


std::vector<std::string> FFaOptionFileCreator::getOptVector() const
{
  std::vector<std::string> ret;

  for (const Option& option : myOptions)
    if (option.first[0] != '#' && option.first[0] != '\n')
    {
      ret.push_back(option.first);
      if (!option.second.empty())
        ret.push_back(option.second);
    }

  return ret;
}


bool FFaOptionFileCreator::writeOptFile() const
{
  FILE* fd = fopen(myFilename.c_str(),"w");
  if (!fd)
  {
    perror(myFilename.c_str());
    return false;
  }

  std::string ext = FFaFilePath::getExtension(myFilename);
  if (ext.compare("fop") == 0)
    fprintf(fd,"#FEDEM output options\n");
  else if (ext.compare("fco") == 0)
    fprintf(fd,"#FEDEM calculation options\n");
  else
    fprintf(fd,"#FEDEM options file\n");

  for (const Option& option : myOptions)
    if (option.second.empty())
      fprintf(fd,"%s\n",option.first.c_str());
    else
      fprintf(fd,"%s %s\n",option.first.c_str(),option.second.c_str());

  return fclose(fd) ? false : true;
}
