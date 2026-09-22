// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

/*!
  \file FFaTensor2.H
  \brief 2nd order symmetric tensors in 2D space.
*/

#ifndef FFA_TENSOR2_H
#define FFA_TENSOR2_H

#include <array>
#include <iostream>

class FaVec3;
class FaMat33;
class FaMat34;
class FFaTensor1;
class FFaTensor3;


/*!
  \brief Class to represent a 2D symmetrical tensor.
  \details Tensor layout:
  \code
  T = {t11, t22, t12}
  \endcode
*/

class FFaTensor2
{
  std::array<double,3> myT; //!< Tensor component values (11,22,12)

public:
  //! \brief Default constructor initializing the tensor to 0 (or given value).
  FFaTensor2(double d = 0.0) { myT.fill(d); }
  //! \brief Constructor creating a tensor from a float array.
  FFaTensor2(const float* t) { for (int i=0; i<3; i++) myT[i] = t[i]; }
  //! \brief Constructor creating a tensor from a double array.
  FFaTensor2(const double* t) { for (int i=0; i<3; i++) myT[i] = t[i]; }
  //! \brief Copy constructor for casting from FFaTensor1.
  FFaTensor2(const FFaTensor1& t);
  //! \brief Copy constructor for casting from FFaTensor3.
  FFaTensor2(const FFaTensor3& t);
  //! \brief Constructor creating a tensor from 2 or 3 component values.
  FFaTensor2(double t11, double t22, double t12 = 0.0) { myT = {t11,t22,t12}; }

  //! \brief Initializes all tensor components to the given value.
  void fill(double d) { myT.fill(d); }

  //! \brief Overloaded assignment operator.
  FFaTensor2& operator= (const FFaTensor1& t);
  FFaTensor2& operator= (const FFaTensor3& t);

  //! \brief Addition operator.
  FFaTensor2& operator+= (const FFaTensor2& t)
  {
    for (int i=0; i<3; i++) myT[i] += t.myT[i];
    return *this;
  }

  //! \brief Subtraction operator.
  FFaTensor2& operator-= (const FFaTensor2& t)
  {
    for (int i=0; i<3; i++) myT[i] -= t.myT[i];
    return *this;
  }

  //! \brief Scaling operator (multiplication by a scalar).
  FFaTensor2& operator*= (double d)
  {
    for (int i=0; i<3; i++) myT[i] *= d;
    return *this;
  }

  //! \brief Scaling operator (division by a scalar).
  FFaTensor2& operator/= (double d)
  {
    for (int i=0; i<3; i++) myT[i] /= d;
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

  //! \brief Rotates the tensor to the given coordinate system axes.
  FFaTensor2& rotate(const double ex[2], const double ey[2]);

  //! \brief Returns the von Mises value of the tensor.
  double vonMises() const;
  //! \brief Returns the max shear value of the tensor.
  double maxShear() const;
  //! \brief Gets the max shear of the tensor as a directed vector.
  void   maxShear(FaVec3& v) const;
  //! \brief Returns the max or absolute max principal value of the tensor.
  double maxPrinsipal(bool absMax = false) const;
  //! \brief Returns the min principal value of the tensor.
  double minPrinsipal() const;

  //! \brief Gets the principal values of the tensor.
  void prinsipalValues(double& max, double& min) const;
  //! \brief Gets the principal directions and associated values of the tensor.
  void prinsipalValues(FaVec3& values, FaMat33& rotation) const;

  // Global operators

  //! \brief Unary negation operator.
  friend FFaTensor2 operator- (const FFaTensor2&);
  //! \brief Global addition operator.
  friend FFaTensor2 operator+ (const FFaTensor2&, const FFaTensor2&);
  //! \brief Global subtraction operator.
  friend FFaTensor2 operator- (const FFaTensor2&, const FFaTensor2&);

  //! \brief Scaling operator, multiplication of a tensor and a scalar.
  friend FFaTensor2 operator* (const FFaTensor2&, double);
  //! \brief Scaling operator, multiplication of a scalar and a tensor.
  friend FFaTensor2 operator* (double, const FFaTensor2&);
  //! \brief Scaling operator, division by a scalar.
  friend FFaTensor2 operator/ (const FFaTensor2&, double);

  //! \brief Equality operator,
  friend bool operator== (const FFaTensor2&, const FFaTensor2&);
  //! \brief Inequality operator,
  friend bool operator!= (const FFaTensor2&, const FFaTensor2&);

  //! \brief Multiplication of a tensor and a rotation matrix.
  friend FFaTensor3 operator* (const FFaTensor2&, const FaMat33&);
  //! \brief Multiplication of a tensor and a position matrix.
  friend FFaTensor3 operator* (const FFaTensor2&, const FaMat34&);
  //! \brief Multiplication of a rotation matrix and a tensor.
  friend FFaTensor3 operator* (const FaMat33&, const FFaTensor2&);
  //! \brief Multiplication of a position matrix and a tensor.
  friend FFaTensor3 operator* (const FaMat34&, const FFaTensor2&);

  //! \brief Printing operator.
  friend std::ostream& operator<< (std::ostream& s, const FFaTensor2& b);
  //! \brief Reading operator.
  friend std::istream& operator>> (std::istream& s, FFaTensor2& b);
};


// --- inline functions ---

inline const double& FFaTensor2::operator[] (int i) const
{
#ifdef FFA_INDEXCHECK
  if (i < 0 || i > 2)
    std::cerr <<"FFaTensor2::operator[]: index i="<< i
              <<" is out of range [0,2]"<< std::endl;
#endif
  return myT[i];
}

inline double& FFaTensor2::operator[] (int i)
{
#ifdef FFA_INDEXCHECK
  if (i < 0 || i > 2)
    std::cerr <<"FFaTensor2::operator[]: index i="<< i
              <<" is out of range [0,2]"<< std::endl;
#endif
  return myT[i];
}

#endif
