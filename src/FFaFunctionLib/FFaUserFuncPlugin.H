// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#ifndef FFA_USER_FUNC_PLUGIN_H
#define FFA_USER_FUNC_PLUGIN_H

#include "FFaLib/FFaOS/FFaDynamicLibraryBase.H"
#include "FFaLib/FFaPatterns/FFaSingelton.H"


class FFaUserFuncPlugin : public FFaDynamicLibraryBase,
			  public FFaSingelton<FFaUserFuncPlugin>
{
protected:
  FFaUserFuncPlugin() {}
  virtual ~FFaUserFuncPlugin() {}

public:
  bool validate(const std::string& libName, int nchar = 0, char* sign = NULL);
  bool getSign(int nchar, char* sign) const;

  int getFuncs(int maxUF, int* funcId, bool silence = false) const;
  int getFuncName(int id, int nchar = 0, char* name = NULL) const;
  int getParName(int id, int ipar = 1, int nchar = 0, char* name = NULL) const;
  double getDefaultParVal(int id, int ipar) const;
  const char** getPixmap(int id) const;
  int getFlag(int id) const;

  double getValue(int bId, int fId, const double* params,
                  double x, int& ierr) const;
  double getValue(int bId, int fId, const double* params,
		  const double* args, int& err) const;
  double getDiff(int bId, int fId, int ia, const double* params,
		 const double* args, int& err) const;
  int wave(int bId, int fId, double d, double g,
           const double* params, const double* args,
           double& h, double* u, double* du) const;

private:
  friend class FFaSingelton<FFaUserFuncPlugin>;
};

#endif
