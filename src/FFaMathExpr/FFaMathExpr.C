// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#include "FFaMathExpr.H"
#include "FFaMathString.H"
#include "FFaMathVar.H"
#include "FFaMathOps.H"
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <cmath>
#include <utility>


/*!
  \fn FFaMathExpr::Init()
  \brief Initializes all members to zero.
  \details Used by the constructors only.
*/
void FFaMathExpr::Init()
{
  op = ErrOp;
  mmb1 = NULL;
  mmb2 = NULL;
  ValC = FFaMathOps::ErrVal;
  pvar = NULL;
  pfunc = NULL;
  pinstr = NULL;
  pvals = NULL;
  ppile = NULL;
  pfuncpile = NULL;
  containfuncflag = false;
}

/*!
  \fn FFaMathExpr::FFaMathExpr()
  \brief Default constructor of class FFaMathExpr.
*/
FFaMathExpr::FFaMathExpr()
{
  Init();
  BuildCode();
}

/*!
  \fn FFaMathExpr::~FFaMathExpr()
  \brief Destructor of class FFaMathExpr.
*/
FFaMathExpr::~FFaMathExpr()
{
  Destroy();
}

/*!
  \fn FFaMathExpr::FFaMathExpr(const FFaMathExpr&)
  \brief Copy constructor of class FFaMathExpr.
  \param[in] ROp Object of type FFaMathExpr
*/
FFaMathExpr::FFaMathExpr(const FFaMathExpr& ROp)
{
  Init();

  op = ROp.op;
  pvar = ROp.pvar;
  ValC = ROp.ValC;
  pfunc = ROp.pfunc;

  if(ROp.mmb1 != NULL)
    mmb1 = new FFaMathExpr(*(ROp.mmb1));

  if(ROp.mmb2 != NULL)
    mmb2 = new FFaMathExpr(*(ROp.mmb2));

  BuildCode();
}

/*!
  \fn FFaMathExpr::FFaMathExpr(const FFaMathExpr&&)
  \brief Move constructor of class FFaMathExpr.
  \param[in] ROp Object of type FFaMathExpr
*/
FFaMathExpr::FFaMathExpr(const FFaMathExpr&& ROp)
{
  Init();

  op = ROp.op;
  pvar = ROp.pvar;
  ValC = ROp.ValC;
  pfunc = ROp.pfunc;
  mmb1 = ROp.mmb1;
  mmb2 = ROp.mmb2;

  BuildCode();
}

/*!
  \fn FFaMathExpr::FFaMathExpr(double)
  \brief Constructor of class FFaMathExpr from a double value.
  \param[in] x The double value this expression should evaluate to
*/
FFaMathExpr::FFaMathExpr(double x)
{
  Init();

  if (x != FFaMathOps::ErrVal) {
    if (x >= 0.0) {
      op = Num;
      ValC = x;
    }
    else {
      op = Opp;
      mmb2 = new FFaMathExpr(-x);
      ValC = FFaMathOps::ErrVal;
    }
  }

  BuildCode();
}

/*!
  \fn FFaMathExpr::FFaMathExpr(const FFaMathVar&)
  \brief Constructor of class FFaMathExpr from a FFaMathVar object.
  \param[in] varp Object of type FFaMathVar
*/
FFaMathExpr::FFaMathExpr(const FFaMathVar& varp)
{
  Init();

  op = Var;
  pvar = &varp;

  BuildCode();
}


/*!
  \fn FFaMathExpr::operator=(const FFaMathExpr&)
  \brief Overloaded copy assignment operator.
  \param[in] ROp Object of type FFaMathExpr
*/
FFaMathExpr& FFaMathExpr::operator= (const FFaMathExpr& ROp)
{
  if (this == &ROp)
    return *this;

  Destroy();
  Init();

  op = ROp.op;
  pvar = ROp.pvar;
  ValC = ROp.ValC;
  pfunc = ROp.pfunc;

  if(ROp.mmb1 != NULL)
    mmb1 = new FFaMathExpr(*(ROp.mmb1));

  if(ROp.mmb2 != NULL)
    mmb2 = new FFaMathExpr(*(ROp.mmb2));

  BuildCode();

  return *this;
}

/*!
  \fn FFaMathExpr::operator=(const FFaMathExpr&&)
  \brief Overloaded move assignment operator.
  \param[in] ROp Object of type FFaMathExpr
*/
FFaMathExpr& FFaMathExpr::operator= (const FFaMathExpr&& ROp)
{
  if (this == &ROp)
    return *this;

  Destroy();
  Init();

  op = ROp.op;
  pvar = ROp.pvar;
  ValC = ROp.ValC;
  pfunc = ROp.pfunc;
  mmb1 = ROp.mmb1;
  mmb2 = ROp.mmb2;

  BuildCode();

  return *this;
}


/*!
  \fn FFaMathExpr::operator==(const double) const
  \brief Overloaded equal-to operator for double values.
  \param[in] v The double value to compare with
*/
bool FFaMathExpr::operator== (const double v) const
{
  return(this->op == Num && this->ValC == v);
}

/*!
  \fn FFaMathExpr::operator==(const FFaMathExpr&) const
  \brief Overloaded equal-to operator.
  \param[in] op2 The FFaMathExpr object to compare with
*/
bool FFaMathExpr::operator== (const FFaMathExpr& op2) const
{
  if(this->op != op2.op)
    return false;

  if(this->op == Var)
    return(*(this->pvar) == *(op2.pvar));

  if(this->op == Fun)
    return(this->pfunc == op2.pfunc); // *this->pfunc==*op2.pfunc could imply infinite loops in cases of self-dependence

  if(this->op == Num)
    return(this->ValC == op2.ValC);

  if(this->mmb1 == NULL && op2.mmb1 != NULL)
    return false;

  if(this->mmb2 == NULL && op2.mmb2 != NULL)
    return false;

  if(op2.mmb1 == NULL && this->mmb1 != NULL)
    return false;

  if(op2.mmb2 == NULL && this->mmb2 != NULL)
    return false;

  return(((this->mmb1 == NULL && op2.mmb1 == NULL) || (*(this->mmb1) == *(op2.mmb1))) &&
 	 ((this->mmb2 == NULL && op2.mmb2 == NULL) || (*(this->mmb2) == *(op2.mmb2))));
}

/*!
  \fn FFaMathExpr::operator!=(const FFaMathExpr&) const
  \brief Overloaded not-equal-to operator.
  \param[in] op2 The FFaMathExpr object to compare with
*/
bool FFaMathExpr::operator!= (const FFaMathExpr& op2) const
{
  if(this->op != op2.op)
    return true;

  if(this->op == Var)
    return(this->pvar != op2.pvar);

  if(this->op == Fun)
    return(!(this->pfunc == op2.pfunc)); // *this->pfunc==*op2.pfunc could imply infinite loops in cases of self-dependence

  if(this->op == Num)
    return(this->ValC != op2.ValC);

  if(this->mmb1 == NULL && op2.mmb1 != NULL)
    return true;

  if(this->mmb2 == NULL && op2.mmb2 != NULL)
    return true;

  if(op2.mmb1 == NULL && this->mmb1 != NULL)
    return true;

  if(op2.mmb2 == NULL && this->mmb2 != NULL)
    return true;

  return(((this->mmb1 != NULL || op2.mmb1 != NULL) && (*(this->mmb1) != *(op2.mmb1))) ||
	 ((this->mmb2 != NULL || op2.mmb2 != NULL) && (*(this->mmb2) != *(op2.mmb2))));
}

