// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#include "FFaLib/FFaAlgebra/FFaBody.H"
#include "FFaLib/FFaAlgebra/FFaVec3.H"
#include "FFaLib/FFaAlgebra/FFaMat34.H"
#include "FFaLib/FFaAlgebra/FFaTensor3.H"
#include "FFaLib/FFaOS/FFaFilePath.H"
#include <fstream>


int BodyTest (const std::string& fname, double z0, double z1)
{
  std::ifstream cad(fname,std::ios::in);
  if (!cad) return 2;

  FFaBody::prefix = FFaFilePath::getPath(fname);
  FFaBody* body = FFaBody::readFromCAD(cad);
  if (!body) return 3;

  std::cout <<"\n# Vertices: "<< body->getNoVertices()
            <<"\n# Faces   : "<< body->getNoFaces() << std::endl;

  if (!FFaFilePath::isExtension(fname,"ftc"))
  {
    std::string outFile(fname);
    if (body->writeCAD(FFaFilePath::addExtension(outFile,"ftc"),FaMat34()))
      std::cout <<"Wrote "<< outFile << std::endl;
    else
      return 9;
  }

  double Vb, A1, A2;
  FaVec3 C0b, C0s, minX, maxX;
  if (!body->computeBoundingBox(minX,maxX)) return 4;

  std::cout <<"\nBounding Box: "<< minX <<"\t"<< maxX
            <<"\nCalculation center: "<< 0.5*(minX+maxX) << std::endl;

  body->computeTotalVolume(Vb,C0b);
  std::cout <<"Volume = "<< Vb <<"\nCenter = "<< C0b << std::endl;

  FFaTensor3 Ib;
  body->computeTotalVolume(Vb,C0b,&Ib);
  std::cout <<"Volume = "<< Vb
            <<"\nInertia = "<< Ib
            <<"\nCenter = "<< C0b << std::endl;

  std::cout <<"z0 = "<< z0 << std::endl;
  if (!body->computeVolumeBelow(Vb,A1,C0b,C0s,FaVec3(0.0,0.0,1.0),z0)) return 5;
  std::cout <<"Volume below = "<< Vb <<"\nCenter below = "<< C0b <<"\n"
            <<"Section area = "<< A1 <<"\nCenter area  = "<< C0s << std::endl;
  if (z1 == z0)
  {
    delete body;
    return 0;
  }

  std::cout <<"z1 = "<< z1 << std::endl;
  if (!body->saveIntersection(FaMat34(FaVec3(0,0,-z0)))) return 6;
  if (!body->computeVolumeBelow(Vb,A2,C0b,C0s,FaVec3(0.0,0.0,1.0),z1)) return 7;
  std::cout <<"Volume below = "<< Vb <<"\nCenter below = "<< C0b <<"\n"
            <<"Section area = "<< A2 <<"\nCenter area  = "<< C0s << std::endl;

  if (!body->computeIncArea(Vb,C0s,FaVec3(0.0,0.0,1.0),
                            FaMat34(FaVec3(0.0,0.0,-z1)))) return 8;
  std::cout <<"Area increment = "<< Vb <<" "<< A2-A1
            <<"\nIncrement center = "<< C0s << std::endl;

  delete body;
  return 0;
}
