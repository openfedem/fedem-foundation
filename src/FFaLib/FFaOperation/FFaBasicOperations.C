// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

/*!
  \file FFaBasicOperations.C
  \brief Implementation of operations for the basic algebra types.
  \author Jacob Storen
*/

#include <cmath>

#include "FFaLib/FFaOperation/FFaBasicOperations.H"
#include "FFaLib/FFaOperation/FFaOperation.H"
#include "FFaLib/FFaDynCalls/FFaDynCB.H"
#include "FFaLib/FFaAlgebra/FFaMat34.H"
#include "FFaLib/FFaAlgebra/FFaTensor3.H"
#include "FFaLib/FFaAlgebra/FFaTensor2.H"
#include "FFaLib/FFaAlgebra/FFaTensor1.H"


namespace
{
  static double specialValue = 0.0;
  static double specialEquiv = 0.0;

  // FaMat34 -> double

  void FaMat34ToLength(double& d, const FaMat34& m) { d = m.translation().length(); }
  void FaMat34ToPosX  (double& d, const FaMat34& m) { d = m.translation().x(); }
  void FaMat34ToPosY  (double& d, const FaMat34& m) { d = m.translation().y(); }
  void FaMat34ToPosZ  (double& d, const FaMat34& m) { d = m.translation().z(); }
  void FaMat34ToAngX  (double& d, const FaMat34& m) { d = m.getEulerZYX(0); }
  void FaMat34ToAngY  (double& d, const FaMat34& m) { d = m.getEulerZYX(1); }
  void FaMat34ToAngZ  (double& d, const FaMat34& m) { d = m.getEulerZYX(2); }
  void FaMat34ToRotX  (double& d, const FaMat34& m) { d = m.getRotation(0); }
  void FaMat34ToRotY  (double& d, const FaMat34& m) { d = m.getRotation(1); }
  void FaMat34ToRotZ  (double& d, const FaMat34& m) { d = m.getRotation(2); }

  // FaVec3 -> double

  void FaVec3ToX        (double& d, const FaVec3& v) { d = v.x(); }
  void FaVec3ToY        (double& d, const FaVec3& v) { d = v.y(); }
  void FaVec3ToZ        (double& d, const FaVec3& v) { d = v.z(); }
  void FaVec3ToLengthYZ (double& d, const FaVec3& v) { d = hypot(v.y(),v.z()); }
  void FaVec3ToLength   (double& d, const FaVec3& v) { d = v.length(); }

  // FFaTensor3 -> double

  void VonMises3      (double& out, const FFaTensor3& in) { out = in.vonMises(); }
  void maxShear3      (double& out, const FFaTensor3& in) { out = in.maxShear(); }
  void signedAbsMax3  (double& out, const FFaTensor3& in) { out = in.maxPrinsipal(true); }
  void maxPrinsipal3  (double& out, const FFaTensor3& in) { out = in.maxPrinsipal(); }
  void middlePrinsipal(double& out, const FFaTensor3& in) { out = in.middlePrinsipal(); }
  void minPrinsipal3  (double& out, const FFaTensor3& in) { out = in.minPrinsipal(); }
  void Xx3            (double& out, const FFaTensor3& in) { out = in[0]; }
  void Yy3            (double& out, const FFaTensor3& in) { out = in[1]; }
  void Zz3            (double& out, const FFaTensor3& in) { out = in[2]; }
  void Xy3            (double& out, const FFaTensor3& in) { out = in[3]; }
  void Xz3            (double& out, const FFaTensor3& in) { out = in[4]; }
  void Yz3            (double& out, const FFaTensor3& in) { out = in[5]; }

  // FFaTensor2 -> double

  void VonMises2      (double& out, const FFaTensor2& in) { out = in.vonMises(); }
  void maxShear2      (double& out, const FFaTensor2& in) { out = in.maxShear(); }
  void signedAbsMax2  (double& out, const FFaTensor2& in) { out = in.maxPrinsipal(true); }
  void maxPrinsipal2  (double& out, const FFaTensor2& in) { out = in.maxPrinsipal(); }
  void minPrinsipal2  (double& out, const FFaTensor2& in) { out = in.minPrinsipal(); }
  void Xx2            (double& out, const FFaTensor2& in) { out = in[0]; }
  void Yy2            (double& out, const FFaTensor2& in) { out = in[1]; }
  void Xy2            (double& out, const FFaTensor2& in) { out = in[2]; }

  // FFaTensor1 -> double

  void Tensor1toDouble(double& out, const FFaTensor1& in) { out = in; }

  // double -> double