/*!
  \fn FFaMathExpr::operator-() const
  \brief Overloaded negation operator.
*/
FFaMathExpr FFaMathExpr::operator- () const
{
  if (op == Num)
    return -ValC;

  if (op == Opp)
    return *mmb2;

  FFaMathExpr resultat;
  resultat.op = Opp;
  resultat.mmb2 = new FFaMathExpr(*this);

  return resultat;
}

/*!
  \fn FFaMathExpr::operator+(const FFaMathExpr&) const
  \brief Overload addition operator.
  \param[in] op2 The FFaMathExpr object to add to this one.
*/
FFaMathExpr FFaMathExpr::operator+ (const FFaMathExpr& op2) const
{
  if(this->op == FFaMathExpr::Num && op2.op == FFaMathExpr::Num)
    return this->ValC + op2.ValC;

  if(*this == 0.0)
    return op2;

  if(op2 == 0.0)
    return *this;

  if(this->op == FFaMathExpr::Opp)
    return op2 - *(this->mmb2);

  if(op2.op == FFaMathExpr::Opp)
    return *this - *(op2.mmb2);

  FFaMathExpr resultat;
  resultat.op = FFaMathExpr::Add;
  resultat.mmb1 = new FFaMathExpr(*this);
  resultat.mmb2 = new FFaMathExpr(op2);

  return resultat;
}

/*!
  \fn FFaMathExpr::operator-(const FFaMathExpr&) const
  \brief Overloaded subtraction operator.
  \param[in] op2 The FFaMathExpr object to subtract from this one
*/
FFaMathExpr FFaMathExpr::operator- (const FFaMathExpr& op2) const
{
  if(this->op == Num && op2.op == Num)
    return this->ValC - op2.ValC;

  if(*this == 0.0)
    return -op2;

  if(op2 == 0.0)
    return *this;

  if(this->op == Opp)
    return -(op2 + *(this->mmb2));

  if(op2.op == Opp)
    return *this + *(op2.mmb2);

  FFaMathExpr resultat;
  resultat.op = Sub;
  resultat.mmb1 = new FFaMathExpr(*this);
  resultat.mmb2 = new FFaMathExpr(op2);

  return resultat;
}

/*!
  \fn FFaMathExpr::operator*(const FFaMathExpr&) const
  \brief Overload multiplication operator.
  \param[in] op2 The FFaMathExpr object to multiply this one with
*/
FFaMathExpr FFaMathExpr::operator* (const FFaMathExpr& op2) const
{
  if(this->op == Num && op2.op == Num)
    return this->ValC * op2.ValC;

  if(*this == 0.0 || op2 == 0.0)
    return 0.0;

  if(*this == 1.0)
    return op2;
  
  if(op2 == 1.0)
    return *this;

  if(this->op == Opp)
    return -(*(this->mmb2) * op2);

  if(op2.op == Opp)
    return -(*this * *(op2.mmb2));

  FFaMathExpr resultat;
  resultat.op = Mult;
  resultat.mmb1 = new FFaMathExpr(*this);
  resultat.mmb2 = new FFaMathExpr(op2);

  return resultat;
}

/*!
  \fn FFaMathExpr::operator/(const FFaMathExpr&) const
  \brief Overloaded division operator.
  \param[in] op2 The FFaMathExpr object to divide this one with
*/
FFaMathExpr FFaMathExpr::operator/ (const FFaMathExpr& op2) const
{
  if(this->op == Num && op2.op == Num)
    return (op2.ValC ? this->ValC / op2.ValC : FFaMathOps::ErrVal);

  if(*this == 0.0)
    return 0.0;
  
  if(op2 == 1.0)
    return *this;

  if(op2 == 0.0)
    return FFaMathOps::ErrVal;

  if(this->op == Opp)
    return -(*(this->mmb2) / op2);

  if(op2.op == Opp)
    return -(*this / (*(op2.mmb2)));

  FFaMathExpr resultat;
  resultat.op = Div;
  resultat.mmb1 = new FFaMathExpr(*this);
  resultat.mmb2 = new FFaMathExpr(op2);

  return resultat;
}

/*!
  \fn FFaMathExpr::operator^(const FFaMathExpr&) const
  \brief Overloaded power operator.
  \param op2 The FFaMathExpr object to raise this one with
*/
FFaMathExpr FFaMathExpr::operator^ (const FFaMathExpr& op2) const
{
  if(*this == 0.0)
    return 0.0;

  if(op2 == 0.0)
    return 1.0;

  if(op2 == 1.0)
    return *this;

  FFaMathExpr resultat;
  resultat.op = Pow;
  resultat.mmb1 = new FFaMathExpr(*this);
  resultat.mmb2 = new FFaMathExpr(op2);

  return resultat;
}


