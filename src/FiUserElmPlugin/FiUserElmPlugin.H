// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

/*!
  \file FiUserElmPlugin.H
  \brief User-defined elements plugin.
*/

#ifndef FI_USER_ELM_PLUGIN_H
#define FI_USER_ELM_PLUGIN_H

#include "FFaLib/FFaOS/FFaDynamicLibraryBase.H"
#include "FFaLib/FFaPatterns/FFaSingelton.H"


/*!
  \brief Singleton class for user-defined element plugins.
*/

class FiUserElmPlugin : public FFaDynamicLibraryBase,
                        public FFaSingelton<FiUserElmPlugin>
{
protected:
  //! \brief Default constructor.
  FiUserElmPlugin() {}
  //! \brief Empty destructor.
  virtual ~FiUserElmPlugin() {}

public:
  //! \brief Checks if the named library can be loaded as a plugin.
  //! \param[in] libName Full path to the dynamic library.
  //! \param[in] nchar Max length of the signature string
  //! \param[out] sign A signature string describing the loaded plugin
  //! return \e true if the named plugin contains user-defined element types.
  bool validate(const std::string& libName, int nchar = 0, char* sign = NULL);
  //! \brief Returns the signature string of the loaded plugin.
  //! \param[in] nchar Max length of the signature string
  //! \param[out] sign A signature string describing the loaded plugin
  //! \param[in] silence If \e true, suppress error messages
  bool getSign(int nchar, char* sign, bool silence = false);

  //! \brief Returns a list of element types defined in the plugin.
  //! \param[in] maxUE Max number of user-defined element types
  //! \param[out] eType List of unique ids identifying each element type
  //! \return Number of different element types in the plugin
  int getElementTypes(int maxUE, int* eType);
  //! \brief Returns the name of an element type.
  //! \param[in] eType Unique id identifying the element type
  //! \param[in] nchar Max length of the element type name
  //! \param[out] name The element type name corresponding to \a eType
  //! \return Number of nodes for the element type
  int getTypeName(int eType, int nchar, char* name);

  //! \brief Initializes the plugin.
  //! \param[in] gdata Global parameters that applies to all elements
  int init(const double* gdata);
  //! \brief Returns the required length of the work areas for a given element.
  //! \param[in] eId Unique id identifying this element instance (the baseID)
  //! \param[in] eType Unique id identifying the element type
  //! \param[in] nenod Number of element nodes
  //! \param[in] nedof Number of element degrees of freedom
  //! \param[out] niwork Required size of the integer work area for this element
  //! \param[out] nrwork Required size of the real work area for this element
  int init(int eId, int eType, int nenod, int nedof, int& niwork, int& nrwork);
  //! \brief Initializes the state-independent part of the element work areas.
  //! \param[in] eId Unique id identifying this element instance (the baseID)
  //! \param[in] eType Unique id identifying the element type
  //! \param[in] nenod Number of element nodes
  //! \param[in] nedof Number of element degrees of freedom
  //! \param[in] X Global initial nodal coordinates of the element
  //! \param[in] T Local initial nodal coordinate systems of the element
  //! \param iwork Integer work area for this element
  //! \param rwork Real work area for this element
  int init(int eId, int eType, int nenod, int nedof,
           const double* X, const double* T, int* iwork, double* rwork);

  //! \brief Calculates the total mass for a given user-defined element.
  //! \param[in] eId Unique id identifying this element instance (the baseID)
  //! \param[in] eType Unique id identifying the element type
  //! \param[in] nenod Number of element nodes
  //! \param[in] X Global initial nodal coordinates of the element
  //! \param iwork Integer work area for this element
  //! \param rwork Real work area for this element
  //! \param[out] mass Mass of the element
  int mass(int eId, int eType, int nenod,
           const double* X, int* iwork, double* rwork, double& mass);

  //! \brief Updates the state of a given user-defined element.
  //! \param[in] eId Unique id identifying this element instance (the baseID)
  //! \param[in] eType Unique id identifying the element type
  //! \param[in] nenod Number of element nodes
  //! \param[in] nedof Number of element degrees of freedom
  //! \param[in] X Global nodal coordinates of the element
  //! \param[in] T Local nodal coordinate systems of the element
  //! \param[in] V Global nodal velocities of the element
  //! \param[in] A Global nodal accelerations of the element
  //! \param iwork Integer work area for this element
  //! \param rwork Real work area for this element
  //! \param[out] Kt Tangent stiffness matrix
  //! \param[out] Ct Damping matrix
  //! \param[out] M Mass matrix
  //! \param[out] Fs Internal elastic forces
  //! \param[out] Fd Damping forces
  //! \param[out] Fi Intertia forces
  //! \param[out] Q External forces
  //! \param[in] t Current time
  //! \param[in] dt Time step size
  //! \param[in] istep Time step number
  //! \param[in] iter Iteration number
  int update(int eId, int eType, int nenod, int nedof,
             const double* X, const double* T,
             const double* V, const double* A, int* iwork, double* rwork,
             double* Kt, double* Ct, double* M,
             double* Fs, double* Fd, double* Fi, double* Q,
             double t, double dt, int istep, int iter);

  //! \brief Calculates the local origin of a user-defined element.
  //! \param[in] eId Unique id identifying this element instance (the baseID)
  //! \param[in] eType Unique id identifying the element type
  //! \param[in] nenod Number of element nodes
  //! \param[in] X Global nodal coordinates of the element
  //! \param[in] T Local nodal coordinate systems of the element
  //! \param[in] iwork Integer work area for this element
  //! \param[in] rwork Real work area for this element
  //! \param[out] Tlg Local-to-global transformation matrix for this element
  int origin(int eId, int eType, int nenod,
             const double* X, const double* T,
             const int* iwork, const double* rwork, double* Tlg);

  //! \brief Returns the name of a result quantity of a user-defined element.
  //! \param[in] eId Unique id identifying this element instance (the baseID)
  //! \param[in] eType Unique id identifying the element type
  //! \param[in] idx Result quantity index
  //! \param[in] iwork Integer work area for this element
  //! \param[in] rwork Real work area for this element
  //! \param[in] nchar Maximum length of the result quantity name
  //! \param[out] name Name of result quantity
  //! \return Total number of result quantities for this element
  //! \return Zero, if \a idx is out of range
  int result(int eId, int eType, int idx,
             const int* iwork, const double* rwork, int nchar, char* name);
  //! \brief Returns the value of a result quantity of a user-defined element.
  //! \param[in] eId Unique id identifying this element instance (the baseID)
  //! \param[in] eType Unique id identifying the element type
  //! \param[in] idx Result quantity index
  //! \param[in] iwork Integer work area for this element
  //! \param[in] rwork Real work area for this element
  //! \param[out] value The result quantity
  //! \return Total number of result quantities for this element
  //! \return Zero, if \a idx is out of range
  int result(int eId, int eType, int idx,
             const int* iwork, const double* rwork, double& value);

private:
  friend class FFaSingelton<FiUserElmPlugin>;
};

#endif
