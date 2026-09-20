// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

/*!
  \file FFaMat34.H
  \brief Position matrices in 3D space.
*/

#ifndef FFA_MAT34_H
#define FFA_MAT34_H

#include "FFaLib/FFaAlgebra/FFaMat33.H"
#include "FFaLib/FFaAlgebra/FFaVec3.H"


/*!
  \brief Class for positions in 3D space.
  \details This class is a 3&times;4 matrix actually representing a 4&times;4
  matrix defining the position and orientation of a point in 3D space,
  or a coordinate system with axis directions and origin offset.
  The class is equipped with many convenience methods for the most common
  operations on position objects. Many of the mathematical opertions are defined
  as is if the fourth row of the matrix equals `0 0 0 1`.
  This row is however not stored.
*/

class FaMat34
{
public:
  //! \brief Default constructor creating an identity matrix at the origin.
  FaMat34() {}
  //! \brief Constructor creating a matrix from a float array.
  FaMat34(const float*  mat) : r(mat), p(mat+9) {}
  //! \brief Constructor creating a matrix from a double array.
  FaMat34(const double* mat) : r(mat), p(mat+9) {}
  //! \brief Constructor creating an identity matrix at the the point \a pos.
  explicit FaMat34(const FaVec3& pos) : p(pos) {}
  //! \brief Constructor creating a identity matrix from four column vectors.
  FaMat34(const FaVec3& v0, const FaVec3& v1,
          const FaVec3& v2, const FaVec3& v3) : r(v0,v1,v2), p(v3) {}
  //! \brief Constructor creating a matrix from given rotation and position.
  FaMat34(const FaMat33& m, const FaVec3& v) : r(m), p(v) {}

  //! \brief Addition operator.
  FaMat34& operator+= (const FaMat34& m);
  //! \brief Addition operator, position only.
  FaMat34& operator+= (const FaVec3&  v);
  //! \brief Subtraction operator.
  FaMat34& operator-= (const FaMat34& m);
  //! \brief Subtraction operator, position only.
  FaMat34& operator-= (const FaVec3&  v);
  //! \brief Scaling operator (multiplication by a scalar).
  FaMat34& operator*= (double d);
  //! \brief Scaling operator (division by a scalar).
  FaMat34& operator/= (double d);

  //! \brief Zero-based indexing operator accessing a matrix column (read only).
  const FaVec3& operator[] (int i) const;
  //! \brief Zero-based indexing operator accessing a column (for update).
  FaVec3&       operator[] (int i);

  //! \brief One-based indexing operator accessing a matrix element (read only).
  const double& operator() (int i, int j) const;
  //! \brief One-based indexing operator accessing an element (for update).
  double&       operator() (int i, int j);

  // Special functions

  //! \brief Returns a const reference to the orientation matrix \ref r.
  const FaMat33& direction() const { return r; }
  //! \brief Returns a const reference to the position vector \ref p.
  const FaVec3& translation() const { return p; }

  //! \brief Returns the inverse of the matrix.
  FaMat34  inverse() const;
  //! \brief Initializes the matrix to the indentity rotation at the origin.
  FaMat34& setIdentity();

  //! \brief Returns \e true if the matrix \a m equals \a *this
  //! within the given \a tolerance.
  bool isCoincident(const FaMat34& m, double tolerance = 1.0e-10) const;

  //! \brief Computes a relative rotation from the given Euler angles.
  FaMat34& eulerRotateZYX(const FaVec3& anglesXYZ, const FaMat34& fromMatrix);
  //! \brief Computes an incremental rotation from the given Euler angles.
  FaMat34& eulerRotateZYX(const FaVec3& anglesXYZ);
  //! \brief Computes a relative rotation from given Euler angles and offset.
  FaMat34& eulerTransform(const FaVec3& offsetXYZ, const FaVec3& anglesXYZ,
                          const FaMat34& fromMatrix);
  //! \brief Computes a relative rotation from given rotation angles and offset.
  FaMat34& quatrTransform(const FaVec3& offsetXYZ, const FaVec3& anglesXYZ,
                          const FaMat34& fromMatrix);

  //! \brief Computes a globalized coordinate system from trwo vectors.
  FaMat34& makeGlobalizedCS(const FaVec3& origin,
                            const FaVec3& p1);
  //! \brief Computes a globalized coordinate system from three vectors.
  FaMat34& makeGlobalizedCS(const FaVec3& origin,
                            const FaVec3& p1, const FaVec3& p2);
  //! \brief Computes a globalized coordinate system from four vectors.
  FaMat34& makeGlobalizedCS(const FaVec3& origin,
                            const FaVec3& p1, const FaVec3& p2,
                            const FaVec3& p3);