/*!
  \fn FFaMathExpr::FFaMathExpr(const char*,
                               int,FFaMathVar**,
                               int,FFaMathFunction**)
  \brief Constructor of class FFaMathExpr from a text string.
  \param[in] sp An expression string
  \param[in] nvarp variable index
  \param ppvarp object of type FFaMathVar
  \param[in] nfuncp function index
  \param ppfuncp object of type FFaMathFunction
*/
FFaMathExpr::FFaMathExpr(const char* sp,
			 int nvarp, FFaMathVar** ppvarp,
			 int nfuncp, FFaMathFunction** ppfuncp)
{
  Init();
  if (!sp)
  {
    BuildCode();
    return;
  }

  int i, j, k;
  char*s = FFaMathString::CopyStr(sp);
#if FFA_DEBUG > 1
  printf("FFaMathExpr: Original expression: %s\n",s);
#endif

  bool simplify = true;
  while (simplify) {
    s = FFaMathString::SimplifyStr(s); // fjerner parenteser og tomme tegn foerst og sist
    if(!s) {
      BuildCode();
      return;
    }
    else if(!strcmp(s,"Error")) {
      BuildCode();
      delete[] s;
      return;
    }

    // fjerner : og ; foerst i strengen
    simplify = s[0] == ':' || s[0] == ';';
    if (simplify) memmove(s,s+1,strlen(s));
  }
#if FFA_DEBUG > 1
  printf("FFaMathExpr: Simplified: %s\n",s);
#endif

  // sjekker om strengen kun best�r av tall
  if(FFaMathString::IsNumeric(s)) {
    op = Num;         // setter operasjon lik nummerisk
    ValC = atof(s);   // setter ValC lik verdien til nummeret

    BuildCode();
    delete[] s;

    return;
  }

  // Hvis strengen er pi
  if(FFaMathString::IsPi(s) && strlen(s) == 2) {
    op = Num;
    ValC = 3.141592653589793238462643383279L;

    BuildCode();
    delete[] s;

    return;
  }

  // Hvis streng er variabel
  if(FFaMathString::IsVar(s, 0, nvarp, ppvarp))
    for(i = 0; i < nvarp; i++) // for alle variabler
      if(FFaMathString::EqStr(s,(*(ppvarp + i)) -> name)) {
	pvar = ppvarp[i];
	op = Var;

	BuildCode();            
	delete[] s;

	return;
      }
  // isolerer tall ved � legge til parenteser 
  s = FFaMathString::IsolateNumbers(s, nvarp, ppvarp, nfuncp, ppfuncp);
#if FFA_DEBUG > 1
  printf("FFaMathExpr: Isolating numbers: %s\n",s);
#endif

  if (nvarp > 0) {
    // isolerer variable ved � legge til parenteser
    s = FFaMathString::IsolateVars(s, nvarp, ppvarp);
#if FFA_DEBUG > 1
  printf("FFaMathExpr: Isolating variables: %s\n",s);
#endif
  }

  // legger inn ; mellom funksjon og parentesen til variablen,  og : mellom to funksjoner 
  for (k = 0; k < (int)strlen(s) && k >= 0; k++)
    if (s[k] == '(')
      k = FFaMathString::SearchCorOpenbracket(s, k);
    else if ((j = FFaMathString::IsFunction(s, k, nfuncp, ppfuncp))) {
      for (i = k+j; s[i] == ' '; i++);
      if(s[i]=='('){
	j = FFaMathString::SearchCorOpenbracket(s, i);
	if(j != -1) {
	  s = FFaMathString::InsStr(s, i, ';');
	  k = j + 1;
	}
      } else if(s[i] != ':' && s[i] != ';') {
	s = FFaMathString::InsStr(s, i, ':');
	k = i;
      }
    }

  s = FFaMathString::SupprSpaces(s);          // fjerner alle tomme tegn
#if FFA_DEBUG > 1
  printf("FFaMathExpr: Inserting \";\" and \":\": %s\n",s);
#endif

  i = FFaMathString::SearchOperator(s, Juxt); // finner eventuelle ','
  if (i != -1) {               // hvis ',' finnes deler opp strengen i to
    char* s1 = FFaMathString::CopyStr(s, 0, i - 1);
    char* s2 = FFaMathString::CopyStr(s, i + 1, strlen(s) - 1);
    op = Juxt;
    mmb1 = new FFaMathExpr(s1, nvarp, ppvarp, nfuncp, ppfuncp);
    mmb2 = new FFaMathExpr(s2, nvarp, ppvarp, nfuncp, ppfuncp);

    BuildCode();               // gj�r operasjonen s1, s2
    delete[] s;
    delete[] s1;
    delete[] s2;

    return;
  }

  i = FFaMathString::SearchOperator(s, Add);  // finner eventuelle '+'
  if (i != -1) {               // hvis '+' finnes deler opp strengen i to
    char* s1 = FFaMathString::CopyStr(s, 0, i - 1);
    char* s2 = FFaMathString::CopyStr(s, i + 1, strlen(s) - 1);
    op = Add;
    mmb1 = new FFaMathExpr(s1, nvarp, ppvarp, nfuncp, ppfuncp);
    mmb2 = new FFaMathExpr(s2, nvarp, ppvarp, nfuncp, ppfuncp);

    BuildCode();               // gj�r addisjonen s1 + s2
    delete[] s;
    delete[] s1;
    delete[] s2;

    return;
  }

  i = FFaMathString::SearchOperator(s, Sub);  // finner eventuelle '-'
  if (i != -1) {               // hvis '-' finnes deler opp strengen i to
    char* s1 = FFaMathString::CopyStr(s, 0, i - 1);
    char* s2 = FFaMathString::CopyStr(s, i + 1, strlen(s) - 1);
    op = Sub;
    mmb1 = new FFaMathExpr(s1, nvarp, ppvarp, nfuncp, ppfuncp);
    mmb2 = new FFaMathExpr(s2, nvarp, ppvarp, nfuncp, ppfuncp);

    BuildCode();               // gj�r "beregning" s1 - s2
    delete[] s;
    delete[] s1;
    delete[] s2;

    return;
  }

  if(s[0] == '-') {           // spesielt hvis strengen begynner med '-'
    char*s2 = FFaMathString::CopyStr(s, 1, strlen(s) - 1);
    op = Opp;
    mmb2 = new FFaMathExpr(s2, nvarp, ppvarp, nfuncp, ppfuncp);

    BuildCode();              // gj�r beregningen  -s2 (evt. 0 - s2)
    delete[] s;
    delete[] s2;

    return;
  }

  char flag = 1;

  /* Forl�kka kapsler inn funksjon, med en operand, med parenteser
   * f. eks blir sin;(x) til (sin;(x))
   */
  for (i = 0; i < (int)strlen(s) && i >= 0; i++)
    if (s[i] == '(')
      i = FFaMathString::SearchCorOpenbracket(s, i);
    else if ((j = FFaMathString::IsFunction(s, i, nfuncp, ppfuncp))) {
      for (k = i+j; s[k] == ' '; k++);
      if(s[k] == ';') {
	for (j = k; s[j] != '('; j++);
	j = FFaMathString::SearchCorOpenbracket(s, j);
	if(j != -1) {
	  s = FFaMathString::InsStr(s, j, ')');
	  s = FFaMathString::InsStr(s, i, '(');
	  i = j + 2;
	}
      } else if(s[k] == ':') {
	for(j = k; j < (int)strlen(s); j++)
	  if(s[j] == '(') {
	    j = FFaMathString::SearchCorOpenbracket(s, j);
	    break;
	  }

	if(j == -1)
	  break;

	for(j++; j < (int)strlen(s); j++) {
	  if(s[j] == '(') {
	    j = FFaMathString::SearchCorOpenbracket(s, j);
	    if(j == -1) {
	      flag = 0;
	      break;
	    }
	    continue;
	  }
	  if(FFaMathString::IsFunction(s, j, nfuncp, ppfuncp))
	    break;
	}

	if(flag == 0) { 
	  flag = 1;
	  break;
	}

	while (j > i&& s[j - 1] != ')')
	  j--;

	if(j <= i + 1)
	  break;

	// kapsler inn strengen
	s = FFaMathString::InsStr(s, i, '(');
	s = FFaMathString::InsStr(s, j + 1, ')');
	i = j + 1;
      }
    }

#if FFA_DEBUG > 1
  printf("FFaMathExpr: Inserting parantheses: %s\n",s);
#endif
 
  // legger til '*' mellom to operander  
  for(i = 0; i < (int)strlen(s) - 1; i++) 
    if(s[i] == ')' && s[i + 1] == '(')
      s = FFaMathString::InsStr(s, ++i, '*');
#if FFA_DEBUG > 1
  printf("FFaMathExpr: Inserting \"*\": %s\n",s);
#endif

  // hvis funksjon er omkapsla med paranteser
  int nchar = strlen(s) - 1;
  if (s[0] == '(' && FFaMathString::SearchCorOpenbracket(s, 0) == nchar) {
    // finner funksjonsnavn, setter operasjon og operand for funksjoner med en operand
    if ((j = FFaMathString::GetFunction(s,1,op)) < 1)
      for (i = 0; i < nfuncp; i++)
        if (FFaMathString::CompStr(s,1,ppfuncp[i]->name) && j < (int)strlen(ppfuncp[i]->name)) {
          op = Fun;
          pfunc = ppfuncp[i];
          j = strlen(pfunc->name);
        }

    if (j > 0) {
      char* s2 = FFaMathString::CopyStr(s, 1+j, nchar-1);
#if FFA_DEBUG > 1
      if (s2) printf("FFaMathExpr: Function operand: %s\n",s2);
#endif
      mmb2 = new FFaMathExpr(s2, nvarp, ppvarp, nfuncp, ppfuncp);
      delete[] s2;
    }

    if (op == Fun && mmb2 != NULL && mmb2->NMembers() != pfunc->nvars) {
      delete mmb2;
      mmb2 = NULL;
      op = ErrOp;
    }
  }
 
  // hvis '*' finnes deler opp strengen i to
  else if ((i = FFaMathString::SearchOperator(s,Mult)) != -1) {
    char* s1 = FFaMathString::CopyStr(s, 0, i-1);
    char* s2 = FFaMathString::CopyStr(s, i+1, nchar);

    op   = Mult;
    mmb1 = new FFaMathExpr(s1, nvarp, ppvarp, nfuncp, ppfuncp);
    mmb2 = new FFaMathExpr(s2, nvarp, ppvarp, nfuncp, ppfuncp);

    delete[] s1;
    delete[] s2;
  }

  // hvis '/' finnes deler opp strengen i to
  else if ((i = FFaMathString::SearchOperator(s,Div)) != -1) {
    char* s1 = FFaMathString::CopyStr(s, 0, i-1);
    char* s2 = FFaMathString::CopyStr(s, i+1, nchar);

    op   = Div;
    mmb1 = new FFaMathExpr(s1, nvarp, ppvarp, nfuncp, ppfuncp);
    mmb2 = new FFaMathExpr(s2, nvarp, ppvarp, nfuncp, ppfuncp);

    delete[] s1;
    delete[] s2;
  }

  // hvis '%' finnes deler opp strengen i to
  else if ((i = FFaMathString::SearchOperator(s,Mod)) != -1) {
    char* s1 = FFaMathString::CopyStr(s, 0, i-1);
    char* s2 = FFaMathString::CopyStr(s, i+1, nchar);

    op   = Mod;
    mmb1 = new FFaMathExpr(s1, nvarp, ppvarp, nfuncp, ppfuncp);
    mmb2 = new FFaMathExpr(s2, nvarp, ppvarp, nfuncp, ppfuncp);

    delete[] s1;
    delete[] s2;
  }

  // hvis '^' finnes deler opp strengen i to
  else if ((i = FFaMathString::SearchOperator(s,Pow)) != -1) {
    char* s1 = FFaMathString::CopyStr(s, 0, i-1);
    char* s2 = FFaMathString::CopyStr(s, i+1, nchar);

    op   = Pow;
    mmb1 = new FFaMathExpr(s1, nvarp, ppvarp, nfuncp, ppfuncp);
    mmb2 = new FFaMathExpr(s2, nvarp, ppvarp, nfuncp, ppfuncp);

    delete[] s1;
    delete[] s2;
  }

  // hvis '#' finnes deler opp strengen i to
  else if ((i = FFaMathString::SearchOperator(s,NthRoot)) != -1) {
    char* s1 = FFaMathString::CopyStr(s, 0, i-1);
    char* s2 = FFaMathString::CopyStr(s, i+1, nchar);

    if (i == 0 || s[i-1] != ')') // ingen spesifising medf�rer kvadratrot
      op = Sqrt;
    else {
      op = NthRoot;
      mmb1 = new FFaMathExpr(s1, nvarp, ppvarp, nfuncp, ppfuncp);
    }

    mmb2 = new FFaMathExpr(s2, nvarp, ppvarp, nfuncp, ppfuncp);

    delete[] s1;
    delete[] s2;
  }

  // hvis 'E' finnes deler opp strengen i to
  else if ((i = FFaMathString::SearchOperator(s,E10)) != -1) {
    char* s1 = FFaMathString::CopyStr(s, 0, i-1);
    char* s2 = FFaMathString::CopyStr(s, i+1, nchar);

    op   = E10;
    mmb1 = new FFaMathExpr(s1, nvarp, ppvarp, nfuncp, ppfuncp);
    mmb2 = new FFaMathExpr(s2, nvarp, ppvarp, nfuncp, ppfuncp);

    delete[] s1;
    delete[] s2;
  }

  else if ((i = FFaMathString::SearchOperator(s,LogicalLessOrEqual)) != -1) {
    char *s1, *s2;
    if(s[i+1] == '=') {
      s1 = FFaMathString::CopyStr(s, 0, i-1);
      s2 = FFaMathString::CopyStr(s, i+2, nchar);
      op = LogicalLessOrEqual;
    } else {
      s1 = FFaMathString::CopyStr(s, 0, i-1);
      s2 = FFaMathString::CopyStr(s, i+1, nchar);
      op = LogicalLess;
    }

    mmb1 = new FFaMathExpr(s1, nvarp, ppvarp, nfuncp, ppfuncp);
    mmb2 = new FFaMathExpr(s2, nvarp, ppvarp, nfuncp, ppfuncp);

    delete[] s1;
    delete[] s2;
  }

  else if ((i = FFaMathString::SearchOperator(s,LogicalGreaterOrEqual)) != -1) {
    char *s1, *s2;
    if(s[i+1] == '=') {
      s1 = FFaMathString::CopyStr(s, 0, i-1);
      s2 = FFaMathString::CopyStr(s, i+2, nchar);
      op = LogicalGreaterOrEqual;
    } else {
      s1 = FFaMathString::CopyStr(s, 0, i-1);
      s2 = FFaMathString::CopyStr(s, i+1, nchar);
      op = LogicalGreater;
    }

    mmb1 = new FFaMathExpr(s1, nvarp, ppvarp, nfuncp, ppfuncp);
    mmb2 = new FFaMathExpr(s2, nvarp, ppvarp, nfuncp, ppfuncp);

    delete[] s1;
    delete[] s2;
  }

  else if ((i = FFaMathString::SearchOperator(s,LogicalAnd)) != -1) {
    char* s1 = FFaMathString::CopyStr(s, 0, i-2);
    char* s2 = FFaMathString::CopyStr(s, i+1, nchar);

    op   = LogicalAnd;
    mmb1 = new FFaMathExpr(s1, nvarp, ppvarp, nfuncp, ppfuncp);
    mmb2 = new FFaMathExpr(s2, nvarp, ppvarp, nfuncp, ppfuncp);

    delete[] s1;
    delete[] s2;
  }

  else if ((i = FFaMathString::SearchOperator(s,LogicalOr)) != -1) {
    char* s1 = FFaMathString::CopyStr(s, 0, i-2);
    char* s2 = FFaMathString::CopyStr(s, i+1, nchar);

    op   = LogicalOr;
    mmb1 = new FFaMathExpr(s1, nvarp, ppvarp, nfuncp, ppfuncp);
    mmb2 = new FFaMathExpr(s2, nvarp, ppvarp, nfuncp, ppfuncp);

    delete[] s1;
    delete[] s2;
  }

  else if ((i = FFaMathString::SearchOperator(s,LogicalEqual)) != -1) {
    char* s1 = FFaMathString::CopyStr(s, 0, i-2);
    char* s2 = FFaMathString::CopyStr(s, i+1, nchar);

    op   = LogicalEqual;
    mmb1 = new FFaMathExpr(s1, nvarp, ppvarp, nfuncp, ppfuncp);
    mmb2 = new FFaMathExpr(s2, nvarp, ppvarp, nfuncp, ppfuncp);

    delete[] s1;
    delete[] s2;
  }

  else if ((i = FFaMathString::SearchOperator(s,LogicalNotEqual)) != -1) {
    char* s1 = FFaMathString::CopyStr(s, 0, i-2);
    char* s2 = FFaMathString::CopyStr(s, i+1, nchar);

    op   = LogicalNotEqual;
    mmb1 = new FFaMathExpr(s1, nvarp, ppvarp, nfuncp, ppfuncp);
    mmb2 = new FFaMathExpr(s2, nvarp, ppvarp, nfuncp, ppfuncp);

    delete[] s1;
    delete[] s2;
  }

  delete[] s;
  BuildCode(); // gjoer beregning paa gitt operasjon og operand
}


