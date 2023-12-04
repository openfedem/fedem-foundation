// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#include "FFaMathString.H"
#include "FFaMathVar.H"
#include <string.h>
#include <ctype.h>


/*!
  \fn char* FFaMathString::CopyStr(const char* s, int i1, int i2)
  \brief The string between to indices is returned.

  \param s String
  \param i1 Start index
  \param i2 Stopp index

  \return The string between start index and stop index in a given string

  \details If \a i1 and \a i2 are both zero, the whole string is copied.
*/
char* FFaMathString::CopyStr(const char* s, int i1, int i2)
{
  if (i1 < 0 || !s)
    return 0;
  int n = strlen(s);
  if (i2 <= 0 || i2 >= n)
    i2 = n-1;
  if (i1 > i2)
    return 0;

  n = i2 - i1 + 1;
  char* s1 = new char[n+1];
  strncpy(s1,s+i1,n);
  s1[n] = 0;

  return s1;
}


/*!
  \fn char* FFaMathString::InsStr(const char*s, int n, char c)
  \brief A character is inserted into string in a given position.

  \param s String
  \param n Position index
  \param c Character to be inserted

  \return The new string with inserted character
*/
char* FFaMathString::InsStr(const char*s, int n, char c)
{
  if (n < 0 || n > (int)strlen(s))
    return (char*)s;

  char* s1 = new char[strlen(s) + 2];
  strncpy(s1,s,n);
  s1[n] = c;
  strncpy(s1+n+1,s+n,strlen(s)-n);
  s1[strlen(s) + 1] = 0;
  delete[] s;

  return s1;
}


/*!
  \fn bool FFaMathString::EqStr(const char* s1, const char* s2)
  \brief Equality of string one and two is checked.

  \param s1 First string
  \param s2 Second string

  \return \e true if the two strings are equal, otherwise \e false.
*/
bool FFaMathString::EqStr(const char* s1, const char* s2)
{
  return strcmp(s1,s2) == 0;
}


/*!
  \fn bool FFaMathString::CompStr(const char* s, int n, const char* s2)
  \brief Searchs a string for another string from given position.

  \param s String to search in
  \param n Position index
  \param s2 String to be found

  \return \e true if the string is found from the given position, otherwise \e false.
*/
bool FFaMathString::CompStr(const char* s, int n, const char* s2)
{
  if (n < 0 || n >= (int)strlen(s))
    return 0;

  return strncmp(s+n,s2,strlen(s2)) == 0;
}


/*!
  \brief Static helper checking if a character is a digit or '.'
*/
static bool isNumeric(char c)
{
  return isdigit(c) || c == '.';
}


/*!
  \fn bool FFaMathString::IsNumeric(const char* s)
  \brief Checks if a string is a number

  \param s string to be evaluated

  \return \e true if the string is a number, otherwise \e false.
*/
bool FFaMathString::IsNumeric(const char* s)
{
  size_t n = strlen(s);
  for (size_t i = 0; i < n; i++)
    if (!isNumeric(s[i]))
      return false;

  return true;
}


/*!
  \fn int FFaMathString::SearchCorOpenbracket(const char*s, int n)
  \brief Searchs a string, from a given position, for a corresponding bracket to '('

  \param s String
  \param n Position index

  \return The position to the corresponding bracket. -1 if not found.
*/
int FFaMathString::SearchCorOpenbracket(const char*s, int n)
{
  int i, c=1;

  for(i = n + 1; i < (int)strlen(s); i++) {
    if(s[i] == '(')
      c++;
    else if(s[i] == ')')
      c--;

    if(!c)
      return i;
  }

  return -1;
}

/*!
  \fn int FFaMathString::SearchCorClosebracket(const char*s, int n)
  \brief Searchs a string, from a given position, for a corresponding bracket to ')'

  \param s String
  \param n Position index

  \return The position to the corresponding bracket. -1 if not found.
*/
int FFaMathString::SearchCorClosebracket(const char*s, int n)
{
  int i, c=1;

  for(i = n - 1; i >= 0; i--){
    if(s[i] == ')')
      c++;
    else if(s[i] == '(')
      c--;

    if(!c)
      return i;
  }

  return -1;
}


