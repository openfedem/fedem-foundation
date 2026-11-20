// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

/*!
  \file FiRAOTable.C
  \brief Container for Response Amplitude Operator (RAO) tables.
*/

#include "FiRAOTable.H"
#include <fstream>
#include <string>
#include <cstring>
#include <cstdlib>
#include <cctype>


namespace
{
  //! \brief Interpolates the RAO data between two frequencies \a x0 and \a x1.
  RAOcomp interpolate(double x, double x0, double x1,
                      const RAOcomp& r0, const RAOcomp& r1)
  {
    RAOcomp retVal;
    retVal.ampl  = r0.ampl  + (x-x0) * ((r1.ampl  - r0.ampl ) / (x1-x0));
    retVal.phase = r0.phase + (x-x0) * ((r1.phase - r0.phase) / (x1-x0));
    return retVal;
  }
}


RAOcomp FiRAOTable::getValue(double freq, RAOdof idof) const
{
  if (myData.empty()) return RAOcomp();
  if (idof < 0 || idof >= NDOF) return RAOcomp();

  using RAOIter = RAOTable::const_iterator;
  RAOIter rIter = myData.upper_bound(freq); // first element with key > freq

  if (rIter != myData.end() && rIter != myData.begin())
  {
    RAOIter rIterPrev = rIter; rIterPrev--;
    return interpolate(freq, rIterPrev->first, rIter->first,
                       rIterPrev->second[idof], rIter->second[idof]);
  }
  else if (freq >= myData.rbegin()->first)
    return myData.rbegin()->second[idof];
  else
    return myData.begin()->second[idof];
}


bool FiRAOTable::getValues(std::vector<double>& x,
                           std::vector<RAOcomp>& y, RAOdof dof) const
{
  x.clear();
  y.clear();
  if (dof < 0 || dof >= NDOF) return false;

  x.reserve(myData.size());
  y.reserve(myData.size());
  for (const RAOTable::value_type& entry : myData)
  {
    x.push_back(entry.first);
    y.push_back(entry.second[dof]);
  }

  return true;
}


bool FiRAOTable::getDirections(const std::string& RAOfile,
                               std::vector<int>& angles)
{
  angles.clear();

  std::ifstream is(RAOfile,std::ios::in);
  if (!is)
  {
    std::cerr <<" *** Error: Failed to open RAO file "<< RAOfile << std::endl;
    return false;
  }

  std::string line;
  while (is.good())
  {
    getline(is,line);

    size_t i = 0;
    while (i < line.size() && isspace(line[i])) i++;

    if (i+9 < line.size())
      if (line.substr(i,i+9) == "Direction")
        angles.push_back(atoi(line.c_str()+i+10));
  }

#ifdef FI_DEBUG
  std::cout <<"FiRAOTable::readDirections:";
  for (size_t i = 0; i < angles.size(); i++)
    std::cout <<" "<< angles[i];
  std::cout << std::endl;
#endif
  return !angles.empty();
}


bool FiRAOTable::readDirection(std::istream& is, int angle)
{
#ifdef FI_DEBUG
  std::cout <<"FiRAOTable::readDirection "<< angle << std::endl;
#endif

  std::string line;
  while (is.good())
  {
    getline(is,line);

    size_t i = 0;
    while (i < line.size() && isspace(line[i])) i++;

    if (i+9 < line.size())
      if (line.substr(i,i+9) == "Direction")
        if (atoi(line.c_str()+i+10) == angle)
        {
#ifdef FI_DEBUG
          std::cout <<"FiRAOTable::readDirection "<< line << std::endl;
#endif
          is >> *this;
          return true;
        }
  }

  std::cerr <<" *** Error: Failed to read RAO table for wave direction "<< angle
            << std::endl;
  return false;
}


//! \cond DO_NOT_DOCUMENT

std::istream& operator>>(std::istream& is, FiRAOTable& rao)
{
  while (is.good())
  {
    int c = ' ';
    while (isspace(c) && is.good())
      c = is.get();
    if (c == '\'')
      is.ignore(BUFSIZ,'\n');
    else if (isdigit(c) || c == '.' || c == '-')
    {
      is.putback(c);
      double freq, period;
      is >> freq >> period;
      is >> rao.myData[freq];
    }
    else if (c >= 0)
    {
      is.putback(c);
      break;
    }
  }

  return is;
}


std::ostream& operator<<(std::ostream& os, const FiRAOTable& rao)
{
  for (const FiRAOTable::RAOTable::value_type& entry : rao.myData)
    os << entry.first <<'\t'<< entry.second << std::endl;

  return os;
}

//! \endcond


void FiRAOTable::applyToWave(FiWave& wave, RAOdof idof) const
{
#ifndef M_PI
  constexpr double M_PI = 3.14159265358979323846;
#endif

  for (WaveComp& wc : wave)
  {
    RAOcomp rao = this->getValue(wc.omega,idof);
    wc.A   *= idof < ROLL ? rao.ampl : rao.ampl*M_PI/180.0;
    wc.eps += rao.phase*M_PI/180.0;
  }
}


bool FiRAOTable::applyRAO(const std::string& RAOfile, int direction,
                          const FiWave& wave, std::vector<FiWave>& motion)
{
#ifdef FI_DEBUG
  std::cout <<"FiRAOTable::applyRAO "<< RAOfile << std::endl;
#endif

  std::ifstream is(RAOfile,std::ios::in);
  if (!is)
  {
    std::cerr <<" *** Error: Failed to open RAO file "<< RAOfile << std::endl;
    return false;
  }

  FiRAOTable rao;
  if (!rao.readDirection(is,direction))
    return false;

  motion.clear();
  motion.reserve(NDOF);
  for (int dof = 0; dof < NDOF; dof++)
  {
    motion.push_back(wave);
    rao.applyToWave(motion.back(),static_cast<RAOdof>(dof));
  }

  return true;
}


bool FiRAOTable::applyRAO(const std::string& RAOfile, int direction,
                          int nRw, int nComp, const double* waveData,
                          std::vector<FiWave>& motion)
{
  FiWave wave;
  wave.reserve(nComp);

  for (int i = 0; i < nComp*nRw; i += nRw)
    wave.emplace_back(waveData[i],waveData[i+1],waveData[i+2]);

  return applyRAO(RAOfile,direction,wave,motion);
}


bool FiRAOTable::applyRAO(const std::string& RAOfile, int direction,
                          int nRw, int nComp, const double* waveData,
                          double** motionData)
{
  std::vector<FiWave> motion;
  if (!applyRAO(RAOfile,direction,nRw,nComp,waveData,motion))
    return false;

  for (int dof = 0; dof < NDOF; dof++)
    memcpy(motionData[dof],&motion[dof].front(),nComp*3*sizeof(double));

  return true;
}
