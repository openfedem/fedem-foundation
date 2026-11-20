// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

/*!
  \file FiRAOTable.H
  \brief Container for Response Amplitude Operator (RAO) tables.
*/

#ifndef FI_RAO_TABLE_H
#define FI_RAO_TABLE_H

#include <map>
#include <array>
#include <vector>
#include <iostream>


//! \brief Struct representing a RAO motion component.
struct RAOcomp
{
  double ampl = 0.0;  //!< Motion amplitude
  double phase = 0.0; //!< Phase shift
};

//! \brief Global input stream operator.
inline std::istream& operator>>(std::istream& is, RAOcomp& rao)
{
  is >> rao.ampl >> rao.phase;
  return is;
}

//! \brief Global output stream operator.
inline std::ostream& operator<<(std::ostream& os, const RAOcomp& rao)
{
  os << rao.ampl <<' '<< rao.phase;
  return os;
}


//! \brief Enumerator for hydrodynamic motion DOFs.
enum RAOdof {
  SURGE, //!< X-translation
  SWAY,  //!< Y-translation
  HEAVE, //!< Z-translation
  ROLL,  //!< X-rotation
  PITCH, //!< Y-rotation
  YAW,   //!< Z-ratiation
  NDOF   //!< Total number of DOF types
};


//! \brief Struct representing the six DOFs of a RAO motion component.
struct RAOentry
{
  std::array<RAOcomp,NDOF> dof; //!< The six components of the RAO motion

  RAOcomp& Surge; //!< Reference to the X-translation DOF
  RAOcomp& Sway;  //!< Reference to the Y-translation DOF
  RAOcomp& Heave; //!< Reference to the Z-translation DOF
  RAOcomp& Roll;  //!< Reference to the X-rotation DOF
  RAOcomp& Pitch; //!< Reference to the Y-rotation DOF
  RAOcomp& Yaw;   //!< Reference to the Z-rotation DOF

  //! \brief The default constructor initializes the DOF references.
  RAOentry() : Surge(dof[SURGE]), Sway(dof[SWAY]), Heave(dof[HEAVE]),
               Roll(dof[ROLL]), Pitch(dof[PITCH]), Yaw(dof[YAW]) {}

  //! \brief Indexing operator.
  const RAOcomp& operator[](int i) const { return dof[i]; }
  //! \brief Assignment operatator
  RAOentry& operator=(const RAOentry& rao) { dof = rao.dof; return *this; }
};

//! \brief Global input stream operator.
inline std::istream& operator>>(std::istream& is, RAOentry& rao)
{
  for (RAOcomp& dof : rao.dof)
    is >> dof;
  return is;
}

//! \brief Global output stream operator.
inline std::ostream& operator<<(std::ostream& os, const RAOentry& rao)
{
  for (const RAOcomp& dof : rao.dof)
    os <<'\t'<< dof;
  return os;
};


//! \brief Struct representing a wave component.
struct WaveComp
{
  double A = 0.0;     //!< Wave amplitude
  double omega = 0.0; //!< Angular frequency
  double eps = 0.0;   //!< Phase shift
  //! \brief Constructor initializing #A, #omega and #eps.
  WaveComp(double a, double o, double e) : A(a), omega(o), eps(e) {}
};

using FiWave = std::vector<WaveComp>; //!< Convenience type alias


/*!
  \brief Container class for Response Amplitude Operator (RAO) tables.
*/

class FiRAOTable
{
public:
  //! \brief Returns the size of the RAO table.
  size_t size() const { return myData.size(); }

  //! \brief Extracts the RAO data associated with DOF \a idof.
  bool getValues(std::vector<double>& x,
                 std::vector<RAOcomp>& y, RAOdof idof) const;

  //! \brief Reads RAO data from an input stream.
  bool readDirection(std::istream& is, int angle = 0);

private:
  //! \brief Evaluate the RAO table for a given frequency and DOF.
  RAOcomp getValue(double freq, RAOdof idof) const;

  //! \brief Applies the RAO table on a wave component for the given DOF.
  void applyToWave(FiWave& wave, RAOdof idof) const;

public:
  //! \brief Reads the direction angles from a ROA file.
  static bool getDirections(const std::string& RAOfile,
                            std::vector<int>& angles);

  //! \brief Applies the RAO in the specified file to a set of wave components.
  static bool applyRAO(const std::string& RAOfile, int direction,
                       const FiWave& wave, std::vector<FiWave>& motion);

  //! \brief Overloaded method specifying the wave components through a C-array.
  static bool applyRAO(const std::string& RAOfile, int direction,
                       int nRw, int nComp, const double* waveData,
                       std::vector<FiWave>& motion);

  //! \brief Overloaded method returning the motion data through a C-array.
  static bool applyRAO(const std::string& RAOfile, int direction,
                       int nRw, int nComp, const double* waveData,
                       double** motionData);

  //! \brief Global input stream operator.
  friend std::istream& operator>>(std::istream& is, FiRAOTable& rao);
  //! \brief Global output stream operator.
  friend std::ostream& operator<<(std::ostream& os, const FiRAOTable& rao);

private:
  using RAOTable = std::map<double,RAOentry>; //!< Convenience type alias

  RAOTable myData; //!< The actual RAO data container
};

#endif
