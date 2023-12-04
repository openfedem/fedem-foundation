// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#include "FFaLib/FFaGeometry/FFaCompoundGeometry.H"
#include <fstream>


int GeometryTest (const char* fname, std::istream* pointdata)
{
  std::ifstream is(fname,std::ios::in);
  if (!is) return 2;

#ifdef FT_HAS_GEOMETRY
  FFaCompoundGeometry myGeo;
  is >> myGeo;
  std::cout <<"Read geometry:"<< myGeo << std::endl;
#else
  std::cerr <<" *** Built without FFaGeometry"<< std::endl;
#endif

  if (pointdata)
    while (pointdata->good())
    {
      FaVec3 X;
      *pointdata >> X;
#ifdef FT_HAS_GEOMETRY
      if (myGeo.isInside(X))
        std::cout <<"Point "<< X <<" is inside"<< std::endl;
#endif
    }

  return 0;
}
