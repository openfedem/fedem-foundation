// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#include "FFaMathOps.H"
#include <stdio.h>
#include <math.h>

const double FFaMathOps::ErrVal = tan(atan(1.0)*2.0);
const double zeroTol = 1E-100L;


static bool binaryArgsOK (double*& p, double inftyTol = 1E100L)
{
  if (*p == FFaMathOps::ErrVal || fabs(*p) > inftyTol)
    *(--p) = FFaMathOps::ErrVal;
  else if (*(--p) == FFaMathOps::ErrVal || fabs(*p) > inftyTol)
    *p = FFaMathOps::ErrVal;
  else
    return true;

  return false;
}

/*
  Note: The two functions NextVal and RFunc are (should be) never invoked.
  It is only checked for equality with their function pointers during the
  evaluation (see FFaMathExpr::Val). However, their bodies can not be empty
  since then some compilers optize away the body entirely such that they end
  up with identical function pointers. This will then cause a logic error
  with segmentation fault as a consequence (KMO 8/8-2017).
*/

void FFaMathOps::NextVal(double*&)
{
  fprintf(stderr,"*** Logic error: FFaMathOps::NexVal invoked.\n");
}

void FFaMathOps::RFunc(double*&)
{
  fprintf(stderr,"*** Logic error: FFaMathOps::RFunc invoked.\n");
}

/*!
  \fn void FFaMathOps::Addition(double*& p)
  \brief Calculate the addition of two parametre.

  \param p A pointer to a table of parametre

  \return Nothing. The result is placed in the head of the pointer p.
*/
void FFaMathOps::Addition(double*& p)
{
  if (binaryArgsOK(p))
    *p += (*(p + 1));
}

/*!
  \fn void FFaMathOps::Subtraction(double*& p)
  \brief Calculate the subtraction of two parametre.

  \param p A pointer to a table of parametre

  \return Nothing. The result is placed in the head of the pointer p.
*/
void FFaMathOps::Subtraction(double*& p)
{
  if (binaryArgsOK(p))
    *p -= (*(p + 1));
}

/*!
  \fn void FFaMathOps::Multiplication(double*& p)
  \brief Calculate the multiplication of two parametre.

  \param p A pointer to a table of parametre

  \return Nothing. The result is placed in the head of the pointer p.
*/
void FFaMathOps::Multiplication(double*& p)
{
  if (!binaryArgsOK(p))
    return;
  else if (fabs(*(p+1)) < zeroTol || fabs(*p) < zeroTol)
    *p = 0.0;
  else
    *p *= (*(p+1));
}

/*!
  \fn void FFaMathOps::Division(double*& p)
  \brief Calculate the division of two parametre.

  \param p A pointer to a table of parametre

  \return Nothing. The result is placed in the head of the pointer p.
*/
void FFaMathOps::Division(double*& p)
{
  if (!binaryArgsOK(p))
    return;
  else if (fabs(*(p+1)) < zeroTol)
    *p = FFaMathOps::ErrVal;
  else if (fabs(*p) < zeroTol)
    *p = 0.0;
  else
    *p /= (*(p + 1));
}

/*!
  \fn void FFaMathOps::Modulus(double*& p)
  \brief Calculate the modulus of two parametre.

  \param p A pointer to a table of parametre

  \return Nothing. The result is placed in the head of the pointer p.
*/
void FFaMathOps::Modulus(double*& p)
{
  if (!binaryArgsOK(p))
    return;
  else if (fabs(*(p+1)) < zeroTol)
    *p = FFaMathOps::ErrVal;
  else if (fabs(*p) < zeroTol)
    *p = 0.0;
  else
    *p = fmod(*p,*(p+1));
}

/*!
  \fn void FFaMathOps::Max(double*& p)
  \brief Calculate the maximum of two parametre.

  \param p A pointer to a table of parametre

  \return Nothing. The result is placed in the head of the pointer p.
*/
void FFaMathOps::Max(double*& p)
{
  if (!binaryArgsOK(p))
    return;
  else if (*(p+1) > *p)
    *p = *(p+1);
}