/*!
  \fn FFaMathExpr::Destroy()
  \brief Destructor of class FFaMathExpr.
*/
void FFaMathExpr::Destroy()
{
  if(mmb1 != NULL && mmb2 != NULL && mmb1 != mmb2) {
    delete mmb1;
    delete mmb2;
    mmb1 = NULL;
    mmb2 = NULL;

  } else if(mmb1 != NULL) {
    delete mmb1;
    mmb1 = NULL;

  } else if(mmb2 != NULL) {
    delete mmb2;
    mmb2 = NULL;
  }

  if(pinstr != NULL) {
    delete[]pinstr;
    pinstr = NULL;
  }

  if(pvals != NULL) {
    if (op == ErrOp || op == Num)
      delete pvals[0];
    delete[]pvals;
    pvals = NULL;
  }

  if(ppile != NULL) {
    delete[]ppile;
    ppile = NULL;
  }

  if(pfuncpile != NULL) {
    delete[]pfuncpile;
    pfuncpile = NULL;
  }
}


/*!
  \fn FFaMathExpr::ContainVar(const FFaMathVar&) const
  \brief Checks for a variable in the expression.
  \param[in] varp The FFaMathVar object to check for existance of
*/
bool FFaMathExpr::ContainVar(const FFaMathVar& varp) const
{
  if (op == Var)
    return (*pvar == varp);

  if (mmb1 != NULL && mmb1->ContainVar(varp))
    return true;

  if (mmb2 != NULL && mmb2->ContainVar(varp))
    return true;

  return false;
}

