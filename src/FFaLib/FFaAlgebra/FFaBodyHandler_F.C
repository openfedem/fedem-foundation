// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#include <fstream>
#include <vector>
#include <string>
#include <cstring>

#include "FFaLib/FFaAlgebra/FFaBody.H"
#include "FFaLib/FFaAlgebra/FFaMat34.H"
#include "FFaLib/FFaOS/FFaFortran.H"
#include "FFaLib/FFaOS/FFaFilePath.H"
#include "FFaLib/FFaDefinitions/FFaMsg.H"

static std::vector<FFaBody*> ourBodies;


INTEGER_FUNCTION(ffa_body,FFA_BODY) (const char* fileName, const int nchar)
{
  std::string bodyFile(fileName,nchar);
  std::ifstream cad(bodyFile.c_str(),std::ios::in);
  if (!cad)
  {
    ListUI <<" *** Failed to open body file "<< bodyFile <<"\n";
    return -1;
  }

  FFaBody::prefix = FFaFilePath::getPath(bodyFile);

  FFaBody* body = FFaBody::readFromCAD(cad);
  if (!body)
  {
    ListUI <<" *** Empty or invalid body file "<< bodyFile <<"\n";
    return -2;
  }

  ourBodies.push_back(body);
  return ourBodies.size()-1;
}


static bool checkBodyIndex (int bodyIndex, int& ierr)
{
  int nBody = ourBodies.size();
  if (bodyIndex >= 0 && bodyIndex < nBody)
  {
    ierr = 0;
    return true;
  }
  else
  {
    ListUI <<" *** Body index "<< bodyIndex <<" out of range [0,"
           << nBody-1 <<"].\n";
    ierr = -1;
    return false;
  }
}


SUBROUTINE(ffa_get_nofaces,FFA_GET_NOFACES) (const int& bodyIndex, int& nface)
{
  if (checkBodyIndex(bodyIndex,nface))
    nface = ourBodies[bodyIndex]->getNoFaces();
}


SUBROUTINE(ffa_get_face,FFA_GET_FACE) (const int& bodyIndex, const int& fIndex,
				       double* coords, int& nVert)
{
  if (checkBodyIndex(bodyIndex,nVert))
  {
    double* p = coords;
    for (nVert = 0; nVert < 4; nVert++, p+=3)
    {
      int vIdx = ourBodies[bodyIndex]->getFaceVtx(fIndex,nVert);
      if (vIdx < 0) break;
      memcpy(p,ourBodies[bodyIndex]->getVertex(vIdx).getPt(),3*sizeof(double));
    }
  }
}


SUBROUTINE(ffa_partial_volume,FFA_PARTIAL_VOLUME) (const int& bodyIndex,
						   const double* normal,
						   const double& z0,
						   double& Vb,
						   double& As,
						   double* C0b,
						   double* C0s,
						   int& ierr)
{
  if (checkBodyIndex(bodyIndex,ierr))
  {
    FaVec3 vn(normal[0],normal[1],normal[2]), volCenter, areaCenter;
    ourBodies[bodyIndex]->computeVolumeBelow(Vb,As,volCenter,areaCenter,vn,z0);
    for (int i = 0; i < 3; i++)
    {
      C0b[i] = volCenter[i];
      C0s[i] = areaCenter[i];
    }
  }
}


SUBROUTINE(ffa_total_volume,FFA_TOTAL_VOLUME) (const int& bodyIndex,
					       double& V,
					       double* C0,
					       int& ierr)
{
  if (checkBodyIndex(bodyIndex,ierr))
  {
    FaVec3 volumeCenter;
    ourBodies[bodyIndex]->computeTotalVolume(V,volumeCenter);
    for (int i = 0; i < 3; i++) C0[i] = volumeCenter[i];
  }
}


SUBROUTINE(ffa_save_intersection,FFA_SAVE_INTERSECTION) (const int& bodyIndex,
							 const double* CS,
							 int& ierr)
{
  if (checkBodyIndex(bodyIndex,ierr))
    ourBodies[bodyIndex]->saveIntersection(FaMat34(CS));
}


SUBROUTINE(ffa_inc_area,FFA_INC_AREA) (const int& bodyIndex,
				       const double* normal,
				       const double* CS,
				       double& dAs, double* C0s,
				       int& ierr)
{
  if (checkBodyIndex(bodyIndex,ierr))
  {
    FaVec3 vn(normal[0],normal[1],normal[2]), areaCenter;
    ourBodies[bodyIndex]->computeIncArea(dAs,areaCenter,vn,FaMat34(CS));
    for (int i = 0; i < 3; i++) C0s[i] = areaCenter[i];
  }
}


SUBROUTINE(ffa_erase_bodies,FFA_ERASE_BODIES) ()
{
  for (size_t i = 0; i < ourBodies.size(); i++)
    delete ourBodies[i];

  ourBodies.clear();
}