/*!
  \fn void FFaMathOps::Min(double*& p)
  \brief Calculate the minimum of two parametre.

  \param p A pointer to a table of parametre

  \return Nothing. The result is placed in the head of the pointer p.
*/
void FFaMathOps::Min(double*& p)
{
  if (!binaryArgsOK(p))
    return;
  else if (*(p+1) < *p)
    *p = *(p+1);
}

/*!
  \fn void FFaMathOps::Puissance(double*& p)
  \brief Calculate the power two parametre (x^y).

  \param p A pointer to a table of parametre

  \return Nothing. The result is placed in the head of the pointer p.
*/
void FFaMathOps::Puissance(double*& p)
{
  if (!binaryArgsOK(p))
    return;

  double v1 = *p, v2 = *(p+1);
  if (fabs(v1) < zeroTol)
    *p = 0.0;
  else if (fabs(v2*log(fabs(v1))) > 11000.0)
    *p = FFaMathOps::ErrVal;
  else if (v1 < 0.0 && fmod(v2,1.0))
    *p = FFaMathOps::ErrVal;
  else
    *p = pow(v1,v2);
}

/*!
  \fn void FFaMathOps::RacineN(double*& p)
  \brief Calculate the n'th root of two parametre ( (x)^(1/y) ).

  \param p A pointer to a table of parametre

  \return Nothing. The result is placed in the head of the pointer p.
*/
void FFaMathOps::RacineN(double*& p)
{
  if (!binaryArgsOK(p))
    return;

  double v1 = *p, v2 = *(p+1);
  if (fabs(v1) < zeroTol || v2*log(fabs(v1)) < -11000.0)
    *p = FFaMathOps::ErrVal;
  else if (v2 >= 0.0)
    *p = pow(v2,1.0/v1);
  else if (fabs(fmod(v1,2.0)) == 1.0)
    *p = -pow(-v2,1.0/v1);
  else
    *p = FFaMathOps::ErrVal;
}

/*!
  \fn void FFaMathOps::Puiss10 (double*& p)
  \brief Calculate the E10 of two parametre ( xEy ).

  \param p A pointer to a table of parametre

  \return Nothing. The result is placed in the head of the pointer p.
*/
void FFaMathOps::Puiss10 (double*& p)
{
  if (!binaryArgsOK(p) || fabs(*(p+1)) < zeroTol)
    return;
  else if (fabs(*(p+1)) > 2000.0)
    *p = FFaMathOps::ErrVal;
  else if (fabs(*p) < zeroTol)
    *p = 0.0;
  else
    *p *= pow(10.0,*(p+1));
}

/*!
  \fn void FFaMathOps::ArcTangente2 (double*& p)
  \brief Calculate the atan of two parametre.

  \param p A pointer to a table of parametre

  \return Nothing. The result is placed in the head of the pointer p.
*/
void FFaMathOps::ArcTangente2 (double*& p)
{
  if (!binaryArgsOK(p,1E18L))
    return;
  else if (fabs(*p) < zeroTol && fabs(*(p+1)) < zeroTol)
    *p = FFaMathOps::ErrVal;
  else
    *p = atan2(*p,*(p+1));
}

/*!
  \fn void FFaMathOps::Absolu (double*& p)
  \brief Calculates the absolute value of a parametre.

  \param p A pointer to a table of parametre

  \return Nothing. The result is placed in the head of the pointer p.
*/
void FFaMathOps::Absolu (double*& p)
{
  if (*p != FFaMathOps::ErrVal && *p < 0.0) *p = -(*p);
}

/*!
  \fn void FFaMathOps::Oppose (double*& p)
  \brief Changes the sign of a parametre.

  \param p A pointer to a table of parametre

  \return Nothing. The result is placed in the head of the pointer p.
*/
void FFaMathOps::Oppose (double*& p)
{
  if (*p != FFaMathOps::ErrVal) *p = -(*p);
}

