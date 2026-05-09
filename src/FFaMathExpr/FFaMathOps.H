// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#ifndef FFA_MATH_OPS_H
#define FFA_MATH_OPS_H


namespace FFaMathOps
{
  extern const double ErrVal;

  void Addition(double*& p);
  void Subtraction(double*& p);
  void Multiplication(double*& p);
  void Division(double*& p);
  void Modulus(double*& p);
  void Max(double*& p);
  void Min(double*& p);
  void Puissance(double*& p);
  void RacineN(double*& p);
  void Puiss10(double*& p);
  void ArcTangente2(double*& p);
  void NextVal(double*& p);
  void RFunc(double*& p);
  void Absolu(double*& p);
  void Oppose(double*& p);
  void ArcSinus(double*& p);
  void ArcCosinus(double*& p);
  void ArcTangente(double*& p);
  void Logarithme(double*& p);
  void NaturalLogarithme(double*& p);
  void Exponentielle(double*& p);
  void Sinus(double*& p);
  void Tangente(double*& p);
  void Cosinus(double*& p);
  void Racine(double*& p);
  void LessThan(double*& p);
  void GreaterThan(double*& p);
  void BooleanAnd(double*& p);
  void BooleanOr(double*& p);
  void BooleanEqual(double*& p);
  void BooleanNotEqual(double*& p);
  void BooleanLessOrEqual(double*& p);
  void BooleanGreaterOrEqual(double*& p);
  void BooleanNot(double*& p);
}

#endif