/*!
  \fn FFaMathExpr::ContainFuncNoRec(const FFaMathFunction&) const
  \brief Checks if the operation is a sub-function.
  \param[in] func The FFaMathFunction object to check for
*/
bool FFaMathExpr::ContainFuncNoRec(const FFaMathFunction& func) const
{
  if (op == Fun)
    return (*pfunc == func);

  if (mmb1 != NULL && mmb1->ContainFuncNoRec(func))
    return true;

  if (mmb2 != NULL && mmb2->ContainFuncNoRec(func))
    return true;

  return false;
}

/*!
  \fn FFaMathExpr::ContainFunc(const FFaMathFunction&) const
  \brief Recursive check for a sub-function.
  \param[in] func The FFaMathFunction object to check for
*/
bool FFaMathExpr::ContainFunc(const FFaMathFunction& func) const
{
  if (containfuncflag)
    return false;

  if (op == Fun && *pfunc == func)
    return true;

  containfuncflag = true;
  bool containFunc = false;
  if (op == Fun && pfunc->op.ContainFunc(func))
    containFunc = true;
  else if (mmb1 != NULL && mmb1->ContainFunc(func))
    containFunc = true;
  else if (mmb2 != NULL && mmb2->ContainFunc(func))
    containFunc = true;

  containfuncflag = false;
  return containFunc;
}

/*!
  \fn FFaMathExpr::HasError(const FFaMathExpr*) const
  \brief Checks if this expression contains errors.
*/
bool FFaMathExpr::HasError(const FFaMathExpr* pop) const
{ 
  if (op == ErrOp)
    return true;

  if (op == Fun && pfunc->type == 1)
  {
    if (pfunc->op == *(pop == NULL ? this : pop))
      return true;
    else if (pfunc->op.HasError(pop == NULL ? this : pop))
      return true;
  }

  if (mmb1 != NULL && mmb1->HasError(pop == NULL ? this : pop))
    return true;

  if (mmb2 != NULL && mmb2->HasError(pop == NULL ? this : pop))
    return true;

  return (op == Fun && pfunc->type == -1);
}

/*!
  \fn FFaMathExpr::NMembers() const
  \brief Returns the number of members of an operation.
*/
int FFaMathExpr::NMembers() const
{
  if(op == Fun)
    return pfunc->type == 1 ? pfunc->op.NMembers() : (pfunc->type == 0 ? 1 : 0);

  if(op != Juxt)
    return 1;
  else if(mmb2 == NULL)
    return 0;
  else 
    return 1 + mmb2 -> NMembers();
}

/*!
  \fn FFaMathExpr::NthMember(int) const
  \brief Returns the n'th member of an operation.
  \param[in] n Index of the member to return
*/
FFaMathExpr FFaMathExpr::NthMember(int n) const
{
  if(op == Fun && pfunc -> type == 1 && pfunc -> op.NMembers() > 1) {
    FFaMathFunction* prf = new FFaMathFunction(pfunc->op.NthMember(n), pfunc->nvars, pfunc->ppvar);
    size_t nchar = strlen(pfunc->name) + 10;
    char* s = new char[nchar];
    snprintf(s, nchar, "(%s_%i)", pfunc->name, n);
    prf -> SetName(s);
    delete[] s;

    return(*prf)(*mmb2);
  }

  else if (n == 1) {
    if (op != Juxt)
      return *this; 
    else if (mmb1 != NULL)
      return *mmb1;
  }

  else if (op == Juxt && n > 1 && mmb2 != NULL)
    return mmb2 -> NthMember(n - 1);

  return FFaMathOps::ErrVal;
}

/*!
  \fn FFaMathExpr::Substitute(const FFaMathVar&,const FFaMathExpr&) const
  \brief Replaces the variable \a var with the expression \a rop.
  \param[in] var The variable to be replaced
  \param[in] rop The expression to be inserted in place of the variable
*/
FFaMathExpr FFaMathExpr::Substitute(const FFaMathVar& var,
                                    const FFaMathExpr& rop) const
{
  if(!ContainVar(var))
    return *this;

  if(op == Var)
    return rop;

  FFaMathExpr r;
  r.op = op;
  r.pvar = pvar;
  r.ValC = ValC;
  r.pfunc = pfunc;

  if(mmb1 != NULL)
    r.mmb1 = new FFaMathExpr(mmb1 -> Substitute(var, rop));

  if(mmb2 != NULL)
    r.mmb2 = new FFaMathExpr(mmb2 -> Substitute(var, rop));

  return r;
}

/*!
  \fn FFaMathExpr::Diff(const FFaMathVar&) const
  \brief Differentiate the expression.
  \param[in] var The FFaMathVar object to differentiate with respect to
  \return A tree of objects of the differentiated expression
*/
FFaMathExpr FFaMathExpr::Diff(const FFaMathVar& var) const
{
  if (!ContainVar(var))
    return 0.0;

  FFaMathExpr rop;

  switch (op) {
  case Var:
    return 1.0;

  case Juxt:
    rop.op = Juxt;
    rop.mmb1 = new FFaMathExpr(mmb1->Diff(var));
    rop.mmb2 = new FFaMathExpr(mmb2->Diff(var));
    return rop;

  case Add:
    return (mmb1->Diff(var) + mmb2->Diff(var));

  case Sub:
    return (mmb1->Diff(var) - mmb2->Diff(var));

  case Opp:
    return (-mmb2->Diff(var));

  case Mult:
    return ((*mmb1)*(mmb2->Diff(var)) + (*mmb2)*(mmb1->Diff(var)));

  case Div:
    if (mmb2->ContainVar(var))
      return (((*mmb2)*(mmb1->Diff(var)) - (*mmb1)*(mmb2->Diff(var))) / ((*mmb2) ^ FFaMathExpr(2.0)));
    else
      return (mmb1->Diff(var) / (*mmb2));

  case Pow:
    if (mmb2->ContainVar(var)) {
      rop.op = Log;
      rop.mmb2 = new FFaMathExpr(*mmb1);
      return ((*this)*(rop*mmb2->Diff(var) + (*mmb2)*mmb1->Diff(var) / (*mmb1)));
    }
    else
      return (*mmb2)*(mmb1->Diff(var))*((*mmb1) ^ ((*mmb2) - FFaMathExpr(1.0)));

  case Sqrt:
    rop.op = Sqrt;
    rop.mmb2 = new FFaMathExpr(*mmb2);
    return (mmb2->Diff(var) / (FFaMathExpr(2.0) * rop));

  case NthRoot:
    rop = (*mmb2) ^ (FFaMathExpr(1.0) / (*mmb1));
    return rop.Diff(var);

  case E10:
    rop = (*mmb1) * (FFaMathExpr(10.0) ^ (*mmb2));
    return rop.Diff(var);

  case Ln:
    return (mmb2->Diff(var) / (*mmb2));

  case Log:
    return (mmb2->Diff(var) * FFaMathExpr(log10(exp(1.0))) / (*mmb2));

  case Exp:
    return (mmb2->Diff(var) * (*this));

  case Sin:
    rop.op = Cos;
    rop.mmb2 = new FFaMathExpr(*mmb2);
    return (mmb2->Diff(var) * rop);

  case Cos:
    rop.op = Sin;
    rop.mmb2 = new FFaMathExpr(*mmb2);
    return (-mmb2->Diff(var) * rop);

  case Tg:
    return (mmb2->Diff(var) * (FFaMathExpr(1.0) + ((*this) ^ FFaMathExpr(2.0))));

  case Atan:
    rop = FFaMathExpr(2.0);
    if (mmb2->op != Juxt)
      return (mmb2->Diff(var) / (FFaMathExpr(1.0) + ((*mmb2) ^ rop)));
    else 
      return ((mmb2->NthMember(1).Diff(var)) * (mmb2->NthMember(2)) -
	      (mmb2->NthMember(2).Diff(var)) * (mmb2->NthMember(1))) /
	    (((mmb2->NthMember(1)) ^ rop) + ((mmb2->NthMember(2)) ^ rop));

  case Asin:
    rop.op = Sqrt;
    rop.mmb2 = new FFaMathExpr(FFaMathExpr(1.0) - ((*mmb2) ^ FFaMathExpr(2.0)));
    return (mmb2->Diff(var) / rop);

  case Acos:
    rop.op = Sqrt;
    rop.mmb2 = new FFaMathExpr(FFaMathExpr(1.0) - ((*mmb2) ^ FFaMathExpr(2.0)));
    return (-mmb2->Diff(var) / rop);

  case Abs:
    return (mmb2->Diff(var) * (*mmb2) / (*this));

  default:
    return FFaMathOps::ErrVal;
  }
}