/*!
  \fn void FFaMathOps::ArcSinus (double*& p)
  \brief Calculates the asin of a parametre.

  \param p A pointer to a table of parametre

  \return Nothing. The result is placed in the head of the pointer p.
*/
void FFaMathOps::ArcSinus (double*& p)
{
  if (*p != FFaMathOps::ErrVal)
    *p = fabs(*p) > 1.0 ? FFaMathOps::ErrVal : asin(*p);
}

/*!
  \fn void FFaMathOps::ArcCosinus (double*& p)
  \brief Calculates the acos of a parametre.

  \param p A pointer to a table of parametre

  \return Nothing. The result is placed in the head of the pointer p.
*/
void FFaMathOps::ArcCosinus (double*&p)
{
  if (*p != FFaMathOps::ErrVal)
    *p = fabs(*p) > 1.0 ? FFaMathOps::ErrVal : acos(*p);
}

/*!
  \fn void FFaMathOps::ArcTangente (double*& p)
  \brief Calculates the atan of a parametre.

  \param p A pointer to a table of parametre

  \return Nothing. The result is placed in the head of the pointer p.
*/
void FFaMathOps::ArcTangente (double*& p)
{
  if (*p != FFaMathOps::ErrVal) *p = atan(*p);
}

/*!
  \fn void FFaMathOps::Logarithme (double*& p)
  \brief Calculates the logarithme of a parametre (log x).

  \param p A pointer to a table of parametre

  \return Nothing. The result is placed in the head of the pointer p.
*/
void FFaMathOps::Logarithme (double*& p)
{
  if (*p != FFaMathOps::ErrVal)
    *p = *p < zeroTol ? FFaMathOps::ErrVal : log10(*p);
}

/*!
  \fn void FFaMathOps::NaturalLogarithme (double*& p)
  \brief Calculates the natural logarithme of a parametre (ln x).

  \param p A pointer to a table of parametre

  \return Nothing. The result is placed in the head of the pointer p.
*/
void FFaMathOps::NaturalLogarithme (double*& p)
{
  if (*p != FFaMathOps::ErrVal)
    *p = *p < zeroTol ? FFaMathOps::ErrVal : log(*p);
}

/*!
  \fn void FFaMathOps::Exponentielle (double*& p)
  \brief Calculates the exp of a parametre (exp x).

  \param p A pointer to a table of parametre

  \return Nothing. The result is placed in the head of the pointer p.
*/
void FFaMathOps::Exponentielle (double*& p)
{
  if (*p != FFaMathOps::ErrVal)
    *p = *p > 11000.0 ? FFaMathOps::ErrVal : exp(*p);
}

/*!
  \fn void FFaMathOps::Sinus (double*& p)
  \brief Calculates the sinius of a parametre.

  \param p A pointer to a table of parametre

  \return Nothing. The result is placed in the head of the pointer p.
*/
void FFaMathOps::Sinus (double*& p)
{
  if (*p != FFaMathOps::ErrVal)
    *p = fabs(*p) > 1E18L ? FFaMathOps::ErrVal : sin(*p);
}

/*!
  \fn void FFaMathOps::Tangente (double*& p)
  \brief Calculates the tangent of a parametre.

  \param p A pointer to a table of parametre

  \return Nothing. The result is placed in the head of the pointer p.
*/
void FFaMathOps::Tangente (double*& p)
{
  if (*p != FFaMathOps::ErrVal)
    *p = fabs(*p) > 1E18L ? FFaMathOps::ErrVal : tan(*p);
}

/*!
  \fn void FFaMathOps::Cosinus (double*& p)
  \brief Calculates the cosinus of a parametre .

  \param p A pointer to a table of parametre

  \return Nothing. The result is placed in the head of the pointer p.
*/
void FFaMathOps::Cosinus (double*& p)
{
  if (*p != FFaMathOps::ErrVal)
    *p = fabs(*p) > 1E18L ? FFaMathOps::ErrVal : cos(*p);
}

