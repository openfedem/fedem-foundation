// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#ifndef FFL_TESSELATOR_H
#define FFL_TESSELATOR_H

#include <list>
#include <vector>

class FaVec3;


namespace FFlTesselator
{
  using IntVec = std::vector<int>;

  //! \brief Tesselates the \a polygon into triangles.
  bool tesselate(std::vector<IntVec>& shapeIndexes,
                 const std::list<int>& polygon,
                 const std::vector<FaVec3*>& vertexes,
                 const FaVec3& normal);

  //! \brief Sets the callback to be invoked for each triangle identified.
  void setCallback(void(*cb)(int,int,int));
}

#endif