/*!
  \fn FFaMathExpr::Val() const
  \brief Returns the value of the total expression.
*/
double FFaMathExpr::Val() const
{
  DoublePtRefFuncType* p1 = pinstr;    // liste av funksjoner, variable, operasjoner
  double**             p2 = pvals;     // verdi til "noden" p1
  double*              p3 = ppile - 1; // beregna verdi til funksjonen
  FFaMathFunction**    p4 = pfuncpile; // benyttes ved bruk av funksjon f(*)

  for (; *p1 != NULL; p1++) // saa lenge det finnes noder aa beregne paa
    if (*p1 == &FFaMathOps::NextVal)
      *(++p3) = **(p2++);
    else if (*p1 == &FFaMathOps::RFunc) { // hvis det som skal beregnes er en funksjon f(*)
      FFaMathFunction* rf = *(p4++);
      p3 -= rf->nvars - 1;
      *p3 = rf->Val(p3);
    }
    else // hvis ikke en funksjon utfoeres dette
      (**p1)(p3);

  return *p3; // returnerer beregna verdi
}

/*!
  \fn FFaMathExpr::BCDouble(DoublePtRefFuncType*&,DoublePtRefFuncType*,
                            DoublePtRefFuncType*,
                            double**&,double**,double**,
                            double*&, double*, double*,
                            FFaMathFunction**&,FFaMathFunction**,
                            FFaMathFunction**, DoublePtRefFuncType)
  \brief Sets a lot of parameters when the function has two operands.
*/
void FFaMathExpr::BCDouble(DoublePtRefFuncType*& pf, DoublePtRefFuncType* pf1,
                           DoublePtRefFuncType* pf2,
                           double**& pv, double** pv1, double** pv2,
                           double*&  pp, double*  pp1, double*  pp2,
                           FFaMathFunction**& prf, FFaMathFunction** prf1,
                           FFaMathFunction** prf2, DoublePtRefFuncType f)
{
  unsigned long int nval;
  DoublePtRefFuncType *pf3, *pf4;
  double **pv3, **pv4;
  FFaMathFunction **prf3, **prf4;

  nval = 2;
  for(pf4 = pf1; *pf4 != NULL; pf4++, nval++);
  for(pf4 = pf2; *pf4 != NULL; pf4++, nval++);
  pf3 = pf = new DoublePtRefFuncType[nval];

  for(pf4 = pf1; *pf4 != NULL; pf3++, pf4++)
    *pf3 = *pf4;

  for(pf4 = pf2; *pf4 != NULL; pf3++, pf4++)
    *pf3 = *pf4;

  *pf3++ = f;
  *pf3 = NULL; 

  nval = 1;
  for(pv4 = pv1; *pv4 != NULL; pv4++, nval++);
  for(pv4 = pv2; *pv4 != NULL; pv4++, nval++);
  pv3 = pv = new double*[nval];

  for(pv4 = pv1; *pv4 != NULL; pv3++, pv4++)
    *pv3 = *pv4;

  for(pv4 = pv2; *pv4 != NULL; pv3++, pv4++)
    *pv3 = *pv4;

  *pv3 = NULL;

  nval = 0;
  for(; *pp1 != FFaMathOps::ErrVal; pp1++, nval++);
  for(; *pp2 != FFaMathOps::ErrVal; pp2++, nval++);
  pp = new double[nval+1];

  memset(pp,0,nval*sizeof(double));
  pp[nval] = FFaMathOps::ErrVal;

  nval = 1;
  for(prf4 = prf1; *prf4 != NULL; prf4++, nval++);
  for(prf4 = prf2; *prf4 != NULL; prf4++, nval++);
  prf3 = prf = new FFaMathFunction*[nval];

  for(prf4 = prf1; *prf4 != NULL; prf3++, prf4++)
    *prf3 = *prf4;

  for(prf4 = prf2; *prf4 != NULL; prf3++, prf4++)
    *prf3 = *prf4;

  *prf3 = NULL;
}

/*!
  \fn FFaMathExpr::BCSimple(DoublePtRefFuncType*&,DoublePtRefFuncType*,
                            double**&,double**,double*&,double*,
                            FFaMathFunction**&,FFaMathFunction**,
                            DoublePtRefFuncType)
  \brief Sets a lot of parameters when the function has two operands.
*/
void FFaMathExpr::BCSimple(DoublePtRefFuncType*& pf, DoublePtRefFuncType* pf1,
                           double**& pv, double** pv1, double*& pp, double* pp1,
                           FFaMathFunction**& prf, FFaMathFunction** prf1,
                           DoublePtRefFuncType f)
{
  unsigned long int nval;
  DoublePtRefFuncType *pf3, *pf4;
  double **pv3, **pv4;
  FFaMathFunction **prf3, **prf4;

  for(nval = 2, pf4 = pf1; *pf4 != NULL; pf4++, nval++);
  pf = new DoublePtRefFuncType[nval];

  for(pf4 = pf1, pf3 = pf; *pf4 != NULL; pf3++, pf4++)
    *pf3 = *pf4;

  *pf3++ = f;
  *pf3 = NULL;

  for(nval = 1, pv4 = pv1; *pv4 != NULL; pv4++, nval++);
  pv = new double*[nval];

  for(pv3 = pv, pv4 = pv1; *pv4 != NULL; pv3++, pv4++)
    *pv3 = *pv4;

  *pv3 = NULL;

  for(nval = 0; *pp1 != FFaMathOps::ErrVal; pp1++, nval++);
  pp = new double[nval+1];

  memset(pp,0,nval*sizeof(double));
  pp[nval] = FFaMathOps::ErrVal;

  for(nval = 1, prf4 = prf1; *prf4 != NULL; prf4++, nval++);
  prf = new FFaMathFunction*[nval];

  for(prf3 = prf, prf4 = prf1; *prf4 != NULL; prf3++, prf4++)
    *prf3 = *prf4;

  *prf3 = NULL;
}

