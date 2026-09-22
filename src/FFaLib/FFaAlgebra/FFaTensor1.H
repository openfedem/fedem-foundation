// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

/*!
  \file FFaTensor1.H
  \brief 2nd order symmetric tensors in 1D space.
*/

#ifndef FFA_TENSOR1_H
#define FFA_TENSOR1_H

#include <iostream>

class FFaTensor2;
class FFaTensor3;


/*!
  \brief Class to represent a 1D simple tensor.
  \details Tensor layout:
  \code
  T = {t11}
  \endcode
*/

class FFaTensor1
{
  double myT; //!< Tensor component value

public:
  //! \brief Default constructor initializing the tensor to 0 (or given value).
  FFaTensor1(double d = 0.0) { myT = d; }
  //! \brief Copy constructor for casting from FFaTensor2.
  FFaTensor1(const FFaTensor2& t);
  //! \brief Copy constructor for casting from FFaTensor1.
  FFaTensor1(const FFaTensor3& t);

  //! \brief Initializes all tensor components to the given value.
  void fill(double d) { myT = d; }

  //! \brief Overloaded assignment operator.
  FFaTensor1& operator= (const FFaTensor2& t);
  //! \brief Overloaded assignment operator.
  FFaTensor1& operator= (const FFaTensor3& t);

  //! \brief Addition operator.
  FFaTensor1& operator+= (const FFaTensor1& t)
  {
    myT += t.myT;
    return *this;
  }

  //! \brief Subtraction operator.
  FFaTensor1& operator-= (const FFaTensor1& t)
  {
    myT -= t.myT;
    return *this;
  }

  //! \brief Scaling operator (multiplication by a scalar).
  FFaTensor1& operator*= (double d)
  {
    myT *= d;
    return *this;
  }

  //! \brief Scaling operator (division by a scalar).
  FFaTensor1& operator/= (double d)
  {
    myT /= d;
    return *this;
  }

  //! \brief Type cast to double operator.
  operator double() const { return myT; }

  // Global operators

  //! \brief Unary negation operator.
  friend FFaTensor1 operator- (const FFaTensor1&);
  //! \brief Global addition operator.
  friend FFaTensor1 operator+ (const FFaTensor1&, const FFaTensor1&);
  //! \brief Global subtraction operator.
  friend FFaTensor1 operator- (const FFaTensor1&, const FFaTensor1&);

  //! \brief Scaling operator, multiplication of a tensor and a scalar.
  friend FFaTensor1 operator* (const FFaTensor1&, double);
  //! \brief Scaling operator, multiplication of a scalar and a tensor.
  friend FFaTensor1 operator* (double, const FFaTensor1&);
  //! \brief Scaling operator, division by a scalar.
  friend FFaTensor1 operator/ (const FFaTensor1&, double);

  //! \brief Equality operator,
  friend bool operator== (const FFaTensor1&, const FFaTensor1&);
  //! \brief Inequality operator,
  friend bool operator!= (const FFaTensor1&, const FFaTensor1&);

  //! \brief Printing operator.
  friend std::ostream& operator<< (std::ostream&, const FFaTensor1&);
  //! \brief Reading operator.
  friend std::istream& operator>> (std::istream&, FFaTensor1&);
};

#endif
