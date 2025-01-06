// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#ifndef FFL_VISUAL_REFS_H
#define FFL_VISUAL_REFS_H

#include "FFlLib/FFlReference.H"
#include "FFlLib/FFlVisualBase.H"
#include <vector>
#include <string>

class FFaCheckSum;

#ifdef FF_NAMESPACE
namespace FF_NAMESPACE {
#endif

class FFlVAppearance;
class FFlVDetail;


class FFlVisualRefs
{
protected:
  FFlVisualRefs() {}
  FFlVisualRefs(const FFlVisualRefs& obj) { this->useVisualsFrom(&obj); }
  FFlVisualRefs& operator=(const FFlVisualRefs&) = delete;

public:
  virtual ~FFlVisualRefs() {}

  void useVisualsFrom(const FFlVisualRefs* obj);

  void setAppearance(const FFlVisualBase* app) { myApp = app; }
  void setAppearance(int id) { myApp = id; }

  void setDetail(const FFlVisualBase* det) { myDetail = det; }
  void setDetail(int id) { myDetail = id; }

  bool setVisual(const FFlVisualBase* vis);
  bool setVisual(const std::string& type, int ID);

  bool resolveVisuals(const std::vector<FFlVisualBase*>& possibleVisRefs,
		      bool suppressErrmsg = false);

  bool hasAppearance() const { return myApp.isResolved(); }
  bool hasDetail() const { return myDetail.isResolved(); }

  FFlVAppearance* getAppearance() const;
  FFlVDetail*     getDetail() const;

  FFlVisualBase* getVisualAppearance() const;
  FFlVisualBase* getVisualDetail() const;

  bool isVisible() const;

protected:
  void checksum(FFaCheckSum* cs, int cstype) const;

  FFlReference<FFlVisualBase> myApp;
  FFlReference<FFlVisualBase> myDetail;
};

#ifdef FF_NAMESPACE
} // namespace
#endif
#endif
