// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#ifndef FFL_STR_COAT_H
#define FFL_STR_COAT_H

#include "FFlLib/FFlElementBase.H"
#include "FFlLib/FFlFEElementTopSpec.H"
#include "FFlLib/FFlFEAttributeSpec.H"
#include "FFlLib/FFlTypeInfoSpec.H"
#ifdef FT_USE_MEMPOOL
#include "FFaLib/FFaPatterns/FFaMemPool.H"
#endif

#ifdef FF_NAMESPACE
namespace FF_NAMESPACE {
#endif


class FFlStrainCoatBase : public FFlElementBase
{
public:
  FFlStrainCoatBase(int ID) : FFlElementBase(ID) {}
  virtual ~FFlStrainCoatBase() {}

  // Reference to underlying finite element
  virtual void setFElement(FFlElementBase* elm) { myFElm = elm; }
  virtual void setFElement(const int ID) { myFElm = ID; }
  virtual bool resolveElmRef(const std::vector<FFlElementBase*>& possibleElms,
			     bool suppressErrmsg = false);
  virtual FFlElementBase* getFElement() const;

protected:
  FFlReference<FFlElementBase> myFElm;
};


class FFlSTRCT3 : public FFlStrainCoatBase
{
public:
  FFlSTRCT3(int ID) : FFlStrainCoatBase(ID) {}
  virtual ~FFlSTRCT3() {}

  static void init();

  FFL_FE_ELEMENT_FACTORY_INIT(FFlSTRCT3);
  FFL_FE_ELEMENT_TOP_SPEC(FFlSTRCT3);
  FFL_FE_ATTRIBUTE_SPEC(FFlSTRCT3);
  FFL_TYPE_INFO(FFlSTRCT3);

  virtual FaMat33 getGlobalizedElmCS() const;

#ifdef FT_USE_MEMPOOL
  FFA_MAKE_MEMPOOL;
#endif
};


class FFlSTRCQ4 : public FFlStrainCoatBase
{
public:
  FFlSTRCQ4(int ID) : FFlStrainCoatBase(ID) {}
  virtual ~FFlSTRCQ4() {}

  static void init();

  FFL_FE_ELEMENT_FACTORY_INIT(FFlSTRCQ4);
  FFL_FE_ELEMENT_TOP_SPEC(FFlSTRCQ4);
  FFL_FE_ATTRIBUTE_SPEC(FFlSTRCQ4);
  FFL_TYPE_INFO(FFlSTRCQ4);

  virtual FaMat33 getGlobalizedElmCS() const;

#ifdef FT_USE_MEMPOOL
  FFA_MAKE_MEMPOOL;
#endif
};


class FFlSTRCT6 : public FFlStrainCoatBase
{
public:
  FFlSTRCT6(int ID) : FFlStrainCoatBase(ID) {}
  virtual ~FFlSTRCT6() {}

  static void init();

  FFL_FE_ELEMENT_FACTORY_INIT(FFlSTRCT6);
  FFL_FE_ELEMENT_TOP_SPEC(FFlSTRCT6);
  FFL_FE_ATTRIBUTE_SPEC(FFlSTRCT6);
  FFL_TYPE_INFO(FFlSTRCT6);

  virtual FaMat33 getGlobalizedElmCS() const;

#ifdef FT_USE_MEMPOOL
  FFA_MAKE_MEMPOOL;
#endif
};


class FFlSTRCQ8 : public FFlStrainCoatBase
{
public:
  FFlSTRCQ8(int ID) : FFlStrainCoatBase(ID) {}
  virtual ~FFlSTRCQ8() {}

  static void init();

  FFL_FE_ELEMENT_FACTORY_INIT(FFlSTRCQ8);
  FFL_FE_ELEMENT_TOP_SPEC(FFlSTRCQ8);
  FFL_FE_ATTRIBUTE_SPEC(FFlSTRCQ8);
  FFL_TYPE_INFO(FFlSTRCQ8);

  virtual FaMat33 getGlobalizedElmCS() const;

#ifdef FT_USE_MEMPOOL
  FFA_MAKE_MEMPOOL;
#endif
};

#ifdef FF_NAMESPACE
} // namespace
#endif
#endif