/*!
  \fn void FFaMathOps::Racine (double*& p)
  \brief Calculates the square root of a parametre (sqrt x).

  \param p A pointer to a table of parametre

  \return Nothing. The result is placed in the head of the pointer p.
*/
void FFaMathOps::Racine (double*& p)
{
  if (*p != FFaMathOps::ErrVal)
    *p = *p > 1E100L || *p < 0.0 ? FFaMathOps::ErrVal : sqrt(*p);
}

/*!
  \fn void FFaMathOps::LessThan (double*& p)
  \brief Boolean calculation of the expression x < y.

  \param p A pointer to a table of parametre

  \return Nothing. The result, 1 if true and 0 if false, is placed in the
   head of the pointer p.
*/
void FFaMathOps::LessThan (double*& p)
{
  if (binaryArgsOK(p))
    *p = *p < *(p + 1);
}

/*!
  \fn void FFaMathOps::GreaterThan (double*& p)
  \brief Boolean calculation of the expression x > y.

  \param p A pointer to a table of parametre

  \return Nothing. The result, 1 if true and 0 if false, is placed in the
   head of the pointer p.
*/
void FFaMathOps::GreaterThan (double*& p)
{
  if (binaryArgsOK(p))
    *p = *p > *(p + 1);
}

/*!
  \fn void FFaMathOps::BooleanAnd (double*& p)
  \brief Boolean calculation of the expression x && y.

  \param p A pointer to a table of parametre

  \return Nothing. The result, 1 if true and 0 if false, is placed in the
   head of the pointer p.
*/
void FFaMathOps::BooleanAnd (double*& p)
{
  if (binaryArgsOK(p))
    *p = *p && *(p + 1);
}

/*!
  \fn void FFaMathOps::BooleanOr (double*& p)
  \brief Boolean calculation of the expression x || y.

  \param p A pointer to a table of parametre

  \return Nothing. The result, 1 if true and 0 if false, is placed in the
   head of the pointer p.
*/
void FFaMathOps::BooleanOr (double*& p)
{
  if (binaryArgsOK(p))
    *p = *p || *(p + 1);
}

/*!
  \fn void FFaMathOps::BooleanEqual (double*& p)
  \brief Boolean calculation of the expression x == y.

  \param p A pointer to a table of parametre

  \return Nothing. The result, 1 if true and 0 if false, is placed in the
   head of the pointer p.
*/
void FFaMathOps::BooleanEqual (double*& p)
{
  if (binaryArgsOK(p))
    *p = *p == *(p + 1);
}

/*!
  \fn void FFaMathOps::BooleanNotEqual (double*& p)
  \brief Boolean calculation of the expression x != y.

  \param p A pointer to a table of parametre

  \return Nothing. The result, 1 if true and 0 if false, is placed in the
   head of the pointer p.
*/
void FFaMathOps::BooleanNotEqual (double*& p)
{
  if (binaryArgsOK(p))
    *p = *p != *(p + 1);
}

/*!
  \fn void FFaMathOps::BooleanLessOrEqual (double*& p)
  \brief Boolean calculation of the expression x <= y.

  \param p A pointer to a table of parametre

  \return Nothing. The result, 1 if true and 0 if false, is placed in the
   head of the pointer p.
*/
void FFaMathOps::BooleanLessOrEqual (double*& p)
{
  if (binaryArgsOK(p))
    *p = *p <= *(p + 1);
}

/*!
  \fn void FFaMathOps::BooleanGreaterOrEqual (double*& p)
  \brief Boolean calculation of the expression x >= y.

  \param p A pointer to a table of parametre

  \return Nothing. The result, 1 if true and 0 if false, is placed in the
   head of the pointer p.
*/
void FFaMathOps::BooleanGreaterOrEqual (double*& p)
{
  if (binaryArgsOK(p))
    *p = *p >= *(p + 1);
}

/*!
  \fn void FFaMathOps::BooleanNot (double*& p)
  \brief Boolean calculation of the expression !x.

  \param p A pointer to a table of parametre

  \return Nothing. The result, 1 if true and 0 if false, is placed in the
   head of the pointer p.
*/
void FFaMathOps::BooleanNot (double*& p)
{
  if (*p != FFaMathOps::ErrVal) *p = !(*p);
}
