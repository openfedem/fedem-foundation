// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

/*!
  \file FFaMathExprFactory.H
  \brief Mathematical expression management.
*/

#ifndef FFA_MATH_EXPR_FACTORY_H
#define FFA_MATH_EXPR_FACTORY_H

#include "FFaLib/FFaPatterns/FFaSingelton.H"
#include <string>
#include <vector>
#include <map>

class FFaMathExpr;
class FFaMathVar;


/*!
  \brief Singelton class holding all mathematical expression objects.
*/

class FFaMathExprFactory : public FFaSingelton<FFaMathExprFactory>
{
  //! \brief The constructor is private to allow singelton construction only.
  FFaMathExprFactory() {}
  //! \brief The destructor is private to allow singelton destruction only.
  virtual ~FFaMathExprFactory() {}

  //! \brief To allow construction and destruction.
  friend class FFaSingelton<FFaMathExprFactory>;

public:
  //! \brief Counts the occurances of arguments \a vars in \a expression.
  static size_t countArgs(const std::string& expression, const char** vars,
			  std::vector<bool>* present = 0);

  //! \brief Creates a new FFaMathFunc instance from the string \a expression.
  int create(int id, const std::string& expression,
	     size_t nvar = 1, const char** vars = 0);

  //! \brief Evaluates a single-argument math expression.
  double getValue(int id, double arg, int& error);
  //! \brief Evaluates a multi-argument math expression.
  double getValue(int id, const double* arg, int& error);

  //! \brief Evaluates the derivative of a single-argument math expression.
  double getDiff(int id, double arg, int& error);
  //! \brief Evaluates a derivative of a multi-argument math expression.
  double getDiff(int id, size_t idArg, const double* arg, int& error);

private:

  //! \brief Class describing a mathematical expression.
  class FFaMathFunc
  {
  public:
    std::string               estr; //!< Function expression as text
    FFaMathExpr*              expr; //!< Resolved expression tree
    std::vector<FFaMathExpr*> diff; //!< Exression tree for function derivatives
    std::vector<FFaMathVar*>  args; //!< Function arguments

    //! \brief Default constructor.
    FFaMathFunc() { expr = NULL; }
    //! \brief The copy constructor is deleted.
    FFaMathFunc(const FFaMathFunc&) = delete;
    //! \brief The assignment operator is deleted.
    FFaMathFunc& operator=(const FFaMathFunc&) = delete;
    //! \brief The destructor deletes the heap-allocated members.
    ~FFaMathFunc();
  };

  std::map<int,FFaMathFunc> myIndexMap; //! Indexed math expression container

  //! \cond DO_NOT_DOCUMENT
  using IMIter = std::map<int,FFaMathFunc>::iterator;
  //! \endcond
};

#endif