  //! \brief Computes a coordinate from points in the XY-plane and the X-axis.
  FaMat34& makeCS_X_XY(const FaVec3& origin,
                       const FaVec3& xpt, const FaVec3& xypl);
  //! \brief Computes a coordinate from points in the XZ-plane and the X-axis.
  FaMat34& makeCS_X_XZ(const FaVec3& origin,
                       const FaVec3& xpt, const FaVec3& xzpl);
  //! \brief Computes a coordinate from points in the XZ-plane and the Z-axis.
  FaMat34& makeCS_Z_XZ(const FaVec3& origin,
                       const FaVec3& zpt, const FaVec3& xzpl);

  //! \brief Calculates the projection of \a x onto the XY-plane of this matrix.
  FaVec3 projectOnXY(const FaVec3& x) const;

  //! \brief Returns the Euler angles corresponding to the orientation \ref r.
  FaVec3 getEulerZYX() const;
  //! \brief Returns the Euler angles corresponding to a relative rotation.
  FaVec3 getEulerZYX(const FaMat34& from) const;
  //! \brief Returns the \a i'th Euler angle corresponding to \ref r.
  double getEulerZYX(int i) const;
  //! \brief Returns the \a i'th Euler angle of a relative rotation.
  double getEulerZYX(int i, const FaMat34& from) const;
  //! \brief Returns the angles corresponding to the orientation \ref r.
  double getRotation(int i) const;
  //! \brief Returns the \a i'th angle corresponding to a relative rotation.
  double getRotation(int i, const FaMat34& from) const;

  // Reading and writing

  //! \brief Writes the matrix to the provided output stream.
  std::ostream& printStd(std::ostream& os) const;
  //! \brief Writes the matrix to the provided output stream.
  std::ostream& printRot(std::ostream& os) const;

  //! \brief Reads the matrix from the provided input stream.
  bool readStd(std::istream& s);
  //! \brief Reads the matrix from the provided input stream.
  bool readRot(std::istream& s);

  // Global operators

  //! \brief Unary negation operator.
  friend FaMat34 operator- (const FaMat34& a);
  //! \brief Global addition operator.
  friend FaMat34 operator+ (const FaMat34& a, const FaMat34& b);
  //! \brief Global addition operator, positions only.
  friend FaMat34 operator+ (const FaMat34& a, const FaVec3&  b);
  //! \brief Global subtraction operator.
  friend FaMat34 operator- (const FaMat34& a, const FaMat34& b);
  //! \brief Global subtraction operator, positions only.
  friend FaMat34 operator- (const FaMat34& a, const FaVec3&  b);

  //! \brief Multiplication of two matrices.
  friend FaMat34 operator* (const FaMat34& a, const FaMat34& b);
  //! \brief Multiplication of two matrices, rotations only.
  friend FaMat34 operator* (const FaMat34& a, const FaMat33& b);
  //! \brief Multiplication of a matrix and a point vector.
  friend FaVec3  operator* (const FaMat34& a, const FaVec3&  b);

  //! \brief Equality operator.
  friend bool operator== (const FaMat34& a, const FaMat34& b);
  //! \brief Inequality operator.
  friend bool operator!= (const FaMat34& a, const FaMat34& b);

  //! \brief Printing operator.
  friend std::ostream& operator<< (std::ostream& s, const FaMat34& m);
  //! \brief Reading operator.
  friend std::istream& operator>> (std::istream& s, FaMat34& m);

private:
  FaMat33 r; //!< Orientation matrix
  FaVec3  p; //!< Position vector
};


// --- inline functions ---

inline const FaVec3& FaMat34::operator[] (int i) const
{
#ifdef FFA_INDEXCHECK
  if (i < 0 || i > 3)
    std::cerr <<"FaMat34::operator[]: index i="<< i <<" is out of range [0,3]"
              << std::endl;
#endif
  return i == 3 ? p : r[i];
}

inline FaVec3& FaMat34::operator[] (int i)
{
#ifdef FFA_INDEXCHECK
  if (i < 0 || i > 3)
    std::cerr <<"FaMat34::operator[]: index i="<< i <<" is out of range [0,3]"
              << std::endl;
#endif
  return i == 3 ? p : r[i];
}

// Fortran-like 1-based indexing operators.
// The first index is the row number, whereas the second is the column number.

inline const double& FaMat34::operator() (int i, int j) const
{
  return j == 4 ? p[i-1] : r[j-1][i-1];
}

inline double& FaMat34::operator() (int i, int j)
{
  return j == 4 ? p[i-1] : r[j-1][i-1];
}

#endif
