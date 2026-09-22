// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

/*!
  \file FFaTensor3.H
  \brief 2nd order symmetric tensors in 3D space.
*/

#ifndef FFA_TENSOR3_H
#define FFA_TENSOR3_H

#include <array>
#include <iostream>

class FaVec3;
class FaMat33;
class FaMat34;
class FFaTensor1;
class FFaTensor2;


/*!
  \brief Class to represent a 3D symmetric tensor.
  \details Tensor layout:
  \code
  T = {t11, t22, t33, t12, t13, t23}
  \endcode
*/

class FFaTensor3
{
  std::array<double,6> myT; //!< Tensor component values (11,22,33,12,13,23)

public:
  //! \brief Default constructor initializing the tensor to 0 (or given value).
  FFaTensor3(double d = 0.0) { myT.fill(d); }
  //! \brief Constructor creating a tensor from a float array.
  FFaTensor3(const float* t) { for (int i=0; i<6; i++) myT[i] = t[i]; }
  //! \brief Constructor creating a tensor from a double array.
  FFaTensor3(const double* t) { for (int i=0; i<6; i++) myT[i] = t[i]; }
  //! \brief Copy constructor for casting from FFaTensor2.
  FFaTensor3(const FFaTensor2& t);
  //! \brief Copy constructor for casting from FFaTensor1.
  FFaTensor3(const FFaTensor1& t);
  //! \brief Constructor creating a diagonal tensor from a vector.
  FFaTensor3(const FaVec3& v);
  //! \brief Constructor creating a tensor from 3 to 6 component values.
  FFaTensor3(double t11      , double t22      , double t33,
             double t12 = 0.0, double t13 = 0.0, double t23 = 0.0)
  {
    myT = { t11, t22, t33, t12, t13, t23 };
  }
  //! \brief Constructor creating an intertia tensor of a tetrahedron.
  FFaTensor3(const FaVec3& v1, const FaVec3& v2, const FaVec3& v3)
  {
    this->makeInertia(v1,v2,v3);
  }

  //! \brief Initializes all tensor components to the given value.
  void fill(double d) { myT.fill(d); }

  //! \brief Overloaded assignment operator.
  FFaTensor3& operator= (const FFaTensor1& t);
  //! \brief Overloaded assignment operator.
  FFaTensor3& operator= (const FFaTensor2& t);

  //! \brief Addition operator.
  FFaTensor3& operator+= (const FFaTensor3& t)
  {
    for (int i=0; i<6; i++) myT[i] += t.myT[i];
    return *this;
  }

  //! \brief Subtraction operator.
  FFaTensor3& operator-= (const FFaTensor3& t)
  {
    for (int i=0; i<6; i++) myT[i] -= t.myT[i];
    return *this;
  }

  //! \brief Scaling operator (multiplication by a scalar).
  FFaTensor3& operator*= (double d)
  {
    for (int i=0; i<6; i++) myT[i] *= d;
    return *this;
  }

  //! \brief Scaling operator (division by a scalar).
  FFaTensor3& operator/= (double d)
  {
    for (int i=0; i<6; i++) myT[i] /= d;
    return *this;
  }

  //! \brief Accesses the internal array (read only).
  const double* getPt() const { return myT.data(); }
  //! \brief Accesses the internal array (for update).
  double*       getPt()       { return myT.data(); }

  //! \brief Zero-based indexing operator (read only).
  const double& operator[] (int i) const;
  //! \brief Zero-based indexing operator (for update).
  double&       operator[] (int i);

  // Special functions

  //! \brief Rotates the tensor to the given coordinate system.
  FFaTensor3& rotate(const FaMat33& rotMx);
  //! \brief Rotates the tensor to the given coordinate system.
  FFaTensor3& rotate(const FaMat34& rotMx);

  //! \brief Makes the inertia tensor of a tetrahedron defined by three points.
  FFaTensor3& makeInertia(const FaVec3& v1, const FaVec3& v2, const FaVec3& v3);
  //! \brief Translates an inertia tensor using the parallel-axis theorem.
  FFaTensor3& translateInertia(const FaVec3& x, double mass);

  //! \brief Returns the von Mises value of the tensor.
  double vonMises() const;
  //! \brief Returns the max shear value of the tensor.
  double maxShear() const;
  //! \brief Gets the max shear of the tensor as a directed vector.
  void   maxShear(FaVec3& v) const;
  //! \brief Returns the max or absolute max principal value of the tensor.
  double maxPrinsipal(bool absMax = false) const;
  //! \brief Returns the middle principal value of the tensor.
  double middlePrinsipal() const;
  //! \brief Returns the min principal value of the tensor.
  double minPrinsipal() const;

  //! \brief Gets the principal values of the tensor.
  void prinsipalValues(double& max, double& middle, double& min) const;
  //! \brief Gets the principal directions and associated values of the tensor.
  void prinsipalValues(FaVec3& values, FaMat33& rotation) const;

  // Global operators

  //! \brief Unary negation operator.
  friend FFaTensor3 operator- (const FFaTensor3&);
  //! \brief Global addition operator.
  friend FFaTensor3 operator+ (const FFaTensor3&, const FFaTensor3&);
  //! \brief Global subtraction operator.
  friend FFaTensor3 operator- (const FFaTensor3&, const FFaTensor3&);

  //! \brief Scaling operator, multiplication of a tensor and a scalar.
  friend FFaTensor3 operator* (const FFaTensor3&, double);
  //! \brief Scaling operator, multiplication of a scalar and a tensor.
  friend FFaTensor3 operator* (double, const FFaTensor3&);
  //! \brief Scaling operator, division by a scalar.
  friend FFaTensor3 operator/ (const FFaTensor3&, double);

  //! \brief Equality operator,
  friend bool operator== (const FFaTensor3&, const FFaTensor3&);
  //! \brief Inequality operator,
  friend bool operator!= (const FFaTensor3&, const FFaTensor3&);

  //! \brief Multiplication of a tensor and a rotation matrix.
  friend FFaTensor3 operator* (const FFaTensor3&, const FaMat33&);
  //! \brief Multiplication of a tensor and a position matrix.
  friend FFaTensor3 operator* (const FFaTensor3&, const FaMat34&);
  //! \brief Multiplication of a rotation matrix and a tensor.
  friend FFaTensor3 operator* (const FaMat33&, const FFaTensor3&);
  //! \brief Multiplication of a position matrix and a tensor.
  friend FFaTensor3 operator* (const FaMat34&, const FFaTensor3&);

  //! \brief Printing operator.
  friend std::ostream& operator<< (std::ostream&, const FFaTensor3&);
  //! \brief Reading operator.
  friend std::istream& operator>> (std::istream&, FFaTensor3&);
};


// --- inline functions ---

inline const double& FFaTensor3::operator[] (int i) const
{
#ifdef FFA_INDEXCHECK
  if (i < 0 || i > 5)
    std::cerr <<"FFaTensor3::operator[]: index i="<< i
              <<" is out of range [0,5]"<< std::endl;
#endif
  return myT[i];
}

inline double& FFaTensor3::operator[] (int i)
{
#ifdef FFA_INDEXCHECK
  if (i < 0 || i > 5)
    std::cerr <<"FFaTensor3::operator[]: index i="<< i
              <<" is out of range [0,5]"<< std::endl;
#endif
  return myT[i];
}

#endif
