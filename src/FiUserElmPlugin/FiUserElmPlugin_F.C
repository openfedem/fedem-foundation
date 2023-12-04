// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

/*!
  \file FiUserElmPlugin_F.C
  \brief Fortran wrapper for the FiUserElmPlugin methods.
*/

#include "FiUserElmPlugin.H"
#include "FFaLib/FFaCmdLineArg/FFaCmdLineArg.H"
#include "FFaLib/FFaString/FFaTokenizer.H"
#include "FFaLib/FFaOS/FFaFortran.H"
#include <cstring>


/*!
  \brief Loads the user-defined element plugin library into memory.
  \arg gdata - Global parameters that applies to all element instances
  \arg sign  - Text string containing a description of the loaded library
  \arg nchar - Maximum length of the description string
  \arg ierr  - Error flag
*/

SUBROUTINE (fi_ude_init,FI_UDE_INIT) (const double* gdata, char* sign,
#ifdef _NCHAR_AFTER_CHARARG
                                      int nchar, int& ierr
#else
                                      int& ierr, int nchar
#endif
){
  ierr = 0;
  if (!FiUserElmPlugin::instance()->areLibsLoaded())
  {
    // Get plugin file name from command-line
    std::string plugin;
    FFaCmdLineArg::instance()->getValue("plugin",plugin);
    if (plugin.empty())
    {
      std::cerr <<"FiUserElmPlugin: No plugins specified."<< std::endl;
      ierr = -1;
      return;
    }
    else if (plugin[0] == '<')
    {
      // We have a multi-file list, find the correct one containing
      // user-defined functions (there should only be one)
      size_t i;
      FFaTokenizer files(plugin,'<','>',',');
      for (i = 0; i < files.size(); i++)
        if (FiUserElmPlugin::instance()->validate(files[i]))
          break;

      if (i < files.size())
        plugin = files[i];
      else
      {
        std::cerr <<"FiUserElmPlugin: No valid plugin specified."<< std::endl;
        ierr = -2;
        return;
      }
    }

    // Load the user-defined elements plugin and get its signature.
    // Pad the string with trailing spaces when returning to Fortran.
    if (!FiUserElmPlugin::instance()->load(plugin))
      ierr = -3;
    else if (!FiUserElmPlugin::instance()->getSign(nchar,sign))
      memset(sign,' ',nchar);
    else if ((nchar -= strlen(sign)) > 0)
      memset(sign+strlen(sign),' ',nchar);

    // Pass the global parameters to the loaded user-defined elements plugin
    if (!ierr) ierr = FiUserElmPlugin::instance()->init(gdata);
  }
  else
    memset(sign,' ',nchar);
}


/*!
  \brief Returns the length of the work arrays needed by a user-defined element.
  \arg eId    - Unique id identifying this element instance (the baseID)
  \arg eType  - Unique id identifying the element type
  \arg nenod  - Number of nodes in the element
  \arg nedof  - Number of degrees of freedom in the element
  \arg niwork - Required size of the integer work area for this element
  \arg nrwork - Required size of the double precision work area for this element
*/

SUBROUTINE(fi_ude0,FI_UDE0) (const int& eId, const int& eType,
                             const int& nenod, const int& nedof,
                             int& niwork, int& nrwork)
{
  FiUserElmPlugin::instance()->init(eId,eType,nenod,nedof,niwork,nrwork);
}


/*!
  \brief Initializes the constant (state-independent) part of the work areas.
  \arg eId   - Unique id identifying this element instance (the baseID)
  \arg eType - Unique id identifying the element type
  \arg nenod - Number of nodes in the element
  \arg nedof - Number of degrees of freedom in the element
  \arg X     - Global coordinas of the element nodes (initial configuration)
  \arg T     - Local coordinate systems of the element nodes
  \arg iwork - Integer work area for this element
  \arg rwork - Double precision work area for this element
  \arg ierr  - Error flag
*/

SUBROUTINE(fi_ude1,FI_UDE1) (const int& eId, const int& eType,
                             const int& nenod, const int& nedof,
                             const double* X, const double* T,
                             int* iwork, double* rwork, int& ierr)
{
  ierr = FiUserElmPlugin::instance()->init(eId,eType,nenod,nedof,
                                           X,T,iwork,rwork);
}


/*!
  \brief Updates the state of a given user-defined element.
  \arg eId   - Unique id identifying this element instance (the baseID)
  \arg eType - Unique id identifying the element type
  \arg nenod - Number of nodes in the element
  \arg nedof - Number of degrees of freedom in the element
  \arg X - Global nodal coordinates of the element (current configuration)
  \arg T - Local nodal coordinate systems of the element (current configuration)
  \arg V - Global nodal velocities of the element (current configuration)
  \arg A - Global nodal accelerations of the element (current configuration)
  \arg iwork - Integer work area for this element
  \arg rwork - Real work area for this element
  \arg K  - Tangent stiffness matrix
  \arg C  - Damping matrix
  \arg M  - Mass matrix
  \arg Fs - Internal elastic forces
  \arg Fd - Damping forces
  \arg Fi - Intertia forces
  \arg Q  - External forces
  \arg t  - Current time
  \arg dt - Time step size
  \arg istep - Time step number
  \arg iter  - Iteration number
  \arg ierr  - Error flag

  \details This function is invoked once within the Newton iteration loop for
  each element.
  It should evaluate the updated tangent matrices and associated force vectors.
*/

