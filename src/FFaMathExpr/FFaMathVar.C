// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#include "FFaMathVar.H"
#include "FFaMathString.H"

/*!
  \fn FFaMathVar::FFaMathVar(const char* namep)
  \brief An object of type FFaMathVar is created. Constructor
  \param namep Variable name, string
*/
FFaMathVar::FFaMathVar(const char* namep)
{
  pval = 0.0;
  name = FFaMathString::CopyStr(namep);
}

/*!
  \fn FFaMathVar::~FFaMathVar()
  \brief Destructor for FFaMathVar
*/
FFaMathVar::~FFaMathVar()
{
  if (name)
    delete[] name;
}

/*!
  \fn FFaMathVar::rename(const char* namep)
  \brief Assign a new name to the variable
  \param namep The new variable name, string
*/
void FFaMathVar::rename(const char* namep)
{
  if (FFaMathString::EqStr(name,namep)) return;

  delete[] name;
  name = FFaMathString::CopyStr(namep);
}

/*!
  \fn bool FFaMathVar::operator == (const FFaMathVar& var2) const
  \brief Overload operator '=='

  \param var2 of type FFaMathVar
  \return The result of the equality check
*/
bool FFaMathVar::operator==(const FFaMathVar& var2) const
{
  return this->pval == var2.pval && FFaMathString::EqStr(this->name,var2.name);
}