  void DDNoOp(double& d, const double& v) { d = v; }
  void DDLog (double& d, const double& v) { d = (v > 0.0 ? log10(v) : HUGE_VAL); }

  // float -> double

  void FDNoOp(double& d, const float& v) { d = v; }
  void FDLog (double& d, const float& v) { d = (v > 0.0f ? log10f(v) : HUGE_VAL); }

  // int -> double

  void IDNoOp(double& d, const int& v) { d = v; }

  // DoubleVec = std::vector<double> -> double

  using DoubleVec = std::vector<double>;

  void Max(double& out, const DoubleVec& in)
  {
    double max = -HUGE_VAL;

    for (double v : in)
    {
      double value = v == specialValue ? specialEquiv : v;

      if (value != HUGE_VAL && value > max) max = value;
    }

    if (max == specialEquiv) max = specialValue;
    out = max == -HUGE_VAL ? HUGE_VAL : max;
  }

  void Min(double& out, const DoubleVec& in)
  {
    double min = HUGE_VAL;

    for (double v : in)
    {
      double value = v == specialValue ? specialEquiv : v;

      if (value < min) min = value;
    }

    out = min == specialEquiv ? specialValue : min;
  }

  void Absolute_Max(double& out, const DoubleVec& in)
  {
    bool   isValid = false;
    double max     = 0.0;

    for (double v : in)
    {
      double value = v == specialValue ? specialEquiv : v;

      if (value != HUGE_VAL)
      {
        isValid = true;
	if (fabs(max) < fabs(value)) max = value;
      }
    }

    if (max == specialEquiv) max = specialValue;
    out = isValid && max != -HUGE_VAL ? max : HUGE_VAL;
  }

  void Absolute_Min(double& out, const DoubleVec& in)
  {
    double min = HUGE_VAL;

    for (double v : in)
    {
      double value = v == specialValue ? specialEquiv : v;

      if (value != HUGE_VAL)
	if (fabs(min) > fabs(value)) min = value;
    }

    out = min == specialEquiv ? specialValue : min;
  }

  void Average(double& out, const DoubleVec& in)
  {
    size_t nrOfNAN = 0;
    double sum     = 0.0;

    for (double v : in)
    {
      double value = v == specialValue ? specialEquiv : v;

      if (value != HUGE_VAL)
	sum += value;
      else
	nrOfNAN++;
    }

    if (nrOfNAN < in.size())
    {
      double avr = sum / (in.size()-nrOfNAN);
      out = avr == specialEquiv ? specialValue : avr;
    }
    else
      out = HUGE_VAL;
  }

  void Max_Difference(double& out, const DoubleVec& in)
  {
    double min, max;
    Min(min,in);
    Max(max,in);

    if (min != HUGE_VAL && max != HUGE_VAL)
    {
      if (max == specialValue) max = specialEquiv;
      if (min == specialValue) min = specialEquiv;

      out = max - min;
    }
    else
      out = HUGE_VAL;
  }

  /*!
    \brief Normalized color value to integer transformation.

    \code
      rrggbbaa
      ff0000ff max 1023
      ffaa00ff
      ffff00ff 3/4 767
      aaff00ff
      00ff00ff 2/4 511
      00ffaaff
      00ffffff 1/4 255
      00aaffff
      0000ffff min 0
    \endcode
  */

  void fullColor(unsigned int& color, const double& normalizedNumber)
  {
    if (normalizedNumber == HUGE_VAL) {
      color = 0x888888ff;
      return;
    }

    unsigned int colorNum = (unsigned int) (normalizedNumber * 1023);

    if (colorNum >= 1023)
      color = 0xff0000ff;
    else if (colorNum >= 767)
      color = 0xffff00ff - (colorNum - 767) * 0x00010000;
    else if (colorNum >= 511)
      color = 0x00ff00ff + (colorNum - 511) * 0x01000000;
    else if (colorNum >= 255)
      color = 0x00ffffff - (colorNum - 255) * 0x00000100;
    else
      color = 0x0000ffff + (colorNum      ) * 0x00010000;
  }
}


