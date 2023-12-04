// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#ifndef FFA_MATH_OPS_H
#define FFA_MATH_OPS_H


class FFaMathOps
{
public:
  static const double ErrVal;
  static void FonctionError(double*& p) { *p = ErrVal; }

  static void Addition(double*& p);
  static void Subtraction(double*& p);
  static void Multiplication(double*& p);
  static void Division(double*& p);
  static void Modulus(double*& p);
  static void Max(double*& p);
  static void Min(double*& p);
  static void Puissance(double*& p);
  static void RacineN(double*& p);
  static void Puiss10(double*& p);
  static void ArcTangente2(double*& p);
  static void NextVal(double*&);
  static void RFunc(double*&);
  static void JuxtF(double*&) {}
  static void Absolu(double*& p);
  static void Oppose(double*& p);
  static void ArcSinus(double*& p);
  static void ArcCosinus(double*& p);
  static void ArcTangente(double*& p);
  static void Logarithme(double*& p);
  static void NaturalLogarithme(double*& p);
  static void Exponentielle(double*& p);
  static void Sinus(double*& p);
  static void Tangente(double*& p);
  static void Cosinus(double*& p);
  static void Racine(double*& p);
  static void LessThan(double*& p);
  static void GreaterThan(double*& p);
  static void BooleanAnd(double*& p);
  static void BooleanOr(double*& p);
  static void BooleanEqual(double*& p);
  static void BooleanNotEqual(double*& p);
  static void BooleanLessOrEqual(double*& p);
  static void BooleanGreaterOrEqual(double*& p);
  static void BooleanNot(double*& p);
};

#endif
