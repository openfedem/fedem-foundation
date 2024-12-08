// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#ifndef FFL_VISUAL_BASE_H
#define FFL_VISUAL_BASE_H

#include <vector>

#include "FFlLib/FFlPartBase.H"
#include "FFaLib/FFaPatterns/FFaGenericFactory.H"

class FFlFieldBase;
class FFaCheckSum;

#ifdef FF_NAMESPACE
namespace FF_NAMESPACE {
#endif


class FFlVisualBase : public FFlPartBase
{
protected:
  FFlVisualBase(int ID = 0) : FFlPartBase(ID) {}
  FFlVisualBase(const FFlVisualBase& obj) : FFlPartBase(obj) {}
  FFlVisualBase& operator=(const FFlVisualBase&) = delete;

public:
  virtual ~FFlVisualBase() {}

  const std::string& getTypeName() const;

  std::vector<FFlFieldBase*>::const_iterator begin() const { return myFields.begin(); }
  std::vector<FFlFieldBase*>::const_iterator end()   const { return myFields.end(); }

  void calculateChecksum(FFaCheckSum* cs, int csMask = 0) const;

  virtual FFlVisualBase* clone() const = 0;

protected:
  void addField(FFlFieldBase& field) { myFields.push_back(&field); }

private:
  std::vector<FFlFieldBase*> myFields;
};


using VisualFactory = FFaGenericFactory<FFlVisualBase>;


#define FFL_VISUAL_FACTORY_INIT(clName)			\
public:							\
  static void create(int newID, FFlVisualBase*& created)\
  { created = new clName(newID); }			\
  clName& operator=(const clName&) = delete;		\
  virtual FFlVisualBase* clone() const 	     	 	\
  { return new clName(*this); }

#ifdef FF_NAMESPACE
} // namespace
#endif
#endif