/*!
  \fn int FFaMathString::SearchOperator(const char*s, FFaMathExpr::ROperator op)
  \brief A string is searched for a given operator.

  \param s String
  \param op Operator to be found

  \return The position to the operator. -1 if not found.
*/
int FFaMathString::SearchOperator(const char*s, FFaMathExpr::ROperator op)
{
  char opc = 0;
  char op2 = 0;
  switch (op) {

  case FFaMathExpr::Juxt:
    opc = ',';
    break;

  case FFaMathExpr::Add:
    opc = '+';
    break;

  case FFaMathExpr::Sub:
    opc = '-';
    break;

  case FFaMathExpr::Mult:
    opc = '*';
    break;

  case FFaMathExpr::Div:
    opc = '/';
    break;

  case FFaMathExpr::Mod:
    opc = '%';
    break;

  case FFaMathExpr::Pow:
    opc = '^';
    break;

  case FFaMathExpr::NthRoot:
    opc = '#';
    break;

  case FFaMathExpr::E10:
    opc = 'E';
    break;

  case FFaMathExpr::LogicalAnd:
    opc = op2 = '&';
    break;

  case FFaMathExpr::LogicalOr:
    opc = op2 = '|';
    break;

  case FFaMathExpr::LogicalEqual:
    opc = op2 = '=';
    break;

  case FFaMathExpr::LogicalNotEqual:
    opc = '!';
    op2 = '=';
    break;

  case FFaMathExpr::LogicalLessOrEqual:
    opc = '<';
    break;

  case FFaMathExpr::LogicalGreaterOrEqual:
    opc = '>';
    break;

  default:
    return -1;
  }

  int i;
  for (i = strlen(s)-1; i >= 0; i--)
    if (i > 0 && s[i-1] == opc && s[i] == op2)
      return i; // this is a two-character logical operator
    else if (s[i] == opc && (op != FFaMathExpr::Sub || (i > 0 && s[i-1] == ')')))
      return i;
    else if (s[i] == ')')
      i = FFaMathString::SearchCorClosebracket(s, i);

  return i;
}


/*!
  \fn int FFaMathString::GetFunction(const char*s, int n, FFaMathExpr::ROperator& fn)
  \brief Searchs a string for a function from a given position.

  \param s String
  \param n Position index
  \param fn The function found, or ErrOp if not found

  \return The length of the function if it is found in given position.
   0 if not found.
*/

int FFaMathString::GetFunction(const char*s, int n, FFaMathExpr::ROperator& fn)
{
  fn = FFaMathExpr::ErrOp;
  if (n < 0 || n+1 >= (int)strlen(s))
    return 0;

  if (s[n] == '!' && s[n+1] != '=')
  {
    fn = FFaMathExpr::LogicalNot;
    return 1;
  }

  if (FFaMathString::CompStr(s,n,"ln"))
    fn = FFaMathExpr::Ln;
  else if (FFaMathString::CompStr(s,n,"tg"))
    fn = FFaMathExpr::Tg;

  if (fn > FFaMathExpr::ErrOp)
    return 2;

  if (FFaMathString::CompStr(s,n,"exp"))
    fn = FFaMathExpr::Exp;
  else if (FFaMathString::CompStr(s,n,"log"))
    fn = FFaMathExpr::Log;
  else if (FFaMathString::CompStr(s,n,"abs"))
    fn = FFaMathExpr::Abs;
  else if (FFaMathString::CompStr(s,n,"max"))
    fn = FFaMathExpr::Max;
  else if (FFaMathString::CompStr(s,n,"min"))
    fn = FFaMathExpr::Min;
  else if (FFaMathString::CompStr(s,n,"sin"))
    fn = FFaMathExpr::Sin;
  else if (FFaMathString::CompStr(s,n,"cos"))
    fn = FFaMathExpr::Cos;
  else if (FFaMathString::CompStr(s,n,"tan"))
    fn = FFaMathExpr::Tg;
  else if (FFaMathString::CompStr(s,n,"atg"))
    fn = FFaMathExpr::Atan;

  if (fn > FFaMathExpr::ErrOp)
    return 3;

  if (FFaMathString::CompStr(s,n,"sqrt"))
    fn = FFaMathExpr::Sqrt;
  else if (FFaMathString::CompStr(s,n,"asin"))
    fn = FFaMathExpr::Asin;
  else if (FFaMathString::CompStr(s,n,"acos"))
    fn = FFaMathExpr::Acos;
  else if (FFaMathString::CompStr(s,n,"atan"))
    fn = FFaMathExpr::Atan;

  if (fn > FFaMathExpr::ErrOp)
    return 4;

  if (FFaMathString::CompStr(s,n,"arcsin"))
    fn = FFaMathExpr::Asin;
  else if (FFaMathString::CompStr(s,n,"arccos"))
    fn = FFaMathExpr::Acos;
  else if (FFaMathString::CompStr(s,n,"arctan"))
    fn = FFaMathExpr::Atan;

  if (fn > FFaMathExpr::ErrOp)
    return 6;

  if (FFaMathString::CompStr(s,n,"arctg"))
    fn = FFaMathExpr::Atan;
  else
    return 0;

  return 5;
}


