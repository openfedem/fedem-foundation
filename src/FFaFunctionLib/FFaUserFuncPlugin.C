// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#include "FFaUserFuncPlugin.H"
#include <iostream>
#include <cstdio>


static void nullTerminate (char* str, int nchar)
{
  while (--nchar > 0)
    if (!isspace(str[nchar]))
    {
      str[nchar+1] = '\0';
      return;
    }
}


bool FFaUserFuncPlugin::validate(const std::string& lib, int nchar, char* sign)
{
  if (nchar > 0 && sign) sign[0] = '\0';
  if (this->areLibsLoaded()) return false;
  if (!this->load(lib,true)) return false;

  int funcId = 0;
  int nFunc = this->getFuncs(1,&funcId,true);
  if (nFunc >= 0 && nchar > 0 && sign)
    this->getSign(nchar,sign);

  this->unload(lib,true);
  return nFunc >= 0;
}


bool FFaUserFuncPlugin::getSign(int nchar, char* sign) const
{
  sign[0] = '\0';
  if (!this->areLibsLoaded()) return false;

  LanguageBinding lang = Undefined;
#if defined(win32) || defined(win64)
  DLPROC p = this->getProcAddr("ufGetSignature","UF_GET_SIGN",lang);
#else
  DLPROC p = this->getProcAddr("ufGetSignature","uf_get_sign_",lang);
#endif

  bool stat = false;
  if (lang == C)
  {
    bool(*f)(int,char*) = reinterpret_cast<bool(*)(int,char*)>(p);
    if (f) stat = f(nchar,sign);
  }
  else if (lang == Fortran)
  {
    bool(*f)(char*,int) = reinterpret_cast<bool(*)(char*,int)>(p);
    if (f && (stat = f(sign,nchar)))
      nullTerminate(sign,nchar-1);
  }

  return stat;
}


int FFaUserFuncPlugin::getFuncs(int maxUF, int* funcId, bool silence) const
{
  if (!this->areLibsLoaded()) return -99;

  LanguageBinding lang = Undefined;
#if defined(win32) || defined(win64)
  DLPROC p = this->getProcAddr("ufGetFuncs","UF_GET_FUNCS",lang,silence);
#else
  DLPROC p = this->getProcAddr("ufGetFuncs","uf_get_funcs_",lang,silence);
#endif

  if (lang == C)
  {
    int(*f)(int,int*) = reinterpret_cast<int(*)(int,int*)>(p);
    if (f) return f(maxUF,funcId);
  }
  else if (lang == Fortran)
  {
    int(*f)(const int&,int*) = reinterpret_cast<int(*)(const int&,int*)>(p);
    if (f) return f(maxUF,funcId);
  }

  if (!silence)
    std::cerr <<"FFaUserFuncPlugin: ufGetFuncs function not found."<< std::endl;

  return -999;
}


int FFaUserFuncPlugin::getFuncName(int id, int nchar, char* name) const
{
  if (!this->areLibsLoaded()) return -99;

  LanguageBinding lang = Undefined;
#if defined(win32) || defined(win64)
  DLPROC p = this->getProcAddr("ufGetFuncName","UF_GET_FUNC_NAME",lang);
#else
  DLPROC p = this->getProcAddr("ufGetFuncName","uf_get_func_name_",lang);
#endif

  int stat = -999;
  if (lang == C)
  {
    int(*f)(int,int,char*) =
      reinterpret_cast<int(*)(int,int,char*)>(p);
    if (f) stat = f(id,nchar,name);
  }
  else if (lang == Fortran)
  {
    char c;
    int(*f)(const int&,char*,int) =
      reinterpret_cast<int(*)(const int&,char*,int)>(p);
    if (f && (stat = nchar > 0 && name ? f(id,name,nchar) : f(id,&c,1)))
      nullTerminate(name,nchar-1);
  }

  if (stat == -999)
    std::cerr <<"FFaUserFuncPlugin: ufGetFuncName function not found."
              << std::endl;
  return stat;
}


