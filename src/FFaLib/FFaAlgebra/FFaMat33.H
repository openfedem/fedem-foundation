// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

/*!
  \file FFaMat33.H
  \brief Point transformations in 3D space.
*/

#ifndef FFA_MAT33_H
#define FFA_MAT33_H

#include "FFaLib/FFaAlgebra/FFaVec3.H"


/*!
  \brief Class for point transformations in 3D space.
  \details This class is a 3&times;3 matrix representing the coordinate system
  axes or a transformation of 3D point vectors. The class is equipped with many
  convenience methods for the most common operations on such objects.
*/

class FaMat33
{
public:
  //! \brief Default constructor creating an identity matrix.
  FaMat33() { this->setIdentity(); }
  //! \brief Constructor creating a matrix from a float array.
  FaMat33(const float* mat);
  //! \brief Constructor creating a matrix from a double array.
  FaMat33(const double* mat);
  //! \brief Constructor creating a matrix from 3 column vectors.
  FaMat33(const FaVec3& v0, const FaVec3& v1, const FaVec3& v2);

  //! \brief Addition operator.
  FaMat33& operator+= (const FaMat33& m);
  //! \brief Subtraction operator.
  FaMat33& operator-= (const FaMat33& m);
  //! \brief Scaling operator (multiplication by a scalar).
  FaMat33& operator*= (double d);
  //! \brief Scaling operator (division by a scalar).
  FaMat33& operator/= (double d);

  //! \brief Zero-based indexing operator accessing a matrix column (read only).
  const FaVec3& operator[] (int i) const;
  //! \brief Zero-based indexing operator accessing a column (for update).
  FaVec3&       operator[] (int i);

  //! \brief One-based indexing operator accessing a matrix element (read only).
  const double& operator() (int i, int j) const;
  //! \brief One-based indexing operator accessing an element (for update).
  double&       operator() (int i, int j);

  // Special functions

  //! \brief Returns the inverse of the matrix.
  //! \param[in] eps Zero tolerance for the matrix determinant
  FaMat33  inverse(double eps = 1.0e-16) const;
  //! \brief Initializes the matrix to the identity matrix.
  FaMat33& setIdentity();
  //! \brief Returns the transpose of matrix.
  FaMat33  transpose() const;
  //! \brief Performs a cyclic permutation of the matrix columns.
  FaMat33& shift(int delta);

  //! \brief Returns \e true if the matrix \a m equals \a *this
  //! within the given \a tolerance.
  bool isCoincident(const FaMat33& m, double tolerance = 1.0e-10) const;

  //! \brief Computes a globalized coordinate system from an X-axis vector.
  FaMat33& makeGlobalizedCS(const FaVec3& v1);
  //! \brief Computes a globalized coordinate system from two in-plane vectors.
  FaMat33& makeGlobalizedCS(const FaVec3& v1, const FaVec3& v2);
  //! \brief Computes a globalized coordinate system from three vectors.
  FaMat33& makeGlobalizedCS(const FaVec3& v0,
                            const FaVec3& v1, const FaVec3& v2);
  //! \brief Computes a globalized coordinate system from four vectors.
  FaMat33& makeGlobalizedCS(const FaVec3& v1, const FaVec3& v2,
                            const FaVec3& v3, const FaVec3& v4);

  //! \brief Compute an incremental rotation tensor from the given Euler angles.
  FaMat33& eulerRotateZYX(const FaVec3& angles);
  //! \brief Computes an incremental rotation tensor from the given \a angles.
  FaMat33& incRotate(const FaVec3& angles);

  //! \brief Returns the Euler angles corresponding to an incremental rotation.
  FaVec3 getEulerZYX() const;
  //! \brief Returns the angles corresponding to an incremental rotation.
  FaVec3 getRotation() const;

  //! \brief Returns the matrix representing a rotation about the Z-axis.
  static FaMat33 makeZrotation(double angle);
  //! \brief Returns the matrix representing a rotation about the Y-axis.
  static FaMat33 makeYrotation(double angle);
  //! \brief Returns the matrix representing a rotation about the X-axis.
  static FaMat33 makeXrotation(double angle);

  // Global operators

  //! \brief Unary negation operator.
  friend FaMat33 operator- (const FaMat33& a);
  //! \brief Global addition operator.
  friend FaMat33 operator+ (const FaMat33& a, const FaMat33& b);
  //! \brief Global subtraction operator.
  friend FaMat33 operator- (const FaMat33& a, const FaMat33& b);

  //! \brief Multiplication of two matrices.
  friend FaMat33 operator* (const FaMat33& a, const FaMat33& b);
  //! \brief Scaling operator, multiplication of a matrix and a scalar.
  friend FaMat33 operator* (const FaMat33& a, double d);
  //! \brief Scaling operator, multiplication of a scalar and a matrix.
  friend FaMat33 operator* (double d, const FaMat33& a);
  //! \brief Multiplication of a matrix and a point vector.
  friend FaVec3  operator* (const FaMat33& a, const FaVec3&  b);
  //! \brief Multiplication of a point vector and matrix.
  friend FaVec3  operator* (const FaVec3&  a, const FaMat33& b);
  //! \brief Scaling operator, division by a scalar.
  friend FaMat33 operator/ (const FaMat33& a, double d);

  //! \brief Equality operator.
  friend int operator== (const FaMat33& a, const FaMat33& b);
  //! \brief Inequality operator.
  friend int operator!= (const FaMat33& a, const FaMat33& b);

  //! \brief Printing operator.
  friend std::ostream& operator<< (std::ostream& s, const FaMat33& m);
  //! \brief Reading operator.
  friend std::istream& operator>> (std::istream& s, FaMat33& m);

private:
  FaVec3 v[3]; //!< Columns of the 3&times;3 matrix
};


// --- inline functions ---

inline const FaVec3& FaMat33::operator[] (int i) const
{
#ifdef FFA_INDEXCHECK
  if (i < 0 || i > 2)
    std::cerr <<"FaMat33::operator[]: index i="<< i <<" is out of range [0,2]"
              << std::endl;
#endif
  return v[i];
}

inline FaVec3& FaMat33::operator[] (int i)
{
#ifdef FFA_INDEXCHECK
  if (i < 0 || i > 2)
    std::cerr <<"FaMat33::operator[]: index i="<< i <<" is out of range [0,2]"
              << std::endl;
#endif
  return v[i];
}

// Fortran-like 1-based indexing operators.
// The first index is the row number, whereas the second is the column number.

inline const double& FaMat33::operator() (int i, int j) const
{
#ifdef FFA_INDEXCHECK
  if (i < 1 || i > 3)
    std::cerr <<"FaMat33::operator(): index i="<< i <<" is out of range [1,3]"
              << std::endl;
  if (j < 1 || j > 3)
    std::cerr <<"FaMat33::operator(): index j="<< j <<" is out of range [1,3]"
              << std::endl;
#endif
  return v[j-1][i-1];
}

inline double& FaMat33::operator() (int i, int j)
{
#ifdef FFA_INDEXCHECK
   if (i < 1 || i > 3)
     std::cerr <<"FaMat33::operator(): index i="<< i <<" is out of range [1,3]"
               << std::endl;
   if (j < 1 || j > 3)
     std::cerr <<"FaMat33::operator(): index j="<< j <<" is out of range [1,3]"
               << std::endl;
#endif
  return v[j-1][i-1];
}

#endif