/*!
  \fn FFaMathExpr::BCFun(DoublePtRefFuncType*&,DoublePtRefFuncType*,
                         double**&,double**,double*&,double*,
                         FFaMathFunction**&,FFaMathFunction**,FFaMathFunction*)
  \brief Sets a lot of parameters when the operation is a function.
*/
void FFaMathExpr::BCFun(DoublePtRefFuncType*& pf, DoublePtRefFuncType* pf1,
                        double**& pv, double** pv1, double*& pp, double* pp1,
                        FFaMathFunction**& prf, FFaMathFunction** prf1,
                        FFaMathFunction* rf)
{
  unsigned long int nval;
  DoublePtRefFuncType *pf3, *pf4;
  double **pv3, **pv4;
  FFaMathFunction **prf3, **prf4;
 
  for(nval = 2, pf4 = pf1; *pf4 != NULL; pf4++, nval++);
  pf = new DoublePtRefFuncType[nval];

  for(pf3 = pf, pf4 = pf1; *pf4 != NULL; pf3++, pf4++)
    *pf3 = *pf4;

  *pf3++ = &FFaMathOps::RFunc;
  *pf3 = NULL;

  for(nval = 1, pv4 = pv1; *pv4 != NULL; pv4++, nval++);
  pv = new double*[nval];

  for(pv3 = pv, pv4 = pv1; *pv4 != NULL; pv3++, pv4++)
    *pv3 = *pv4;

  *pv3 = NULL;

  for(nval = 0; *pp1 != FFaMathOps::ErrVal; pp1++, nval++);
  pp = new double[nval+1];

  memset(pp,0,nval*sizeof(double));
  pp[nval] = FFaMathOps::ErrVal;

  for(nval = 2, prf4 = prf1; *prf4 != NULL; prf4++, nval++);
  prf = new FFaMathFunction*[nval];

  for(prf3 = prf, prf4 = prf1; *prf4 != NULL; prf3++, prf4++)
    *prf3 = *prf4;

  *prf3++ = rf;
  *prf3 = NULL;
}