int FFaUserFuncPlugin::getParName(int id, int ipar, int nchar, char* name) const
{
  if (!this->areLibsLoaded()) return -99;

  LanguageBinding lang = Undefined;
#if defined(win32) || defined(win64)
  DLPROC p = this->getProcAddr("ufGetParName","UF_GET_PAR_NAME",lang,true);
#else
  DLPROC p = this->getProcAddr("ufGetParName","uf_get_par_name_",lang,true);
#endif

  int stat = 0;
  if (lang == C)
  {
    int(*f)(int,int,int,char*) =
      reinterpret_cast<int(*)(int,int,int,char*)>(p);
    if (f) stat = f(id,ipar,nchar,name);
  }
  else if (lang == Fortran)
  {
    char c;
    int(*f)(const int&,const int&,char*,int) =
      reinterpret_cast<int(*)(const int&,const int&,char*,int)>(p);
    if (f && (stat = nchar>0 && name ? f(id,ipar,name,nchar) : f(id,ipar,&c,1)))
      nullTerminate(name,nchar-1);
  }

  // Function ufGetParName is optional, so no error message if not present
  return stat;
}


double FFaUserFuncPlugin::getDefaultParVal(int id, int ipar) const
{
  if (!this->areLibsLoaded()) return 0.0;

  LanguageBinding lang = Undefined;
#if defined(win32) || defined(win64)
  DLPROC p = this->getProcAddr("ufGetDefaultParVal","UF_GET_DEF_PAR_VAL",
                               lang,true);
#else
  DLPROC p = this->getProcAddr("ufGetDefaultParVal","uf_get_def_par_val_",
                               lang,true);
#endif

  if (lang == C)
  {
    double(*f)(int,int) = reinterpret_cast<double(*)(int,int)>(p);
    if (f) return f(id,ipar);
  }
  else if (lang == Fortran)
  {
    double(*f)(const int&,const int&) =
      reinterpret_cast<double(*)(const int&,const int&)>(p);
    if (f) return f(id,ipar);
  }

  // Function ufGetDefaultParVal is optional, so no error message if not present
  return 0.0;
}


const char** FFaUserFuncPlugin::getPixmap(int id) const
{
  if (!this->areLibsLoaded()) return NULL;

  // Only a C++ interface is available for this (optional) function
  DLPROC p = this->getProcAddr("ufGetPixmap",true);
  const char**(*f)(int) = reinterpret_cast<const char**(*)(int)>(p);
  if (f) return f(id);

  // Try to get pixmap from file name instead
  char fileName[8192];
  p = this->getProcAddr("ufGetPixmapFileName",true);
  int(*ff)(int,int,char*) = reinterpret_cast<int(*)(int,int,char*)>(p);
  if (!ff || !ff(id,8192,fileName)) return NULL;

  FILE* fimg = fopen(fileName,"rb");
  if (!fimg) return NULL; // Non-existing file

  // Store the xpm bytes in a static array
  static char xpmData[65536];
  size_t size = fread(xpmData,1,65536,fimg);
  fclose(fimg);

  if (size < 3) return NULL; // Empty file

  // Convert xpm to in-memory format
  static const char* xmpConverted[1024];
  char* p1 = NULL;
  int line = 0;
  for (size_t i = 0; i < size && line < 1024; i++)
    if (xpmData[i] == '"')
    {
      if (p1) // End of xpm line
      {
        xmpConverted[line] = p1;
        xpmData[i] = '\0';
        p1 = NULL;
        line++;
      }
      else if (i+1 < size)
        p1 = xpmData + (i+1); // start of xpm line
    }

  return xmpConverted;
}


int FFaUserFuncPlugin::getFlag(int id) const
{
  LanguageBinding lang = Undefined;
#if defined(win32) || defined(win64)
  DLPROC p = this->getProcAddr("ufGetFlag","UF_GET_FLAG",lang,true);
#else
  DLPROC p = this->getProcAddr("ufGetFlag","uf_get_flag_",lang,true);
#endif

  if (lang == C)
  {
    int(*f)(int) = reinterpret_cast<int(*)(int)>(p);
    if (f) return f(id);
  }
  else if (lang == Fortran)
  {
    int(*f)(const int&) = reinterpret_cast<int(*)(const int&)>(p);
    if (f) return f(id);
  }

  // Function ufGetFlag is optional, default flag value is zero
  return 0;
}


