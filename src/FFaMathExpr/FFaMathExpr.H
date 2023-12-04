// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#ifndef FFA_MATH_EXPR_H
#define FFA_MATH_EXPR_H

class FFaMathVar;
class FFaMathFunction;


class FFaMathExpr
{
  typedef void (*DoublePtRefFuncType) (double*&);

  DoublePtRefFuncType *pinstr;
  double **pvals;
  double *ppile;
  FFaMathFunction **pfuncpile;

  mutable bool containfuncflag;

public:
  enum ROperator
    {
      ErrOp, Juxt, Num, Var, Add, Sub, Opp, Mult, Div, Mod, Max, Min, Pow, Sqrt,
      NthRoot, Abs, Sin, Cos, Tg, Log, Ln, Exp, Acos, Asin, Atan, E10,
      LogicalLess, LogicalGreater, LogicalAnd, LogicalOr,
      LogicalNotEqual, LogicalEqual,
      LogicalLessOrEqual, LogicalGreaterOrEqual, LogicalNot, Fun
    };

  ROperator op;

  FFaMathExpr *mmb1, *mmb2;
  FFaMathFunction *pfunc;

private:
  double ValC;
  const FFaMathVar* pvar;

public:
  FFaMathExpr();
  FFaMathExpr(double);
  FFaMathExpr(const FFaMathExpr&);
  FFaMathExpr(const FFaMathExpr&&);
  FFaMathExpr(const FFaMathVar&);
  FFaMathExpr(const char* sp,
	      int nvarp = 0, FFaMathVar** ppvarp = 0,
	      int nfuncp = 0, FFaMathFunction** ppfuncp = 0);
  ~FFaMathExpr();

  bool HasError(const FFaMathExpr* = 0) const;

private:
  static void BCDouble(DoublePtRefFuncType*&pf, DoublePtRefFuncType*pf1,
		       DoublePtRefFuncType*pf2, double**&pv, double**pv1, double**pv2,
		       double*&pp, double*pp1, double*pp2, FFaMathFunction**&prf,
		       FFaMathFunction**prf1, FFaMathFunction**prf2, DoublePtRefFuncType f);
  static void BCSimple(DoublePtRefFuncType*&pf, DoublePtRefFuncType*pf1, double**&pv,
		       double**pv1, double*&pp, double*pp1, FFaMathFunction**&prf,
		       FFaMathFunction**prf1, DoublePtRefFuncType f);
  static void BCFun(DoublePtRefFuncType*&pf, DoublePtRefFuncType*pf1, double**&pv,
		    double**pv1, double*&pp, double*pp1, FFaMathFunction**&prf,
		    FFaMathFunction**prf1, FFaMathFunction *rf);

  bool ContainVar(const FFaMathVar&) const;
  bool ContainFunc(const FFaMathFunction&) const;
  bool ContainFuncNoRec(const FFaMathFunction&) const;

  int NMembers() const;
  FFaMathExpr NthMember(int) const;
  FFaMathExpr Substitute(const FFaMathVar&, const FFaMathExpr&) const;

  void Init();
  void BuildCode();
  void Destroy();

public:
  FFaMathExpr& operator= (const FFaMathExpr&);
  FFaMathExpr& operator= (const FFaMathExpr&&);
  bool operator== (const double v) const;
  bool operator== (const FFaMathExpr&) const;
  bool operator!= (const FFaMathExpr&) const;

  FFaMathExpr operator+ () const { return *this; }
  FFaMathExpr operator- () const;
  FFaMathExpr operator+ (const FFaMathExpr&) const;
  FFaMathExpr operator- (const FFaMathExpr&) const;
  FFaMathExpr operator* (const FFaMathExpr&) const;
  FFaMathExpr operator/ (const FFaMathExpr&) const;
  FFaMathExpr operator^ (const FFaMathExpr&) const;

  double Val() const;

  FFaMathExpr Diff(const FFaMathVar&) const;
};


class FFaMathFunction
{
  double* buf;

public:
  double (*pfuncval)(double);

  char*        name;
  signed char  type;
  FFaMathExpr  op;
  int          nvars;
  FFaMathVar** ppvar;

  FFaMathFunction();
  FFaMathFunction(double (*)(double));
  FFaMathFunction(const FFaMathExpr&, FFaMathVar*);
  FFaMathFunction(const FFaMathExpr&, int, FFaMathVar**);
  FFaMathFunction(const FFaMathFunction&) = delete;

  ~FFaMathFunction();

  FFaMathFunction& operator=(const FFaMathFunction&) = delete;

  void SetName(const char*);

  double Val(double*) const;

  bool operator==(const FFaMathFunction&) const;

  FFaMathExpr operator()(const FFaMathExpr&);
};

#endif
