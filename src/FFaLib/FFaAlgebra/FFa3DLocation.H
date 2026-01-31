// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#ifndef FFA_3D_LOCATION_H
#define FFA_3D_LOCATION_H

#include "FFaLib/FFaAlgebra/FFaVec3.H"
#include "FFaLib/FFaString/FFaEnum.H"
#include <array>

class FaMat33;
class FaMat34;


class FFa3DLocation
{
public:

  enum PosType {
    CART_X_Y_Z,  //!< Cartesian coordinates
    CYL_R_YR_X,  //!< Cylindric coordinates, angle from Y-axis about X-axis
    CYL_R_ZR_Y,  //!< Cylindric coordinates, angle from Z-axis about Y-axis
    CYL_R_XR_Z,  //!< Cylindric coordinates, angle from X-axis about Z-axis
    SPH_R_XR_YZ, //!< Spherical coordinates, asimuth angle from X-axis
    SPH_R_YR_ZX, //!< Spherical coordinates, asimuth angle from Y-axis
    SPH_R_ZR_XY  //!< Spherical coordinates, asimuth angle from Z-axis
  };

  enum RotType {
    EUL_Z_Y_X , //!< Euler ZYX-angles. Supposed to be entered [ X, Y, Z]
    PNT_PX_PXY, //!< Point on X-axis and point in XY-plane
    PNT_PZ_PXZ, //!< Point on Z-axis and point in XZ-plane
    DIR_EX_EXY  //!< Direction vector along X-axis, direction vector in XY-plane
  };

private:

  FFaEnumMapping(PosType) {
    FFaEnumEntry(CART_X_Y_Z , "CART_X_Y_Z");
    FFaEnumEntry(CYL_R_YR_X , "CYL_R_YR_X");
    FFaEnumEntry(CYL_R_ZR_Y , "CYL_R_ZR_Y");
    FFaEnumEntry(CYL_R_XR_Z , "CYL_R_XR_Z");
    FFaEnumEntry(SPH_R_XR_YZ, "SPH_R_XR_YZ");
    FFaEnumEntry(SPH_R_YR_ZX, "SPH_R_YR_ZX");
    FFaEnumEntry(SPH_R_ZR_XY, "SPH_R_ZR_XY");
    FFaEnumEntryEnd;
  };

  FFaEnumMapping(RotType) {
    FFaEnumEntry(EUL_Z_Y_X , "EUL_Z_Y_X" );
    FFaEnumEntry(PNT_PZ_PXZ, "PNT_PZ_PXZ");
    FFaEnumEntry(PNT_PX_PXY, "PNT_PX_PXY");
    FFaEnumEntry(DIR_EX_EXY, "DIR_EX_EXY");
    FFaEnumEntryEnd;
  };

  PosTypeEnum myPosTyp = CART_X_Y_Z;
  RotTypeEnum myRotTyp = EUL_Z_Y_X;

  std::array<FaVec3,3> myL;

  bool saveNumericalData = false; //!< If false, only the type enums are saved

public:
  //! \brief Default constructor.
  FFa3DLocation() { saveNumericalData = false; }
  //! \brief Constructor defining the orientation from two direction vectors.
  FFa3DLocation(PosType t, const FaVec3& v0,
		RotType r, const FaVec3& v1, const FaVec3& v2);
  //! \brief Constructor defining the orientation from a rotation angles.
  FFa3DLocation(PosType t, const FaVec3& v0, RotType r, const FaVec3& v1);
  //! \brief Constructor defining the location from a position matrix.
  FFa3DLocation(const FaMat34& m);
  //! \brief Copy constructor.
  FFa3DLocation(const FFa3DLocation& m) { *this = m; }

  //! \brief Let this instance save its numerical data.
  void saveMyNumericalData() { saveNumericalData = true; }

  //! \brief Assignment operator.
  FFa3DLocation& operator=(const FFa3DLocation& m);
  //! \brief Comparison operator.
  bool operator==(const FFa3DLocation& m) const;

  //! \brief Indexing operator.
  const FaVec3& operator[](int i) const;
  //! \brief Indexing operator, for assignment.
  FaVec3& operator[](int i);

  PosType getPosType() const { return myPosTyp; }
  RotType getRotType() const { return myRotTyp; }
  int getNumFields()   const { return myRotTyp == EUL_Z_Y_X ? 6 : 9; }

  // Converting and setting

  bool changePosType(PosType newType);
  bool changeRotType(RotType newType);

  FFa3DLocation& changePosRefCS(const FaMat34& newRef, const FaMat34& oldRef);
  FFa3DLocation& changeRotRefCS(const FaMat34& newRef, const FaMat34& oldRef);

  FFa3DLocation& set(PosType p, const FaMat34& posRelMx,
                     RotType r, const FaMat34& rotRelMx, const FaMat34& glbPos);
  FFa3DLocation& set(PosType p, RotType r, const FaMat34& glbPos);

  FFa3DLocation& setPos(PosType p, const FaVec3& cartPos);
  FFa3DLocation& setRot(RotType r, const FaMat33& rotMat);

  FaVec3 translation() const;
  FaMat33 direction() const;
  FaMat34 getMatrix() const;
  FaMat34 getMatrix(const FaMat34& posRelMx, const FaMat34& rotRelMx) const;
  bool isValid() const;

  std::ostream& print(std::ostream& s, bool includeNumericalData = true) const;

  // Global operators

  friend std::ostream& operator<<(std::ostream& s, const FFa3DLocation& m);
  friend std::istream& operator>>(std::istream& s, FFa3DLocation& m);
};


inline const FaVec3& FFa3DLocation::operator[] (int i) const
{
#ifdef FFA_INDEXCHECK
  if (i < 0 || i > 2)
    std::cerr <<"FFa3DLocation::operator[]: index i="<< i
              <<" is out of range [0,2]"<< std::endl;
#endif
  return myL[i];
}

inline FaVec3& FFa3DLocation::operator[] (int i)
{
#ifdef FFA_INDEXCHECK
  if (i < 0 || i > 2)
    std::cerr <<"FFa3DLocation::operator[]: index i="<< i
              << " is out of range [0,2]"<< std::endl;
#endif
  return myL[i];
}

#endif
