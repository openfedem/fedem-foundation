// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

/*!
  \file FFaUserFuncPlugin.H
  \brief User-defined functions plugin.
*/

#ifndef FFA_USER_FUNC_PLUGIN_H
#define FFA_USER_FUNC_PLUGIN_H

#include "FFaLib/FFaOS/FFaDynamicLibraryBase.H"
#include "FFaLib/FFaPatterns/FFaSingelton.H"


/*!
  \brief Singleton class for user-defined functions plugins.
*/

class FFaUserFuncPlugin : public FFaDynamicLibraryBase,
                          public FFaSingelton<FFaUserFuncPlugin>
{
public:
  //! \brief Checks if the named library can be loaded as a plugin.
  //! \param[in] lib Full path to the dynamic library
  //! \param[in] nchar Max length of the signature string
  //! \param[out] sign A signature string describing the loaded plugin
  bool validate(const std::string& lib, int nchar = 0, char* sign = NULL);
  //! \brief Returns the signature string of the loaded plugin.
  //! \param[in] nchar Max length of the signature string
  //! \param[out] sign A signature string describing the loaded plugin
  bool getSign(int nchar, char* sign) const;

  //! \brief Returns a list of function types defined in the plugin.
  //! \param[in] maxUF Max number of user-defined function types
  //! \param[out] funcId List of unique ids identifying each function type
  //! \param[in] silence If \e true, suppress error messages
  //! \return Number of different function types in the plugin
  int getFuncs(int maxUF, int* funcId, bool silence = false) const;
  //! \brief Returns a function type name.
  //! \param[in] id Unique id identifying the function type
  //! \param[in] nchar Max length of the function type name
  //! \param[out] name The function type name corresponding to \a id
  //! \return Number of arguments for the function type
  int getFuncName(int id, int nchar = 0, char* name = NULL) const;
  //! \brief Returns a function parameter name for a given function type.
  //! \param[in] id Unique id identifying the function type
  //! \param[in] ipar Function parameter index
  //! \param[in] nchar Max length of the function parameter name
  //! \param[out] name The name of the parameter
  //! \return Total number of parameters for this function type
  int getParName(int id, int ipar = 1, int nchar = 0, char* name = NULL) const;
  //! \brief Returns a default parameter value for a given function type.
  //! \param[in] id Unique id identifying the function type
  //! \param[in] ipar Function parameter index
  double getDefaultParVal(int id, int ipar) const;

  //! \brief Returns a pointer to a pixmap to display in the property panel.
  const char** getPixmap(int id) const;

  //! \brief Returns the function flag for a given function type, \a id.
  //! \details This flag is used to trigger different handling of the
  //! user-defined function in the Fedem dynamics solver: \n
  //! 0 : No particular treatment, the function is evaluated only when needed \n
  //! 1 : (Pre)evaluate this function in the beginning of each increment \n
  //! 2 : Evaluate this function at the end og each time increment \n
  //! 4 : This is a user-defined wave function
  int getFlag(int id) const;

  //! \brief Evaluates single-argument user-defined functions.
  //! \param[in] bId Base id of the function instance
  //! \param[in] fId Unique id identifying the function type
  //! \param[in] params Array of constant function parameters
  //! \param[in] x Function argument
  //! \param[in] err Error indicator
  double getValue(int bId, int fId, const double* params,
                  double x, int& err) const;
  //! \brief Evaluates multi-argument user-defined functions.
  //! \param[in] bId Base id of the function instance
  //! \param[in] fId Unique id identifying the function type
  //! \param[in] params Array of constant function parameters
  //! \param[in] args Function arguments
  //! \param[in] err Error indicator
  double getValue(int bId, int fId, const double* params,
                  const double* args, int& err) const;
  //! \brief Evaluates a derivative of multi-argument user-defined functions.
  //! \param[in] bId Base id of the function instance
  //! \param[in] fId Unique id identifying the function type
  //! \param[in] ia Index of the argument of differentiate with respect to
  //! \param[in] params Array of constant function parameters
  //! \param[in] args Function arguments
  //! \param[in] err Error indicator
  double getDiff(int bId, int fId, int ia, const double* params,
                 const double* args, int& err) const;

  //! \brief Evaluates user-defined wave functions.
  //! \param[in] bId Base id of the function instance
  //! \param[in] fId Unique id identifying the function type
  //! \param[in] d Water depth
  //! \param[in] g Gravity constant
  //! \param[in] params Array of constant function parameters
  //! \param[in] args Function arguments
  //! \param[out] h Wave elevation
  //! \param[out] u Particle velocity
  //! \param[out] du Particle acceleration
  int wave(int bId, int fId, double d, double g,
           const double* params, const double* args,
           double& h, double* u, double* du) const;

private:
  friend class FFaSingelton<FFaUserFuncPlugin>;
};

#endif
