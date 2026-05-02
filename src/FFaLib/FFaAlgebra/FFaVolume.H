// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#ifndef _FFAVOLUME_H_
#define _FFAVOLUME_H_

class FaVec3;
class FFaTensor3;


/*!
  A namespace with functions for calculation of volume, volume center
  and volume moment (inertia) for tetrahedron, wedge and hexahedron object.
*/

namespace FFaVolume
{
  // Volume only calculation.
  void tetVolume (const FaVec3& v1, const FaVec3& v2,
                  const FaVec3& v3, const FaVec3& v4, double& vol);
  void wedVolume (const FaVec3& v1, const FaVec3& v2,
                  const FaVec3& v3, const FaVec3& v4,
                  const FaVec3& v5, const FaVec3& v6, double& vol);
  void hexVolume (const FaVec3& v1, const FaVec3& v2,
                  const FaVec3& v3, const FaVec3& v4,
                  const FaVec3& v5, const FaVec3& v6,
                  const FaVec3& v7, const FaVec3& v8, double& vol);

  // Calculation of volume center. The input vertices are then shifted
  // such that they become relative to the computed volume center.
  // The return value is the volume of the object.
  double tetCenter (FaVec3& v1, FaVec3& v2,
                    FaVec3& v3, FaVec3& v4, FaVec3& vc);
  double wedCenter (FaVec3& v1, FaVec3& v2,
                    FaVec3& v3, FaVec3& v4,
                    FaVec3& v5, FaVec3& v6, FaVec3& vc);
  double hexCenter (FaVec3& v1, FaVec3& v2,
                    FaVec3& v3, FaVec3& v4,
                    FaVec3& v5, FaVec3& v6,
                    FaVec3& v7, FaVec3& v8, FaVec3& vc);

  // Calculation of volume moments (inertias). The input vertices
  // are here assumed to be relative to the objects volume center.
  void tetMoment (const FaVec3& v1, const FaVec3& v2,
                  const FaVec3& v3, const FaVec3& v4, FFaTensor3& vm);
  void wedMoment (const FaVec3& v1, const FaVec3& v2,
                  const FaVec3& v3, const FaVec3& v4,
                  const FaVec3& v5, const FaVec3& v6, FFaTensor3& vm);
  void hexMoment (const FaVec3& v1, const FaVec3& v2,
                  const FaVec3& v3, const FaVec3& v4,
                  const FaVec3& v5, const FaVec3& v6,
                  const FaVec3& v7, const FaVec3& v8, FFaTensor3& vm);
}

#endif
