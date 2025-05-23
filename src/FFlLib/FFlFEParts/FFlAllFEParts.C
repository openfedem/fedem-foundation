// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#include "FFlLib/FFlFEParts/FFlAllFEParts.H"
#include "FFlLib/FFlMemPool.H"

#ifdef FF_NAMESPACE
using namespace FF_NAMESPACE;
#endif

//! Set to \e true when initialized, to avoid initializing more than once.
static bool initialized = false;


void FFl::initAllElements()
{
  if (initialized) return;

  FFlFEAttributeSpec::initTypeID();

  FFlNode::init();
  FFlRGD::init();
  FFlRBAR::init();
  FFlWAVGM::init();
  FFlCMASS::init();
  FFlSPRING::init();
  FFlRSPRING::init();
  FFlBUSH::init();
  FFlBEAM2::init();
  FFlBEAM3::init();
  FFlTRI3::init();
  FFlTRI6::init();
  FFlQUAD4::init();
  FFlQUAD8::init();
  FFlTET4::init();
  FFlTET10::init();
  FFlWEDG6::init();
  FFlWEDG15::init();
  FFlHEX8::init();
  FFlHEX20::init();
#ifdef FT_USE_STRAINCOAT
  FFlSTRCT3::init();
  FFlSTRCT6::init();
  FFlSTRCQ4::init();
  FFlSTRCQ8::init();
#endif

  FFlCFORCE::init();
  FFlCMOMENT::init();
  FFlFACELOAD::init();
  FFlSURFLOAD::init();

  FFlPMASS::init();
  FFlPSPRING::init();
  FFlPBUSHCOEFF::init();
  FFlPBUSHECCENT::init();
  FFlPORIENT::init();
  FFlPORIENT3::init();
  FFlPBEAMECCENT::init();
  FFlPBEAMSECTION::init();
  FFlPBEAMPIN::init();
  FFlPEFFLENGTH::init();
  FFlPSPRING::init();
  FFlPTHICK::init();
  FFlPCOMP::init();
  FFlPNSM::init();
  FFlPRGD::init();
  FFlPWAVGM::init();
  FFlPRBAR::init();
  FFlPMAT::init();
  FFlPMAT2D::init();
  FFlPMAT3D::init();
  FFlPMATSHELL::init();
  FFlPCOORDSYS::init();
#ifdef FT_USE_STRAINCOAT
  FFlPSTRC::init();
  FFlPHEIGHT::init();
  FFlPTHICKREF::init();
  FFlPFATIGUE::init();
#endif
#ifdef FT_USE_VISUALS
  FFlVAppearance::init();
  FFlVDetail::init();
#endif

  FFlGroup::init();

  initialized = true;
}


template<class T> void releaseElement()
{
  FFaSingelton<FFlFEElementTopSpec,T>::removeInstance();
  FFaSingelton<FFlFEAttributeSpec,T>::removeInstance();
  FFaSingelton<FFlTypeInfoSpec,T>::removeInstance();
}


template<class T> void releaseAttribute()
{
  FFaSingelton<FFlFEAttributeSpec,T>::removeInstance();
  FFaSingelton<FFlTypeInfoSpec,T>::removeInstance();
}


template<class T> void releaseTypeInfo()
{
  FFaSingelton<FFlTypeInfoSpec,T>::removeInstance();
}


void FFl::releaseAllElements()
{
  releaseTypeInfo<FFlNode>();
  releaseElement<FFlRGD>();
  releaseElement<FFlRBAR>();
  releaseElement<FFlWAVGM>();
  releaseElement<FFlCMASS>();
  releaseElement<FFlSPRING>();
  releaseElement<FFlRSPRING>();
  releaseElement<FFlBUSH>();
  releaseElement<FFlBEAM2>();
  releaseElement<FFlBEAM3>();
  releaseElement<FFlTRI3>();
  releaseElement<FFlTRI6>();
  releaseElement<FFlQUAD4>();
  releaseElement<FFlQUAD8>();
  releaseElement<FFlTET4>();
  releaseElement<FFlTET10>();
  releaseElement<FFlWEDG6>();
  releaseElement<FFlWEDG15>();
  releaseElement<FFlHEX8>();
  releaseElement<FFlHEX20>();
#ifdef FT_USE_STRAINCOAT
  releaseElement<FFlSTRCT3>();
  releaseElement<FFlSTRCT6>();
  releaseElement<FFlSTRCQ4>();
  releaseElement<FFlSTRCQ8>();
#endif
  ElementFactory::removeInstance();

  releaseTypeInfo<FFlCFORCE>();
  releaseTypeInfo<FFlCMOMENT>();
  releaseAttribute<FFlFACELOAD>();
  releaseAttribute<FFlSURFLOAD>();
  LoadFactory::removeInstance();

  releaseTypeInfo<FFlPMASS>();
  releaseTypeInfo<FFlPSPRING>();
  releaseTypeInfo<FFlPBUSHCOEFF>();
  releaseTypeInfo<FFlPBUSHECCENT>();
  releaseTypeInfo<FFlPORIENT>();
  releaseTypeInfo<FFlPORIENT3>();
  releaseTypeInfo<FFlPBEAMECCENT>();
  releaseTypeInfo<FFlPBEAMSECTION>();
  releaseTypeInfo<FFlPBEAMPIN>();
  releaseTypeInfo<FFlPEFFLENGTH>();
  releaseTypeInfo<FFlPSPRING>();
  releaseTypeInfo<FFlPTHICK>();
  releaseTypeInfo<FFlPCOMP>();
  releaseTypeInfo<FFlPNSM>();
  releaseTypeInfo<FFlPRGD>();
  releaseTypeInfo<FFlPWAVGM>();
  releaseTypeInfo<FFlPRBAR>();
  releaseTypeInfo<FFlPMAT>();
  releaseTypeInfo<FFlPMAT2D>();
  releaseTypeInfo<FFlPMAT3D>();
  releaseTypeInfo<FFlPMATSHELL>();
  releaseTypeInfo<FFlPCOORDSYS>();
#ifdef FT_USE_STRAINCOAT
  releaseAttribute<FFlPSTRC>();
  releaseAttribute<FFlPTHICKREF>();
  releaseTypeInfo<FFlPHEIGHT>();
  releaseTypeInfo<FFlPFATIGUE>();
#endif
  AttributeFactory::removeInstance();

#ifdef FT_USE_VISUALS
  releaseTypeInfo<FFlVAppearance>();
  releaseTypeInfo<FFlVDetail>();
  VisualFactory::removeInstance();
#endif

  releaseTypeInfo<FFlGroup>();

  FFlMemPool::deleteAllLinkMemPools();

  initialized = false;
}
