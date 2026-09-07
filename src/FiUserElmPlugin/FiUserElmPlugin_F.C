// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

/*!
  \file FiUserElmPlugin_F.C
  \brief Fortran wrapper for the FiUserElmPlugin methods.
  \details This file contains the implementation of the following
  Fortran wrappers of the FiUserElmPlugin module:

  - fiuserelminterface::fi_ude_init
  - fiuserelminterface::fi_ude0
  - fiuserelminterface::fi_ude1
  - fiuserelminterface::fi_ude2
  - fiuserelminterface::fi_ude3
  - fiuserelminterface::fi_ude4
  - fiuserelminterface::fi_ude5
  - fiuserelminterface::fi_ude6

  No further documentation is provided here.
  The wrappers are documented in the FiUserElmInterface.f90 file.
*/

#include <cstring>

#include "FiUserElmPlugin/FiUserElmPlugin.H"
#include "FFaLib/FFaString/FFaTokenizer.H"
#include "FFaLib/FFaOS/FFaFortran.H"


SUBROUTINE (fi_ude_init,FI_UDE_INIT) (const char* plugin,
#ifdef _NCHAR_AFTER_CHARARG
                                      const int ncp, const double* gdata,
                                      char* sign, int nchar, int& ierr
#else
                                      const double* gdata, char* sign,
                                      int& ierr, const int ncp, int nchar
#endif
){
  ierr = 0;
  if (!FiUserElmPlugin::instance()->areLibsLoaded())
  {
    // Get plugin file name from command-line
    std::string pluginLib;
    if (ncp > 0 && plugin[0] == '<')
    {
      // We have a multi-file list, find the correct one containing
      // user-defined functions (there should only be one)
      FFaTokenizer files(std::string(plugin,ncp),'<','>',',');
      for (const std::string& file : files)
        if (FiUserElmPlugin::instance()->validate(file))
        {
          pluginLib = file;
          break;
	}
    }
    else
      pluginLib = std::string(plugin,ncp);

    if (pluginLib.empty())
    {
      std::cerr <<"FiUserElmPlugin: No valid plugin in \""
                << std::string(plugin,ncp) <<"\"."<< std::endl;
      ierr = -2;
      return;
    }

    // Load the user-defined elements plugin and get its signature.
    // Pad the string with trailing spaces when returning to Fortran.
    if (!FiUserElmPlugin::instance()->load(pluginLib))
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


SUBROUTINE(fi_ude0,FI_UDE0) (const int& eId, const int& eType,
                             const int& nenod, const int& nedof,
                             int& niwork, int& nrwork)
{
  FiUserElmPlugin::instance()->init(eId,eType,nenod,nedof,niwork,nrwork);
}


SUBROUTINE(fi_ude1,FI_UDE1) (const int& eId, const int& eType,
                             const int& nenod, const int& nedof,
                             const double* X, const double* T,
                             int* iwork, double* rwork, int& ierr)
{
  ierr = FiUserElmPlugin::instance()->init(eId,eType,nenod,nedof,
                                           X,T,iwork,rwork);
}


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


SUBROUTINE(fi_ude3,FI_UDE3) (const int& eId, const int& eType, const int& nenod,
                             const double* X, const double* T,
                             int* iwork, double* rwork, double* Tlg, int& ierr)
{
  ierr = FiUserElmPlugin::instance()->origin(eId,eType,nenod,X,T,
                                             iwork,rwork,Tlg);
}


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


SUBROUTINE (fi_ude5,FI_UDE5) (const int& eId, const int& eType, const int& idx,
                              const int* iwork, const double* rwork,
                              double& value, int& nvar)
{
  nvar = FiUserElmPlugin::instance()->result(eId,eType,idx,iwork,rwork,value);
}


SUBROUTINE(fi_ude6,FI_UDE6) (const int& eId, const int& eType, const int& nenod,
                             const double* X, int* iwork, double* rwork,
                             double& mass, int& ierr)
{
  ierr = FiUserElmPlugin::instance()->mass(eId,eType,nenod,X,iwork,rwork,mass);
}