/*!
  \fn char* FFaMathString::SimplifyStr(char*s)
  \brief Outer bracket of the string is removed
  \param s String

  \return The string without outer bracket.
*/
char* FFaMathString::SimplifyStr(char*s)
{
  if(!s || !strlen(s))
    return 0;

  bool ind = false;
  char* s1 = s;
  char* s2 = 0;

  while(s1[strlen(s1)-1] == '\n')
  {
    s2 = s1;
    s1 = FFaMathString::CopyStr(s1, 0, strlen(s1) - 2);
    if(ind && s1 != s2) delete[] s2;
    if(!s1) return s1;
    ind = true;
  }

  if(s1[0] == '(' && FFaMathString::SearchCorOpenbracket(s1, 0) == (int)strlen(s1) - 1) {
    s2 = s1;
    s1 = FFaMathString::CopyStr(s1, 1, strlen(s1) - 2);
    if(ind && s1 != s2) delete[] s2;
    if(!s1) return s1;
    ind = true;
  }

  while(s1[0] == ' ')
  {
    s2 = s1;
    s1 = FFaMathString::CopyStr(s1, 1, strlen(s1) - 1);
    if(ind && s1 != s2) delete[] s2;
    if(!s1) return s1;
    ind = true;
  }

  while(s1[strlen(s1)-1] == ' ')
  {
    s2 = s1;
    s1 = FFaMathString::CopyStr(s1, 0, strlen(s1) - 2);
    if(ind && s1 != s2) delete[] s2;
    if(!s1) return s1;
    ind = true;
  }

  if(ind)
    s1 = FFaMathString::SimplifyStr(s1);

  if (s1 != s) delete[] s;
  return s1;
}


/*!
  \fn int FFaMathString::IsVar(const char*s, int n, int nvar, const FFaMathVar**ppvar)
  \brief Searchs a string for a variabel from a given position.

  \param s String
  \param n Position index
  \param nvar Variable index
  \param ppvar Variable to be found

  \return The length of the variable if the variable is found in given position.
   0 if not found.
*/
int FFaMathString::IsVar(const char*s, int n, int nvar, FFaMathVar**ppvar)
{
  if(n < 0 || n > (int)strlen(s))
    return 0;

  for (int i = 0; i < nvar; i++)
    if (FFaMathString::CompStr(s, n, (*(ppvar + i)) -> name))
      return strlen((*(ppvar + i)) -> name);

  return 0;
}


/*!
  \fn int FFaMathString::IsPi(const char*s, int n)
  \brief Searchs a string for the constant "pi" from a given position.

  \param s String
  \param n Position index

  \return The length (2) of the "pi" if found in given position.
   0 if not found.
*/
int FFaMathString::IsPi(const char*s, int n)
{
  if (n >= 0 && n+1 < (int)strlen(s))
    if (FFaMathString::CompStr(s, n, "pi") ||
        FFaMathString::CompStr(s, n, "PI") ||
        FFaMathString::CompStr(s, n, "Pi"))
      return 2;

  return 0;
}


