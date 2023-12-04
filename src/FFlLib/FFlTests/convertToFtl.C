// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#include "FFlLib/FFlLinkHandler.H"
#include "FFlLib/FFlElementBase.H"
#include "FFlLib/FFlAttributeBase.H"
#include "FFlLib/FFlFEParts/FFlNode.H"
#include "FFlLib/FFlIOAdaptors/FFlFedemWriter.H"
#include "FFaLib/FFaAlgebra/FFaTensor3.H"
#include "FFaLib/FFaAlgebra/FFaBody.H"
#include "FFaLib/FFaOS/FFaFilePath.H"
#include <fstream>
#include <cstring>


/*!
  \brief Converter program from vrml or ftc to ftl.
*/

int convertToFtl (const char* fname)
{
  std::ifstream cad(fname,std::ios::in);
  if (!cad) return 2;

  FFaBody::prefix = FFaFilePath::getPath(fname);
  FFaBody* body = FFaBody::readFromCAD(cad);
  if (!body) return 3;

  size_t nFace = body->getNoFaces();
  size_t nVert = body->getNoVertices();
  std::cout <<"\n# Vertices: "<< nVert <<"\n# Faces   : "<< nFace << std::endl;

  FaVec3 minX, maxX;
  if (!body->computeBoundingBox(minX,maxX)) return 4;

  std::cout <<"\nBounding Box: "<< minX <<"\t"<< maxX
            <<"\nCalculation center: "<< 0.5*(minX+maxX) << std::endl;

  double Vb;
  FaVec3 C0b;
  body->computeTotalVolume(Vb,C0b);
  std::cout <<"Volume = "<< Vb <<"\nCenter = "<< C0b << std::endl;

  FFaTensor3 Ib;
  body->computeTotalVolume(Vb,C0b,&Ib);
  std::cout <<"Volume = "<< Vb
            <<"\nInertia = "<< Ib
            <<"\nCenter = "<< C0b << std::endl;

  FFlLinkHandler link;
  std::vector<int> elmNodes(3);
  size_t i, j;

  for (i = 0; i < nVert; i++)
    link.addNode(new FFlNode(i+1,body->getVertex(i)));

  for (i = 0; i < nFace; i++)
  {
    FFlElementBase* elm = ElementFactory::instance()->create("TRI3",i+1);
    for (j = 0; j < 3; j++)
      elmNodes[j] = 1+ body->getFaceVtx(i,j);
    elm->setNodes(elmNodes);
    elm->setAttribute("PTHICK",1);
    link.addElement(elm);
  }
  link.addAttribute(AttributeFactory::instance()->create("PTHICK",1));
  link.resolve();

  char* ftlFile = strcat(strtok(strdup(fname),"."),".ftl");
  std::cout <<"\nWriting "<< ftlFile << std::endl;
  FFlFedemWriter fedem(&link);
  return fedem.write(ftlFile) ? 0 : 4;
}
