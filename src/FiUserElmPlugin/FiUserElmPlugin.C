// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

/*!
  \file FiUserElmPlugin.C
  \brief User-defined elements plugin.
*/

#include "FiUserElmPlugin.H"
#include <iostream>
#include <cstring>


/*!
  \brief Static helper to replace trailing white-spaces a with null-character.
  \details This function is needed when the user-defined element is implemented
  in a Fortran library, from which an output character string is padded with
  trailing spaces instead of being null-terminated.
  \callergraph
*/

static void nullTerminate (char* str, int nchar)
{
  while (--nchar > 0)
    if (!isspace(str[nchar]))
    {
      str[nchar+1] = '\0';
      return;
    }
}


//! \callergraph \callgraph
bool FiUserElmPlugin::validate(const std::string& lib, int nchar, char* sign)
{
  if (nchar > 0 && sign) sign[0] = '\0';
  if (this->areLibsLoaded()) return false;
  if (!this->load(lib,true)) return false;

  bool valid = this->getSign(nchar,sign,true);
  this->unload(lib,true);
  return valid;
}


//! \callergraph \callgraph
bool FiUserElmPlugin::getSign(int nchar, char* sign, bool silence)
{
  if (!this->areLibsLoaded()) return false;

  LanguageBinding lang = Undefined;
#if defined(win32) || defined(win64)
  DLPROC p = this->getProcAddr("ueGetSignature","UE_GET_SIGN",lang,silence);
#else
  DLPROC p = this->getProcAddr("ueGetSignature","ue_get_sign_",lang,silence);
#endif

  bool stat = false;
  if (lang == C)
  {
    bool(*f)(int,char*) = reinterpret_cast<bool(*)(int,char*)>(p);
    if (f) stat = nchar > 0 && sign ? f(nchar,sign) : true;
  }
  else if (lang == Fortran)
  {
    bool(*f)(char*,int) = reinterpret_cast<bool(*)(char*,int)>(p);
    if (f && (stat = nchar > 0 && sign ? f(sign,nchar) : true))
      nullTerminate(sign,nchar-1);
  }

  if (!silence && !stat)
    std::cerr <<"FiUserElmPlugin: ueGetSignature function not found."
              << std::endl;

  return stat;
}


//! \callergraph \callgraph
int FiUserElmPlugin::getElementTypes(int maxUE, int* eType)
{
  if (!this->areLibsLoaded()) return -99;

  LanguageBinding lang = Undefined;
#if defined(win32) || defined(win64)
  DLPROC p = this->getProcAddr("ueGetElements","UE_GET_ELMS",lang);
#else
  DLPROC p = this->getProcAddr("ueGetElements","ue_get_elms_",lang);
#endif

  if (lang == C)
  {
    int(*f)(int,int*) = reinterpret_cast<int(*)(int,int*)>(p);
    if (f) return f(maxUE,eType);
  }
  else if (lang == Fortran)
  {
    int(*f)(const int&,int*) = reinterpret_cast<int(*)(const int&,int*)>(p);
    if (f) return f(maxUE,eType);
  }

  std::cerr <<"FiUserElmPlugin: ueGetElements function not found."<< std::endl;
  return -999;
}


//! \callergraph \callgraph
int FiUserElmPlugin::getTypeName(int eType, int nchar, char* name)
{
  if (!this->areLibsLoaded()) return -99;

  LanguageBinding lang = Undefined;
#if defined(win32) || defined(win64)
  DLPROC p = this->getProcAddr("ueGetTypeName","UE_TYPE_NAME",lang);
#else
  DLPROC p = this->getProcAddr("ueGetTypeName","ue_type_name_",lang);
#endif

  int stat = -999;
  if (lang == C)
  {
    int(*f)(int,int,char*) =
      reinterpret_cast<int(*)(int,int,char*)>(p);
    if (f) stat = f(eType,nchar,name);
  }
  else if (lang == Fortran)
  {
    int(*f)(const int&,char*,int) =
      reinterpret_cast<int(*)(const int&,char*,int)>(p);
    if (f && (stat = f(eType,name,nchar)))
      nullTerminate(name,nchar-1);
  }

  if (stat == -999)
    std::cerr <<"FiUserElmPlugin: ueGetTypeName function not found."
              << std::endl;
  return stat;
}


//! \callergraph \callgraph
int FiUserElmPlugin::init(const double* gdata)
{
  LanguageBinding lang = Undefined;
#if defined(win32) || defined(win64)
  DLPROC p = this->getProcAddr("ueGinit","UE_GINIT",lang);
#else
  DLPROC p = this->getProcAddr("ueGinit","ue_ginit_",lang);
#endif

  if (lang == C || lang == Fortran)
  {
    int(*f)(const double*) = reinterpret_cast<int(*)(const double*)>(p);
    if (f) return f(gdata);
  }

  return 0;
}


//! \cond DO_NOT_DOCUMENT
#define INIT_CARGS int,int,int,int,const double*,const double*,int*,double*
#define INIT_FARGS const int&,const int&,const int&,const int&, \
    const double*,const double*,int*,double*