/*!
  \fn int FFaMathString::IsFunction(const char*s, int n, int nfunc, FFaMathFunction**ppfunc)
  \brief Searchs a string or a function f(*) for a function from a given position

  \param s String
  \param n Position index
  \param nfunc Function index
  \param ppfunc Function

  \return The length of the function if it is found from the given position.
   0 if not found.
*/
int FFaMathString::IsFunction(const char*s, int n, int nfunc, FFaMathFunction**ppfunc)
{
  FFaMathExpr::ROperator fn;
  int l = FFaMathString::GetFunction(s, n, fn);
  if (l) return l; // hvis funksjonen er av ferdigdefinerte typer

  for (int i = 0; i < nfunc; i++) // hvis funksjonen er sammensatt
    if (FFaMathString::CompStr(s, n, ppfunc[i] -> name))
      if ((int)strlen(ppfunc[i]->name) > l)
        l = strlen(ppfunc[i]->name);

  return l;
}


/*!
  \fn char* FFaMathString::IsolateVars(char*s, int nvar, FFaMathVar **ppvar)
  \brief Variables in a string is isolated with brackets

  \param s String
  \param nvar Variable index
  \param ppvar Variable

  \return The string with isolated variables
*/
char* FFaMathString::IsolateVars(char*s, int nvar, FFaMathVar **ppvar)
{
  int i, j;
  FFaMathExpr::ROperator fn;

  for (i = 0; i < (int)strlen(s); i++)
    if (s[i] == '(') {
      if ((i = FFaMathString::SearchCorOpenbracket(s, i)) == -1)
        break;
    }
    else if ((j = FFaMathString::IsVar(s, i, nvar, ppvar)) > 0 ||
             (j = FFaMathString::IsPi(s, i)) > 0) {
      s = FFaMathString::InsStr(s, i, '(');
      s = FFaMathString::InsStr(s, i+j+1, ')');
      i += j+1;
    }
    else if ((j = FFaMathString::GetFunction(s, i, fn)) > 0)
      i += j-1;

  return s;
}


/*!
  \fn char* FFaMathString::IsolateNumbers(char*s, int nvar, FFaMathVar**ppvar,
  int nfunc, FFaMathFunction**ppfunc)
  \brief Isolates numbers in a string or function f(*) with brackets.

  \param s String
  \param nvar Variable index
  \param ppvar Variable
  \param nfunc Function index
  \param ppfunc Function

  \return The string or a function with isolated numbers.
*/
char* FFaMathString::IsolateNumbers(char*s, int nvar, FFaMathVar**ppvar,
                                    int nfunc, FFaMathFunction**ppfunc)
{
  int i, i2, t;
  bool ind = false;

  for (i = i2 = 0; i <= (int)strlen(s); i++)
    if (ind && !isNumeric(s[i])) {
      ind = false;
      s = FFaMathString::InsStr(s, i2, '(');
      s = FFaMathString::InsStr(s, ++i, ')');
    }
    else if ((t = FFaMathString::IsVar(s, i, nvar, ppvar)))
      i += t - 1;
    else if ((t = FFaMathString::IsFunction(s, i, nfunc, ppfunc)))
      i += t - 1;
    else if (s[i] == '(') {
      i = FFaMathString::SearchCorOpenbracket(s, i);
      if (i == -1) break;
    }
    else if (!ind && isNumeric(s[i])) {
      i2 = i;
      ind = true;
    }

  return s;
}


/*!
  \fn char* FFaMathString::SupprSpaces(char *s)
  \brief All spaces in a string is removed.

  \param s String to be modified

  \return The modified string without spaces.
*/
char* FFaMathString::SupprSpaces(char *s)
{
  size_t i, n;

  for (i = n = 0; i < strlen(s); i++)
    if (!isspace(s[i])) {
      if (n < i)
        s[n++] = s[i];
      else
        n++;
    }

  if (n == strlen(s))
    return s;

  char* s1 = new char[n+1];
  strncpy(s1,s,n);
  s1[n] = 0;

  delete[] s;
  return s1;
}