SUBROUTINE(fi_ude2,FI_UDE2) (const int& eId, const int& eType,
                             const int& nenod, const int& nedof,
                             const double* X, const double* T,
                             const double* V, const double* A,
                             int* iwork, double* rwork,
                             double* K, double* C, double* M,
                             double* Fs, double* Fd, double* Fi, double* Q,
                             const double& t, const double& dt,
                             const int& istep, const int& iter, int& ierr)
{
  ierr = FiUserElmPlugin::instance()->update(eId,eType,nenod,nedof,X,T,V,A,
                                             iwork,rwork,K,C,M,Fs,Fd,Fi,Q,
                                             t,dt,istep,iter);
}


/*!
  \brief Calculates the local origin of a user-defined element.
  \arg eId   - Unique id identifying this element instance (the baseID)
  \arg eType - Unique id identifying the element type
  \arg nenod - Number of element nodes
  \arg X     - Global nodal coordinates of the element (current configuration)
  \arg T     - Local nodal coordinate systems of the element (current config.)
  \arg iwork - Integer work area for this element
  \arg rwork - Real work area for this element
  \arg Tlg   - Current local-to-global transformation matrix for the element
  \arg ierr  - Error flag

  \details This function is invoked only in pre- and post-processing tasks,
  requiring the position of current element.
  It does not affect the response simulation.
  \note The user-defined element plugin does not need to contain this function.
  If absent, the identity transformation matrix is assumed.
*/

SUBROUTINE(fi_ude3,FI_UDE3) (const int& eId, const int& eType, const int& nenod,
                             const double* X, const double* T,
                             int* iwork, double* rwork, double* Tlg, int& ierr)
{
  ierr = FiUserElmPlugin::instance()->origin(eId,eType,nenod,X,T,
                                             iwork,rwork,Tlg);
}


/*!
  \brief Returns the name of a result quantity of a user-defined element.
  \arg eId   - Unique id identifying this element instance (the baseID)
  \arg eType - Unique id identifying the element type
  \arg idx   - Result quantity index
  \arg iwork - Integer work area for this element
  \arg rwork - Real work area for this element
  \arg name  - Name of result quantity
  \arg nchar - Maximum length of the result quantity name
  \arg nvar  - Total number of result quantities for this element

  \details This function is only invoked once as a pre-processing task.
  \note The user-defined element plugin does not need to contain this function.
  If absent, no output variables are defined.
*/

SUBROUTINE (fi_ude4,FI_UDE4) (const int& eId, const int& eType, const int& idx,
                              const int* iwork, const double* rwork,
#ifdef _NCHAR_AFTER_CHARARG
                              char* name, int nchar, int& nvar
#else
                              char* name, int& nvar, int nchar
#endif
){
  nvar = FiUserElmPlugin::instance()->result(eId,eType,idx,
                                             iwork,rwork,nchar,name);

  // Pad the string with trailing spaces when returning to Fortran
  if (nvar < 1)
    memset(name,' ',nchar);
  else if ((nchar -= strlen(name)) > 0)
    memset(name+strlen(name),' ',nchar);
}


/*!
  \brief Returns a result quantity value of a user-defined element.
  \arg eId   - Unique id identifying this element instance (the baseID)
  \arg eType - Unique id identifying the element type
  \arg idx   - Result quantity index
  \arg iwork - Integer work area for this element
  \arg rwork - Real work area for this element
  \arg value - The result quantity value
  \arg nvar  - Total number of result quantities for this element

  \details This function is only invoked once as a post-processing task
  after each time increment, when saving results to file.
  \note The user-defined element plugin does not need to contain this function.
  If absent, no output variables are defined.
*/

SUBROUTINE (fi_ude5,FI_UDE5) (const int& eId, const int& eType, const int& idx,
                              const int* iwork, const double* rwork,
                              double& value, int& nvar)
{
  nvar = FiUserElmPlugin::instance()->result(eId,eType,idx,iwork,rwork,value);
}


/*!
  \brief Calculates the total mass of a user-defined element.
  \arg eId   - Unique id identifying this element instance (the baseID)
  \arg eType - Unique id identifying the element type
  \arg nenod - Number of element nodes
  \arg X     - Global nodal coordinates of the element (current configuration)
  \arg iwork - Integer work area for this element
  \arg rwork - Real work area for this element
  \arg mass  - Total mass for the element
  \arg ierr  - Error flag

  \details This function is invoked only as a pre-processing task,
  in the process of generating a total mass summary of the model.
  It does not affect the response simulation.
  \note The user-defined element plugin does not need to contain this function.
  If absent, all user-defined elements are assumed to be mass-less.
*/

SUBROUTINE(fi_ude6,FI_UDE6) (const int& eId, const int& eType, const int& nenod,
                             const double* X, int* iwork, double* rwork,
                             double& mass, int& ierr)
{
  ierr = FiUserElmPlugin::instance()->mass(eId,eType,nenod,X,iwork,rwork,mass);
}