//! \endcond

//! \callergraph \callgraph
int FiUserElmPlugin::init(int eId, int eType, int nenod, int nedof,
                          int& niwork, int& nrwork)
{
  LanguageBinding lang = Undefined;
#if defined(win32) || defined(win64)
  DLPROC p = this->getProcAddr("ueInit","UE_INIT",lang);
#else
  DLPROC p = this->getProcAddr("ueInit","ue_init_",lang);
#endif

  int ierr = -999;
  int work[2] = { 0, 0 };
  if (lang == C)
  {
    int(*f)(INIT_CARGS) = reinterpret_cast<int(*)(INIT_CARGS)>(p);
    if (f) ierr = f(eId,eType,nenod,nedof,NULL,NULL,work,NULL);
  }
  else if (lang == Fortran)
  {
    double dummy = 0.0;
    int(*f)(INIT_FARGS) = reinterpret_cast<int(*)(INIT_FARGS)>(p);
    if (f) ierr = f(eId,eType,nenod,nedof,&dummy,&dummy,work,&dummy);
  }
  if (ierr == -999)
    std::cerr <<"FiUserElmPlugin: ueInit function not found."<< std::endl;
  else if (ierr < 0)
    std::cerr <<"FiUserElmPlugin: ueInit function failed."<< std::endl;

  niwork = work[0];
  nrwork = work[1];
  return ierr;
}


//! \callergraph \callgraph
int FiUserElmPlugin::init(int eId, int eType, int nenod, int nedof,
                          const double* X, const double* T,
                          int* iwork, double* rwork)
{
  LanguageBinding lang = Undefined;
#if defined(win32) || defined(win64)
  DLPROC p = this->getProcAddr("ueInit","UE_INIT",lang);
#else
  DLPROC p = this->getProcAddr("ueInit","ue_init_",lang);
#endif

  if (lang == C)
  {
    int(*f)(INIT_CARGS) = reinterpret_cast<int(*)(INIT_CARGS)>(p);
    if (f) return f(eId,eType,nenod,nedof,X,T,iwork,rwork);
  }
  else if (lang == Fortran)
  {
    int(*f)(INIT_FARGS) = reinterpret_cast<int(*)(INIT_FARGS)>(p);
    if (f) return f(eId,eType,nenod,nedof,X,T,iwork,rwork);
  }

  std::cerr <<"FiUserElmPlugin: ueInit function not found."<< std::endl;
  return -999;
}


//! \cond DO_NOT_DOCUMENT
#define MASS_CARGS int,int,int,const double*,int*,double*,double&
#define MASS_FARGS const int&,const int&,const int&, \
    const double*,int*,double*,double&
//! \endcond

//! \callergraph \callgraph
int FiUserElmPlugin::mass(int eId, int eType, int nenod,
                          const double* X, int* iwork, double* rwork,
                          double& mass)
{
  LanguageBinding lang = Undefined;
#if defined(win32) || defined(win64)
  DLPROC p = this->getProcAddr("ueMass","UE_MASS",lang);
#else
  DLPROC p = this->getProcAddr("ueMass","ue_mass_",lang);
#endif

  if (lang == C)
  {
    int(*f)(MASS_CARGS) = reinterpret_cast<int(*)(MASS_CARGS)>(p);
    if (f) return f(eId,eType,nenod,X,iwork,rwork,mass);
  }
  else if (lang == Fortran)
  {
    int(*f)(MASS_FARGS) = reinterpret_cast<int(*)(MASS_FARGS)>(p);
    if (f) return f(eId,eType,nenod,X,iwork,rwork,mass);
  }

  mass = 0.0;
  return 0;
}


//! \cond DO_NOT_DOCUMENT
enum { idUpdate = 0, idOrigin = 1, idResult = 2 };

#define UPDATE_CARGS int,int,int,int, \
    const double*,const double*,const double*,const double*, \
    int*,double*,double*,double*,double*,double*,double*,double*,double*, \
    double,double,int,int
#define UPDATE_FARGS const int&,const int&,const int&,const int&, \
    const double*,const double*,const double*,const double*, \
    int*,double*,double*,double*,double*,double*,double*,double*,double*, \
    const double&,const double&,const int&,const int&
//! \endcond

/*!
  This function is invoked once per element within the Newton iteration loop.
  It evaluates the updated tangent matrices and associated force vectors
  of the current linearized dynamic equilibrium equation.

  \callergraph \callgraph
*/

