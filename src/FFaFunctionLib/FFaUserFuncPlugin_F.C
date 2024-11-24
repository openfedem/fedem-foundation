// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#include <cstring>

#include "FFaFunctionLib/FFaUserFuncPlugin.H"
#include "FFaLib/FFaString/FFaTokenizer.H"
#include "FFaLib/FFaOS/FFaFortran.H"


INTEGER_FUNCTION(ffauf_init,FFAUF_INIT) (const char* plugin,
#ifdef _NCHAR_AFTER_CHARARG
                                         const int ncp, const int& funcId,
                                         char* sign, int nc
#else
                                         const int& funcId, char* sign,
                                         const int ncp, int nc
#endif
){
  if (!FFaUserFuncPlugin::instance()->areLibsLoaded())
  {
    std::string pluginLib;
    if (ncp > 0 && plugin[0] == '<')
    {
      // We have a multi-file list, find the correct one containing
      // user-defined functions (there should only be one)
      FFaTokenizer files(std::string(plugin,ncp),'<','>',',');
      for (const std::string& file : files)
        if (FFaUserFuncPlugin::instance()->validate(file))
        {
          pluginLib = file;
          break;
        }
    }
    else
      pluginLib = std::string(plugin,ncp);

    if (pluginLib.empty())
    {
      std::cerr <<"FFaUserFuncPlugin: No valid plugin in \""
                << std::string(plugin,ncp) <<"\"."<< std::endl;
      return -2;
    }

    // Load the user-defined functions plugin and get its signature.
    // Pad the string with trailing spaces when returning to Fortran.
    if (!FFaUserFuncPlugin::instance()->load(pluginLib))
      return -3;
    else if (!FFaUserFuncPlugin::instance()->getSign(nc,sign))
      memset(sign,' ',nc);
    else if ((nc -= strlen(sign)) > 0)
      memset(sign+strlen(sign),' ',nc);
  }
  else
    memset(sign,' ',nc);

  // Return the number of arguments of this function
  return FFaUserFuncPlugin::instance()->getFuncName(funcId);
}


INTEGER_FUNCTION(ffauf_getnopar,FFAUF_GETNOPAR) (const int& funcId)
{
  return FFaUserFuncPlugin::instance()->getParName(funcId);
}


INTEGER_FUNCTION(ffauf_getflag,FFAUF_GETFLAG) (const int& funcId)
{
  return FFaUserFuncPlugin::instance()->getFlag(funcId);
}


DOUBLE_FUNCTION(ffauf_getvalue,FFAUF_GETVALUE) (const int& baseId,
						const int& funcId,
						const double* par,
						const double* args, int& ierr)
{
  return FFaUserFuncPlugin::instance()->getValue(baseId,funcId,par,args,ierr);
}


DOUBLE_FUNCTION(ffauf_getdiff,FFAUF_GETDIFF) (const int& baseId,
					      const int& funcId, const int& ia,
					      const double* par,
					      const double* args, int& ierr)
{
  return FFaUserFuncPlugin::instance()->getDiff(baseId,funcId,ia,par,args,ierr);
}


INTEGER_FUNCTION(ffauf_wave,FFAUF_WAVE) (const int& baseId, const int& funcId,
                                         const double& d, const double& g,
                                         const double* par, const double* args,
                                         double& h, double* u, double* du)
{
  return FFaUserFuncPlugin::instance()->wave(baseId,funcId,d,g,par,args,h,u,du);
}