/*!
  \fn FFaMathExpr::BuildCode()
  \brief Builds a tree of variables, numbers, operations and functions.
*/
void FFaMathExpr::BuildCode()
{
  switch (op) {
  case ErrOp:
  case Num:
  case Var:
    pinstr = new DoublePtRefFuncType[2];
    pinstr[0] = &FFaMathOps::NextVal;
    pinstr[1] = NULL;

    pvals = new double*[2];
    if (op == Num)
      pvals[0] = new double(ValC);
    else if (op == Var)
      pvals[0] = const_cast<double*>(pvar->ptr());
    else
      pvals[0] = new double(FFaMathOps::ErrVal);
    pvals[1] = NULL;

    ppile = new double[2];
    ppile[0] = 0.0;
    ppile[1] = FFaMathOps::ErrVal;

    pfuncpile = new FFaMathFunction*[1];
    pfuncpile[0] = NULL;
    break;

  case Juxt:
    FFaMathExpr::BCDouble(pinstr, mmb1 -> pinstr, mmb2 -> pinstr, pvals, 
				 mmb1 -> pvals, mmb2 -> pvals, ppile, mmb1 -> ppile, 
				 mmb2 -> ppile, pfuncpile, mmb1 -> pfuncpile, 
				 mmb2 -> pfuncpile, &FFaMathOps::JuxtF);
    break;

  case Add:
    FFaMathExpr::BCDouble(pinstr, mmb1 -> pinstr, mmb2 -> pinstr, pvals, 
				 mmb1 -> pvals, mmb2 -> pvals, ppile, mmb1 -> ppile, 
				 mmb2 -> ppile, pfuncpile, mmb1 -> pfuncpile, 
				 mmb2 -> pfuncpile, &FFaMathOps::Addition);
    break;

  case Sub:
    FFaMathExpr::BCDouble(pinstr, mmb1 -> pinstr, mmb2 -> pinstr, pvals, 
				 mmb1 -> pvals, mmb2 -> pvals, ppile, mmb1 -> ppile, 
				 mmb2 -> ppile, pfuncpile, mmb1 -> pfuncpile, 
				 mmb2 -> pfuncpile, &FFaMathOps::Subtraction);
    break;
    
  case Mult:
    FFaMathExpr::BCDouble(pinstr, mmb1 -> pinstr, mmb2 -> pinstr, pvals, 
				 mmb1 -> pvals, mmb2 -> pvals, ppile, mmb1 -> ppile, 
				 mmb2 -> ppile, pfuncpile, mmb1 -> pfuncpile, 
				 mmb2 -> pfuncpile, &FFaMathOps::Multiplication);
    break;

  case Div:
    FFaMathExpr::BCDouble(pinstr, mmb1 -> pinstr, mmb2 -> pinstr, pvals, 
				 mmb1 -> pvals, mmb2 -> pvals, ppile, mmb1 -> ppile, 
				 mmb2 -> ppile, pfuncpile, mmb1 -> pfuncpile, 
				 mmb2 -> pfuncpile, &FFaMathOps::Division);
    break;

  case Mod:
    FFaMathExpr::BCDouble(pinstr,    mmb1->pinstr,    mmb2->pinstr,
                          pvals,     mmb1->pvals,     mmb2->pvals,
                          ppile,     mmb1->ppile,     mmb2->ppile,
                          pfuncpile, mmb1->pfuncpile, mmb2->pfuncpile,
                          &FFaMathOps::Modulus);
    break;

  case Max:
    FFaMathExpr::BCSimple(pinstr, mmb2->pinstr, pvals, mmb2->pvals,
                          ppile,  mmb2->ppile,  pfuncpile, mmb2->pfuncpile,
                          &FFaMathOps::Max);
    break;

  case Min:
    FFaMathExpr::BCSimple(pinstr, mmb2->pinstr, pvals, mmb2->pvals,
                          ppile,  mmb2->ppile,  pfuncpile, mmb2->pfuncpile,
                          &FFaMathOps::Min);
    break;

  case Pow:
    FFaMathExpr::BCDouble(pinstr, mmb1 -> pinstr, mmb2 -> pinstr, pvals, 
				 mmb1 -> pvals, mmb2 -> pvals, ppile, mmb1 -> ppile, 
				 mmb2 -> ppile, pfuncpile, mmb1 -> pfuncpile, 
				 mmb2 -> pfuncpile, &FFaMathOps::Puissance);
    break;

  case NthRoot:
    FFaMathExpr::BCDouble(pinstr, mmb1 -> pinstr, mmb2 -> pinstr, pvals, 
				 mmb1 -> pvals, mmb2 -> pvals, ppile, mmb1 -> ppile, 
				 mmb2 -> ppile, pfuncpile, mmb1 -> pfuncpile, 
				 mmb2 -> pfuncpile, &FFaMathOps::RacineN);
    break;

  case E10:
    FFaMathExpr::BCDouble(pinstr, mmb1 -> pinstr, mmb2 -> pinstr, pvals, 
				 mmb1 -> pvals, mmb2 -> pvals, ppile, mmb1 -> ppile, 
				 mmb2 -> ppile, pfuncpile, mmb1 -> pfuncpile, 
				 mmb2 -> pfuncpile, &FFaMathOps::Puiss10);
    break;

  case Opp:
    FFaMathExpr::BCSimple(pinstr, mmb2 -> pinstr, pvals, mmb2 -> pvals, ppile, 
	     mmb2 -> ppile, pfuncpile, mmb2 -> pfuncpile, &FFaMathOps::Oppose);
    break;

  case Sin:
    FFaMathExpr::BCSimple(pinstr, mmb2 -> pinstr, pvals, mmb2-> pvals, ppile,
	     mmb2 -> ppile, pfuncpile, mmb2 -> pfuncpile, &FFaMathOps::Sinus);
    break;

  case Sqrt:
    FFaMathExpr::BCSimple(pinstr, mmb2 -> pinstr, pvals, mmb2 -> pvals, ppile,
	     mmb2 -> ppile, pfuncpile, mmb2 -> pfuncpile, &FFaMathOps::Racine);
    break;

  case Log:
    FFaMathExpr::BCSimple(pinstr, mmb2 -> pinstr, pvals, mmb2 -> pvals, ppile, 
	     mmb2 -> ppile, pfuncpile, mmb2 -> pfuncpile, &FFaMathOps::Logarithme);
    break;

  case Ln:
    FFaMathExpr::BCSimple(pinstr, mmb2 -> pinstr, pvals, mmb2 -> pvals, ppile, 
	     mmb2 -> ppile, pfuncpile, mmb2 -> pfuncpile, &FFaMathOps::NaturalLogarithme);
    break;
  
  case Exp:
    FFaMathExpr::BCSimple(pinstr, mmb2 -> pinstr, pvals, mmb2 -> pvals, ppile,
	     mmb2 -> ppile, pfuncpile ,mmb2 -> pfuncpile, &FFaMathOps::Exponentielle);
  break;

  case Cos:
    FFaMathExpr::BCSimple(pinstr, mmb2 -> pinstr, pvals, mmb2 -> pvals, ppile,
	     mmb2 -> ppile, pfuncpile, mmb2 -> pfuncpile, &FFaMathOps::Cosinus);
    break;

  case Tg:
    FFaMathExpr::BCSimple(pinstr, mmb2 -> pinstr, pvals, mmb2 -> pvals, ppile,
	     mmb2 -> ppile, pfuncpile, mmb2 -> pfuncpile, &FFaMathOps::Tangente);
    break;
    
  case Atan:
    FFaMathExpr::BCSimple(pinstr, mmb2 -> pinstr, pvals, mmb2 -> pvals, ppile,
	      mmb2 -> ppile, pfuncpile, mmb2 -> pfuncpile, 
	      (mmb2 -> NMembers() > 1 ? &FFaMathOps::ArcTangente2 : 
	      &FFaMathOps::ArcTangente));
    break;
    
  case Asin:
    FFaMathExpr::BCSimple(pinstr, mmb2 -> pinstr, pvals, mmb2 -> pvals, ppile,
	     mmb2 -> ppile, pfuncpile, mmb2 -> pfuncpile, &FFaMathOps::ArcSinus);
    break;
    
  case Acos:
    FFaMathExpr::BCSimple(pinstr, mmb2 -> pinstr, pvals, mmb2 -> pvals, ppile,
	     mmb2 -> ppile, pfuncpile, mmb2 -> pfuncpile, &FFaMathOps::ArcCosinus);
    break;
    
  case Abs:
    FFaMathExpr::BCSimple(pinstr, mmb2 -> pinstr, pvals, mmb2 -> pvals, ppile,
	     mmb2 -> ppile, pfuncpile, mmb2 -> pfuncpile, &FFaMathOps::Absolu);
    break;
    
  case Fun:
    FFaMathExpr::BCFun(pinstr, mmb2 -> pinstr, pvals, mmb2 -> pvals, ppile, 
	  mmb2 -> ppile, pfuncpile, mmb2 -> pfuncpile, pfunc);
    break;

  case LogicalLess:
    FFaMathExpr::BCDouble(pinstr, mmb1 -> pinstr, mmb2 -> pinstr, pvals, 
				 mmb1 -> pvals, mmb2 -> pvals, ppile, mmb1 -> ppile, 
				 mmb2 -> ppile, pfuncpile, mmb1 -> pfuncpile, 
				 mmb2 -> pfuncpile, &FFaMathOps::LessThan);
    break;

  case LogicalGreater:
    FFaMathExpr::BCDouble(pinstr, mmb1 -> pinstr, mmb2 -> pinstr, pvals, 
				 mmb1 -> pvals, mmb2 -> pvals, ppile, mmb1 -> ppile, 
				 mmb2 -> ppile, pfuncpile, mmb1 -> pfuncpile, 
				 mmb2 -> pfuncpile, &FFaMathOps::GreaterThan);
    break;    

  case LogicalAnd:
    FFaMathExpr::BCDouble(pinstr, mmb1 -> pinstr, mmb2 -> pinstr, pvals, 
				 mmb1 -> pvals, mmb2 -> pvals, ppile, mmb1 -> ppile, 
				 mmb2 -> ppile, pfuncpile, mmb1 -> pfuncpile, 
				 mmb2 -> pfuncpile, &FFaMathOps::BooleanAnd);
    break;    

  case LogicalOr:
    FFaMathExpr::BCDouble(pinstr, mmb1 -> pinstr, mmb2 -> pinstr, pvals, 
				 mmb1 -> pvals, mmb2 -> pvals, ppile, mmb1 -> ppile, 
				 mmb2 -> ppile, pfuncpile, mmb1 -> pfuncpile, 
				 mmb2 -> pfuncpile, &FFaMathOps::BooleanOr );
    break;    

  case LogicalEqual:
    FFaMathExpr::BCDouble(pinstr, mmb1 -> pinstr, mmb2 -> pinstr, pvals, 
				 mmb1 -> pvals, mmb2 -> pvals, ppile, mmb1 -> ppile, 
				 mmb2 -> ppile, pfuncpile, mmb1 -> pfuncpile, 
				 mmb2 -> pfuncpile, &FFaMathOps::BooleanEqual );
    break;    

  case LogicalNotEqual:
    FFaMathExpr::BCDouble(pinstr, mmb1 -> pinstr, mmb2 -> pinstr, pvals, 
				 mmb1 -> pvals, mmb2 -> pvals, ppile, mmb1 -> ppile, 
				 mmb2 -> ppile, pfuncpile, mmb1 -> pfuncpile, 
				 mmb2 -> pfuncpile, &FFaMathOps::BooleanNotEqual );
    break;    

  case LogicalLessOrEqual:
    FFaMathExpr::BCDouble(pinstr, mmb1 -> pinstr, mmb2 -> pinstr, pvals, 
				 mmb1 -> pvals, mmb2 -> pvals, ppile, mmb1 -> ppile, 
				 mmb2 -> ppile, pfuncpile, mmb1 -> pfuncpile, 
				 mmb2 -> pfuncpile, &FFaMathOps::BooleanLessOrEqual );
    break;    

  case LogicalGreaterOrEqual:
    FFaMathExpr::BCDouble(pinstr, mmb1 -> pinstr, mmb2 -> pinstr, pvals, 
				 mmb1 -> pvals, mmb2 -> pvals, ppile, mmb1 -> ppile, 
				 mmb2 -> ppile, pfuncpile, mmb1 -> pfuncpile, 
				 mmb2 -> pfuncpile, &FFaMathOps::BooleanGreaterOrEqual );
    break;

  case LogicalNot:
    FFaMathExpr::BCSimple(pinstr, mmb2 -> pinstr, pvals, mmb2 -> pvals, ppile,
	     mmb2 -> ppile, pfuncpile, mmb2 -> pfuncpile, &FFaMathOps::BooleanNot);
    break;    

  default:
    FFaMathExpr::BCSimple(pinstr, mmb2 -> pinstr, pvals, mmb2 -> pvals, ppile,
	     mmb2 -> ppile, pfuncpile, mmb2 -> pfuncpile, &FFaMathOps::FonctionError);
  }
}
