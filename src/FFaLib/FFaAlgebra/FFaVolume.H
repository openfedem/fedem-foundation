// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

/*!
  \file FFaVolume.H
  \brief Volume calculation for 3D shapes.
*/

#ifndef FFA_VOLUME_H
#define FFA_VOLUME_H

class FaVec3;
class FFaTensor3;


/*!
  \brief A namespace with volume calculation functions for some 3D shapes.
  \details The volume of an object is computed as a sum of tetrahedron and
  pyramid contributions to account for possibly warped faces.
  There are also functions for calculation of the volume center and moments
  of inertia. Note that the volume center functions will update the vertex
  coordinates such that they become relative to the calculated center.
  These relative coordinates are needed by the inertia calculation functions.
*/

namespace FFaVolume
{
  //! \brief Calculates the volume of a tetrahedron.
  void tetVolume (const FaVec3& v1, const FaVec3& v2,
                  const FaVec3& v3, const FaVec3& v4, double& vol);
  //! \brief Calculates the volume of a wedge.
  void wedVolume (const FaVec3& v1, const FaVec3& v2,
                  const FaVec3& v3, const FaVec3& v4,
                  const FaVec3& v5, const FaVec3& v6, double& vol);
  //! \brief Calculates the volume of a hexahedron.
  void hexVolume (const FaVec3& v1, const FaVec3& v2,
                  const FaVec3& v3, const FaVec3& v4,
                  const FaVec3& v5, const FaVec3& v6,
                  const FaVec3& v7, const FaVec3& v8, double& vol);

  //////////////////////////////////////////////////////////////////////////////
  // Calculation of volume center. The input vertices are then shifted
  // such that they become relative to the computed volume center.
  // The return value is the volume of the object.

  //! \brief Calculates the volume and volume center of a tetrahedron.
  double tetCenter (FaVec3& v1, FaVec3& v2,
                    FaVec3& v3, FaVec3& v4, FaVec3& vc);
  //! \brief Calculates the volume and volume center of a wedge.
  double wedCenter (FaVec3& v1, FaVec3& v2,
                    FaVec3& v3, FaVec3& v4,
                    FaVec3& v5, FaVec3& v6, FaVec3& vc);
  //! \brief Calculates the volume and volume center of a hexahedron.
  double hexCenter (FaVec3& v1, FaVec3& v2,
                    FaVec3& v3, FaVec3& v4,
                    FaVec3& v5, FaVec3& v6,
                    FaVec3& v7, FaVec3& v8, FaVec3& vc);

  //////////////////////////////////////////////////////////////////////////////
  // Calculation of volume moments (inertias). The input vertices
  // are here assumed to be relative to the objects volume center.

  //! \brief Calculates the volume moment of intertia of a tetrahedron.
  void tetMoment (const FaVec3& v1, const FaVec3& v2,
                  const FaVec3& v3, const FaVec3& v4, FFaTensor3& vm);
  //! \brief Calculates the volume moment of intertia of a wedge.
  void wedMoment (const FaVec3& v1, const FaVec3& v2,
                  const FaVec3& v3, const FaVec3& v4,
                  const FaVec3& v5, const FaVec3& v6, FFaTensor3& vm);
  //! \brief Calculates the volume moment of intertia of a hexahedron
  void hexMoment (const FaVec3& v1, const FaVec3& v2,
                  const FaVec3& v3, const FaVec3& v4,
                  const FaVec3& v5, const FaVec3& v6,
                  const FaVec3& v7, const FaVec3& v8, FFaTensor3& vm);
}

#endif