void FFa::initBasicOps()
{
  static bool initialized = false;
  if (initialized) return;

  // FaMat34 -> double

  FFaUnaryOp<double,FaMat34>::addOperation("Position Length"  ,FFaDynCB2S(FaMat34ToLength, double&, const FaMat34&));
  FFaUnaryOp<double,FaMat34>::addOperation("Position X"       ,FFaDynCB2S(FaMat34ToPosX  , double&, const FaMat34&));
  FFaUnaryOp<double,FaMat34>::addOperation("Position Y"       ,FFaDynCB2S(FaMat34ToPosY  , double&, const FaMat34&));
  FFaUnaryOp<double,FaMat34>::addOperation("Position Z"       ,FFaDynCB2S(FaMat34ToPosZ  , double&, const FaMat34&));
  FFaUnaryOp<double,FaMat34>::addOperation("Euler Angle ZYX X",FFaDynCB2S(FaMat34ToAngX  , double&, const FaMat34&));
  FFaUnaryOp<double,FaMat34>::addOperation("Euler Angle ZYX Y",FFaDynCB2S(FaMat34ToAngY  , double&, const FaMat34&));
  FFaUnaryOp<double,FaMat34>::addOperation("Euler Angle ZYX Z",FFaDynCB2S(FaMat34ToAngZ  , double&, const FaMat34&));
  FFaUnaryOp<double,FaMat34>::addOperation("Rotation Angle X" ,FFaDynCB2S(FaMat34ToRotX  , double&, const FaMat34&));
  FFaUnaryOp<double,FaMat34>::addOperation("Rotation Angle Y" ,FFaDynCB2S(FaMat34ToRotY  , double&, const FaMat34&));
  FFaUnaryOp<double,FaMat34>::addOperation("Rotation Angle Z" ,FFaDynCB2S(FaMat34ToRotZ  , double&, const FaMat34&));

  FFaUnaryOp<double,FaMat34>::setDefaultOperation("Position X");

  // FaVec3 -> double

  FFaUnaryOp<double,FaVec3>::addOperation("X"       ,FFaDynCB2S(FaVec3ToX       , double&, const FaVec3&));
  FFaUnaryOp<double,FaVec3>::addOperation("Y"       ,FFaDynCB2S(FaVec3ToY       , double&, const FaVec3&));
  FFaUnaryOp<double,FaVec3>::addOperation("Z"       ,FFaDynCB2S(FaVec3ToZ       , double&, const FaVec3&));
  FFaUnaryOp<double,FaVec3>::addOperation("LengthYZ",FFaDynCB2S(FaVec3ToLengthYZ, double&, const FaVec3&));
  FFaUnaryOp<double,FaVec3>::addOperation("Length"  ,FFaDynCB2S(FaVec3ToLength  , double&, const FaVec3&));

  FFaUnaryOp<double,FaVec3>::setDefaultOperation("Length");

  // FFaTensor3 -> double

  FFaUnaryOp<double,FFaTensor3>::addOperation("Von Mises"       ,FFaDynCB2S(VonMises3      , double&, const FFaTensor3&));
  FFaUnaryOp<double,FFaTensor3>::addOperation("Max Shear"       ,FFaDynCB2S(maxShear3      , double&, const FFaTensor3&));
  FFaUnaryOp<double,FFaTensor3>::addOperation("Signed Abs Max"  ,FFaDynCB2S(signedAbsMax3  , double&, const FFaTensor3&));
  FFaUnaryOp<double,FFaTensor3>::addOperation("Max Principal"   ,FFaDynCB2S(maxPrinsipal3  , double&, const FFaTensor3&));
  FFaUnaryOp<double,FFaTensor3>::addOperation("Middle Principal",FFaDynCB2S(middlePrinsipal, double&, const FFaTensor3&));
  FFaUnaryOp<double,FFaTensor3>::addOperation("Min Principal"   ,FFaDynCB2S(minPrinsipal3  , double&, const FFaTensor3&));
  FFaUnaryOp<double,FFaTensor3>::addOperation("Xx"              ,FFaDynCB2S(Xx3            , double&, const FFaTensor3&));
  FFaUnaryOp<double,FFaTensor3>::addOperation("Yy"              ,FFaDynCB2S(Yy3            , double&, const FFaTensor3&));
  FFaUnaryOp<double,FFaTensor3>::addOperation("Zz"              ,FFaDynCB2S(Zz3            , double&, const FFaTensor3&));
  FFaUnaryOp<double,FFaTensor3>::addOperation("Xy"              ,FFaDynCB2S(Xy3            , double&, const FFaTensor3&));
  FFaUnaryOp<double,FFaTensor3>::addOperation("Xz"              ,FFaDynCB2S(Xz3            , double&, const FFaTensor3&));
  FFaUnaryOp<double,FFaTensor3>::addOperation("Yz"              ,FFaDynCB2S(Yz3            , double&, const FFaTensor3&));

  FFaUnaryOp<double,FFaTensor3>::setDefaultOperation("Von Mises");

  // FFaTensor2 -> double

  FFaUnaryOp<double,FFaTensor2>::addOperation("Von Mises"       ,FFaDynCB2S(VonMises2      , double&, const FFaTensor2&));
  FFaUnaryOp<double,FFaTensor2>::addOperation("Max Shear"       ,FFaDynCB2S(maxShear2      , double&, const FFaTensor2&));
  FFaUnaryOp<double,FFaTensor2>::addOperation("Signed Abs Max"  ,FFaDynCB2S(signedAbsMax2  , double&, const FFaTensor2&));
  FFaUnaryOp<double,FFaTensor2>::addOperation("Max Principal"   ,FFaDynCB2S(maxPrinsipal2  , double&, const FFaTensor2&));
  FFaUnaryOp<double,FFaTensor2>::addOperation("Min Principal"   ,FFaDynCB2S(minPrinsipal2  , double&, const FFaTensor2&));
  FFaUnaryOp<double,FFaTensor2>::addOperation("Xx"              ,FFaDynCB2S(Xx2            , double&, const FFaTensor2&));
  FFaUnaryOp<double,FFaTensor2>::addOperation("Yy"              ,FFaDynCB2S(Yy2            , double&, const FFaTensor2&));
  FFaUnaryOp<double,FFaTensor2>::addOperation("Xy"              ,FFaDynCB2S(Xy2            , double&, const FFaTensor2&));

  FFaUnaryOp<double,FFaTensor2>::setDefaultOperation("Von Mises");

  // FFaTensor1 -> double

  FFaUnaryOp<double,FFaTensor1>::addOperation("Von Mises"       ,FFaDynCB2S(Tensor1toDouble, double&, const FFaTensor1&));
  FFaUnaryOp<double,FFaTensor1>::addOperation("Max Principal"   ,FFaDynCB2S(Tensor1toDouble, double&, const FFaTensor1&));
  FFaUnaryOp<double,FFaTensor1>::addOperation("Min Principal"   ,FFaDynCB2S(Tensor1toDouble, double&, const FFaTensor1&));
  FFaUnaryOp<double,FFaTensor1>::addOperation("Xx"              ,FFaDynCB2S(Tensor1toDouble, double&, const FFaTensor1&));

  FFaUnaryOp<double,FFaTensor1>::setDefaultOperation("Von Mises");

  // double -> double

  FFaUnaryOp<double,double>::addOperation("None",FFaDynCB2S(DDNoOp, double&, const double&));
  FFaUnaryOp<double,double>::addOperation("Log" ,FFaDynCB2S(DDLog , double&, const double&));

  FFaUnaryOp<double,double>::setDefaultOperation("None");

  // float -> double

  FFaUnaryOp<double,float>::addOperation("None",FFaDynCB2S(FDNoOp, double&, const float&));
  FFaUnaryOp<double,float>::addOperation("Log" ,FFaDynCB2S(FDLog , double&, const float&));

  FFaUnaryOp<double,float>::setDefaultOperation("None");

  // int -> double

  FFaUnaryOp<double,int>::addOperation("None",FFaDynCB2S(IDNoOp, double&, const int&));

  FFaUnaryOp<double,int>::setDefaultOperation("None");

  // DoubleVec -> double

  FFaUnaryOp<double,DoubleVec>::addOperation("Max",           FFaDynCB2S(Max,           double&, const DoubleVec&));
  FFaUnaryOp<double,DoubleVec>::addOperation("Min",           FFaDynCB2S(Min,           double&, const DoubleVec&));
  FFaUnaryOp<double,DoubleVec>::addOperation("Absolute Max",  FFaDynCB2S(Absolute_Max,  double&, const DoubleVec&));
  FFaUnaryOp<double,DoubleVec>::addOperation("Absolute Min",  FFaDynCB2S(Absolute_Min,  double&, const DoubleVec&));
  FFaUnaryOp<double,DoubleVec>::addOperation("Average",       FFaDynCB2S(Average,       double&, const DoubleVec&));
  FFaUnaryOp<double,DoubleVec>::addOperation("Max Difference",FFaDynCB2S(Max_Difference,double&, const DoubleVec&));

  FFaUnaryOp<double,DoubleVec>::setDefaultOperation("Max");

  // Operation transforming a real color code into an unsigned int

  FFaUnaryOp<unsigned int,double>::addOperation("Full color", FFaDynCB2S(fullColor, unsigned int&, const double&));

  initialized = true;
}


void FFa::setSpecialResultValue(double value, double equiv)
{
  specialValue = value;
  specialEquiv = equiv;
}