enum { idGetValue = 0, idGetDiff = 1, idWave = 2 };


double FFaUserFuncPlugin::getValue(int bId, int fId,
                                   const double* params,
                                   double x, int& err) const
{
  static std::vector<double> args(10,0.0); args.front() = x;
  return this->getValue(bId,fId,params,&args.front(),err);
}


double FFaUserFuncPlugin::getValue(int bId, int fId,
                                   const double* params,
                                   const double* args, int& err) const
{
  LanguageBinding lang = Undefined;
#if defined(win32) || defined(win64)
  DLPROC p = this->getProcAddress("ufGetValue","UF_GET_VALUE",lang,idGetValue);
#else
  DLPROC p = this->getProcAddress("ufGetValue","uf_get_value_",lang,idGetValue);
#endif

  if (lang == C)
  {
    double(*f)(int,int,const double*,const double*,int&) =
      reinterpret_cast<double(*)(int,int,const double*,const double*,int&)>(p);
    if (f) return f(bId,fId,params,args,err);
  }
  else if (lang == Fortran)
  {
    double(*f)(const int&,const int&,const double*,const double*,int&) =
      reinterpret_cast<double(*)(const int&,const int&,
                                 const double*,const double*,int&)>(p);
    if (f) return f(bId,fId,params,args,err);
  }

  std::cerr <<"FFaUserFuncPlugin: ufGetValue function not found."<< std::endl;
  err = -999;
  return 0.0;
}


double FFaUserFuncPlugin::getDiff(int bId, int fId, int ia,
                                  const double* params,
                                  const double* args, int& err) const
{
  LanguageBinding lang = Undefined;
#if defined(win32) || defined(win64)
  DLPROC p = this->getProcAddress("ufGetDiff","UF_GET_DIFF",
                                  lang,idGetDiff);
#else
  DLPROC p = this->getProcAddress("ufGetDiff","uf_get_diff_",
                                  lang,idGetDiff);
#endif

  if (lang == C)
  {
    double(*f)(int,int,int,const double*,const double*,int&) =
      reinterpret_cast<double(*)(int,int,int,
                                 const double*,const double*,int&)>(p);
    if (f) return f(bId,fId,ia,params,args,err);
  }
  else if (lang == Fortran)
  {
    double(*f)(const int&,const int&,const int&,
               const double*,const double*,int&) =
      reinterpret_cast<double(*)(const int&,const int&,const int&,
                                 const double*,const double*,int&)>(p);
    if (f) return f(bId,fId,ia,params,args,err);
  }

  // Function ufGetDiff is optional, so no error message if not present
  err = 0;
  return 0.0;
}


int FFaUserFuncPlugin::wave(int bId, int fId, double d, double g,
                            const double* params, const double* args,
                            double& h, double* u, double* du) const
{
  LanguageBinding lang = Undefined;
  DLPROC          proc = NULL;
#if defined(win32) || defined(win64)
  const char* ftn_wave = "UF_WAVE";
#else
  const char* ftn_wave = "uf_wave_";
#endif
  if (args)
    proc = this->getProcAddress("ufWave",ftn_wave,lang,idWave);
  else
    proc = this->getProcAddress("ufWave",ftn_wave,lang,true);

  if (lang == C)
  {
    int(*f)(int,int,double,double,const double*,const double*,
            double&,double*,double*) =
      reinterpret_cast<int(*)(int,int,double,double,const double*,const double*,
                              double&,double*,double*)>(proc);
    if (f) return f(bId,fId,d,g,params,args,h,u,du);
  }
  else if (lang == Fortran)
  {
    int(*f)(const int&,const int&,const double&,const double&,
            const double*,const double*,double&,double*,double*) =
      reinterpret_cast<int(*)(const int&,const int&,const double&,const double&,
                              const double*,const double*,
                              double&,double*,double*)>(proc);
    if (f) return f(bId,fId,d,g,params,args,h,u,du);
  }

  // Function ufWave is optional, so no error message if not present.
  return -999; // But flag non-existance by return value -999.
}