int FiUserElmPlugin::update(int eId, int eType, int nenod, int nedof,
                            const double* X, const double* T,
                            const double* V, const double* A,
                            int* iwork, double* rwork,
                            double* Kt, double* Ct, double* M,
                            double* Fs, double* Fd, double* Fi, double* Q,
                            double t, double dt, int istep, int iter)
{
  LanguageBinding lang = Undefined;
#if defined(win32) || defined(win64)
  DLPROC p = this->getProcAddress("ueUpdate","UE_UPDATE",lang,idUpdate);
#else
  DLPROC p = this->getProcAddress("ueUpdate","ue_update_",lang,idUpdate);
#endif

  if (lang == C)
  {
    int(*f)(UPDATE_CARGS) = reinterpret_cast<int(*)(UPDATE_CARGS)>(p);
    if (f) return f(eId,eType,nenod,nedof,X,T,V,A,iwork,rwork,
                    Kt,Ct,M,Fs,Fd,Fi,Q,t,dt,istep,iter);
  }
  else if (lang == Fortran)
  {
    int(*f)(UPDATE_FARGS) = reinterpret_cast<int(*)(UPDATE_FARGS)>(p);
    if (f) return f(eId,eType,nenod,nedof,X,T,V,A,iwork,rwork,
                    Kt,Ct,M,Fs,Fd,Fi,Q,t,dt,istep,iter);
  }

  std::cerr <<"FiUserElmPlugin: ueUpdate function not found."<< std::endl;
  return -999;
}


//! \cond DO_NOT_DOCUMENT
#define ORIGIN_CARGS int,int,int, \
    const double*,const double*,const int*,const double*,double*
#define ORIGIN_FARGS const int&,const int&,const int&, \
    const double*,const double*,const int*,const double*,double*
//! \endcond

/*!
  This function is invoked once per element in pre- and post-processing tasks,
  requiring the position of current element.
  It does not affect the response simulation.

 \callergraph \callgraph
*/

int FiUserElmPlugin::origin(int eId, int eType, int nenod,
                            const double* X, const double* T,
                            const int* iwork, const double* rwork, double* Tlg)
{
  LanguageBinding lang = Undefined;
#if defined(win32) || defined(win64)
  DLPROC p = this->getProcAddress("ueOrigin","UE_ORIGIN",lang,idOrigin);
#else
  DLPROC p = this->getProcAddress("ueOrigin","ue_origin_",lang,idOrigin);
#endif

  int ierr = 99;
  if (lang == C)
  {
    int(*f)(ORIGIN_CARGS) = reinterpret_cast<int(*)(ORIGIN_CARGS)>(p);
    if (f) ierr = f(eId,eType,nenod,X,T,iwork,rwork,Tlg);
  }
  else if (lang == Fortran)
  {
    int(*f)(ORIGIN_FARGS) = reinterpret_cast<int(*)(ORIGIN_FARGS)>(p);
    if (f) ierr = f(eId,eType,nenod,X,T,iwork,rwork,Tlg);
  }
  if (ierr <= 0) return ierr;

  // Default coordinate system, identity matrix = global coordinate system
  memset(Tlg,0,12*sizeof(double));
  for (int i = 0; i < 9; i+= 4) Tlg[i] = 1.0;
  return 0;
}


//! \cond DO_NOT_DOCUMENT
#define RESULT_CARGS int,int,int,const int*,const double*,double&,int,char*
#define RESULT_FARGS const int&,const int&,const int&, \
    const int*,const double*,double&,char*,int
//! \endcond

/*!
  This function is invoked once per element as a pre-processing task,
  when saving results to file.

  \callergraph \callgraph
*/

int FiUserElmPlugin::result(int eId, int eType, int idx,
                            const int* iwork, const double* rwork,
                            double& value)
{
  LanguageBinding lang = Undefined;
#if defined(win32) || defined(win64)
  DLPROC p = this->getProcAddress("ueResult","UE_RESULT",lang,idResult);
#else
  DLPROC p = this->getProcAddress("ueResult","ue_result_",lang,idResult);
#endif

  if (lang == C)
  {
    int(*f)(RESULT_CARGS) = reinterpret_cast<int(*)(RESULT_CARGS)>(p);
    if (f) return f(eId,eType,idx,iwork,rwork,value,0,NULL);
  }
  else if (lang == Fortran)
  {
    int(*f)(RESULT_FARGS) = reinterpret_cast<int(*)(RESULT_FARGS)>(p);
    if (f) return f(eId,eType,idx,iwork,rwork,value,NULL,0);
  }

  return 0;
}


/*!
  This function is invoked once as per element as a post-processing task
  after each time increment, when saving results to file.

  \callergraph \callgraph
*/

int FiUserElmPlugin::result(int eId, int eType, int idx,
                            const int* iwork, const double* rwork,
                            int nchar, char* name)
{
  LanguageBinding lang = Undefined;
#if defined(win32) || defined(win64)
  DLPROC p = this->getProcAddress("ueResult","UE_RESULT",lang,idResult);
#else
  DLPROC p = this->getProcAddress("ueResult","ue_result_",lang,idResult);
#endif

  int stat = 0;
  double dummy;
  if (lang == C)
  {
    int(*f)(RESULT_CARGS) = reinterpret_cast<int(*)(RESULT_CARGS)>(p);
    if (f) stat = f(eId,eType,idx,iwork,rwork,dummy,nchar,name);
  }
  else if (lang == Fortran)
  {
    int(*f)(RESULT_FARGS) = reinterpret_cast<int(*)(RESULT_FARGS)>(p);
    if (f && (stat = f(eId,eType,idx,iwork,rwork,dummy,name,nchar)))
      nullTerminate(name,nchar-1);
  }

  return stat;
}
