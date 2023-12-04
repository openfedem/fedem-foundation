// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#include <limits>
#include <fstream>
#include <sstream>
#include <cctype>

#include "FFaLib/FFaAlgebra/FFaBody.H"
#include "FFaLib/FFaAlgebra/FFaMat34.H"

std::string FFaBody::prefix;


/*!
  Static helper functions for CAD file parsing.
*/

static bool getIdentifier(std::istream& in, std::string& identifier,
                          char endChar = '}')
{
  identifier.clear();
  if (!in) return false;

  char c = ' ';
  while (isspace(c) && in.get(c))
    if (c == '#')
    {
      std::string comment;
      std::getline(in,comment);
      std::cout << comment << std::endl;
      c = ' ';
    }

  while (in)
  {
    if (c == endChar)
      break;
    else if (isalnum(c) || c == '_' || endChar == '"')
      identifier += c;
    else
    {
      in.putback(c);
      break;
    }
    in.get(c);
  }

  if (identifier.empty()) return false;
#if FFA_DEBUG > 1
  std::cout <<"Found identifier: \""<< identifier <<"\""<< std::endl;
#endif
  return true;
}

static bool getIdentifier(std::istream& in, std::string& identifier,
                          std::string& label, char& type)
{
  type = 'n';
  if (!getIdentifier(in,identifier)) return false;

  // Check for "DEF" or "USE" keyword
  char d = ' ', c = ' ';
  while (isspace(c) && in.get(c));

  const char* DEF = " DEF";
  const char* USE = " USE";
  for (int i = 1; i < 4 && in; i++, d = c, in.get(c))
    if (c == USE[i] && d == USE[i-1])
      type = 'U';
    else if (c == DEF[i] && d == DEF[i-1])
      type = 'D';
    else
    {
      type = 'n';
      in.putback(c);
      break;
    }

  if (type != 'n')
  {
    if (!getIdentifier(in,label))
      type = 'N';
#if FFA_DEBUG > 1
    else
      std::cout <<'\t'<< type <<" \""<< label <<"\""<< std::endl;
#endif
  }
  return true;
}


static void skipToData(std::istream& in, char beginChar = '{')
{
  in.ignore(std::numeric_limits<int>::max(), beginChar);
}

static void skipToDataEnd(std::istream& in, char endChar = '}',
                          bool skipAllData = false)
{
  if (skipAllData)
    in.ignore(std::numeric_limits<int>::max(), endChar == '}' ? '{' : '[');

  // Handle possibly nested delimeter characters, TT #2892
  for (int c = in.get(); c != endChar && in.good(); c = in.get())
    if (c == '{' && endChar == '}')
      skipToDataEnd(in,endChar);
    else if (c == '[' && endChar == ']')
      skipToDataEnd(in,endChar);
}


/*!
  This method administers the input of a body definition from a file.
  The body may either be defined on FT's internal CAD format,
  or on the external VRML or STL formats.
*/

FFaBody* FFaBody::readFromCAD(std::istream& in)
{
  std::string firstLine;
  std::getline(in,firstLine);
  if (firstLine == "Fedem Technology Simplified CAD model")
    return readCAD(in);
  else if (firstLine == "#VRML V1.0 ascii")
    return readWRL(in,1);
  else if (firstLine == "#VRML V2.0 utf8")
    return readWRL(in,2);
  else if (firstLine.substr(0,5) == "solid")
    return readSTL(in);

  std::cerr <<"FFaBody::readFromCAD: Not a valid geometry file, header = "
            << firstLine << std::endl;

  return NULL;
}


static std::istream& readLine(std::istream& is, std::string& cline)
{
  int c = ' ';
  while (is && isspace(c)) c = is.get();
  if (is)
  {
    is.putback(c);
    std::getline(is,cline);
  }
  return is;
}


FFaBody* FFaBody::readSTL(std::istream& in)
{
  std::cout <<"\nFFaBody: Parsing STL data."<< std::endl;

  FFaBody* newBody = NULL;
  std::string cline("solid");
  if (!readLine(in,cline))
    return newBody;

  while (cline.substr(0,5) == "facet")
  {
    if (!readLine(in,cline))
      return newBody;
    else while (cline.substr(0,10) == "outer loop")
    {
      if (!newBody) newBody = new FFaBody();

      FaVec3 XYZ;
      std::vector<size_t> facet;
      facet.reserve(3);
      while (readLine(in,cline) && cline.substr(0,6) == "vertex")
      {
        std::stringstream strline(cline.substr(6));
        strline >> XYZ;
        facet.push_back(newBody->addVertex(XYZ));
      }
      if (cline.substr(0,7) == "endloop")
      {
        if (facet.size() == 3)
          newBody->addFace(facet[0],facet[1],facet[2]);
        else if (facet.size() >= 4)
          newBody->addFace(facet[0],facet[1],facet[2],facet[3]);
        else
          std::cerr <<"FFaBody::readSTL: Degenereted loop (ignored) "
                    << facet.size() << std::endl;
      }
      else
      {
        std::cerr <<"FFaBody::readSTL: No matching endloop, got "<< cline
                  <<"\n     Bailing... "<< std::endl;
        return newBody;
      }
      if (!readLine(in,cline))
        return newBody;
    }
    if (cline.substr(0,8) != "endfacet")
    {
      std::cerr <<"FFaBody::readSTL: No matching endfacet, got "<< cline
                <<"\n     Bailing... "<< std::endl;
      return newBody;
    }
    if (!readLine(in,cline))
      return newBody;
  }

  return newBody;
}


FFaBody* FFaBody::readCAD(std::istream& in)
{
  std::cout <<"\nFFaBody: Parsing FT CAD data."<< std::endl;

  FaMat34 partCS;
  FFaBody* newBody = NULL;

  std::string identifier;
  if (!getIdentifier(in,identifier))
    return newBody;

  skipToData(in);
  if (identifier != "Part")
    std::cerr <<"FFaBody::readCAD: Unsupported identifier "<< identifier
              << std::endl;
  else while (getIdentifier(in,identifier))
  {
    skipToData(in);
    if (identifier == "Body")
    {
      if (!newBody) newBody = new FFaBody();
      newBody->readBody(in,partCS);
    }
    else // Unsupported identifier
      skipToDataEnd(in);
  }

  skipToDataEnd(in);
  return newBody;
}


FFaBody* FFaBody::readWRL(std::istream& in, int version)
{
  std::cout <<"\nFFaBody: Parsing VRML data, version "<< version << std::endl;

  FFaBody* newBody = NULL;
  if (version == 1)
    readWRL1(newBody,in);
  else
    readWRL2(newBody,in);
  return newBody;
}


void FFaBody::readWRL1(FFaBody*& newBody, std::istream& in)
{
  std::string identifier;

  while (getIdentifier(in,identifier))
  {
    if (identifier == "DEF")
    {
      std::getline(in,identifier);
#ifdef FFA_DEBUG
      std::cout <<"Found label:"<< identifier << std::endl;
#endif
      continue;
    }

    skipToData(in);
    if (identifier == "WWWInline")
    {
      getIdentifier(in,identifier);
      if (identifier == "name")
      {
        skipToData(in,'"');
        getIdentifier(in,identifier,'"');
      }
      else
        identifier.clear();

      if (!identifier.empty())
      {
        identifier = prefix + identifier;
        std::ifstream cad(identifier.c_str(),std::ios::in);
        if (cad)
        {
          std::getline(cad,identifier);
          if (identifier == "#VRML V1.0 ascii")
            readWRL1(newBody,cad);
          else
            std::cerr <<"FFaBody::readWRL: Invalid included wrl-file, header = "
                      << identifier << std::endl;
        }
        else
          std::cerr <<"FFaBody::readWRL: Failed to open included wrl-file "
                    << identifier << std::endl;
      }
      skipToDataEnd(in);
    }
    else if (identifier == "Coordinate3")
    {
      if (!newBody)
        newBody = new FFaBody();
      getIdentifier(in,identifier);
      if (identifier == "point")
      {
        skipToData(in,'[');
        newBody->readCoords(in,FaMat34(),']');
      }
      skipToDataEnd(in);
    }
    else if (identifier == "IndexedFaceSet")
    {
      if (!newBody) newBody = new FFaBody();
      getIdentifier(in,identifier);
      if (identifier == "coordIndex")
      {
        skipToData(in,'[');
        newBody->readFaces(in,']');
      }
      skipToDataEnd(in);
    }
    else if (identifier != "Separator" && identifier != "Group")
      skipToDataEnd(in);
  }

  skipToDataEnd(in);
  if (newBody)
    newBody->startVx = newBody->getNoVertices();
}


void FFaBody::readWRL2(FFaBody*& newBody, std::istream& in)
{
  std::string identifier;

  while (getIdentifier(in,identifier))
    if (identifier == "Group")
    {
      skipToData(in);
      while (getIdentifier(in,identifier))
        if (identifier == "children")
          readChildren(newBody,FaMat34(),in);
        else
          skipToDataEnd(in,']',true);
    }
    else
      skipToDataEnd(in,'}',true);
}


void FFaBody::readChildren(FFaBody*& newBody, const FaMat34& bodyCS,
                           std::istream& in)
{
  std::string identifier;

  skipToData(in,'[');
  while (getIdentifier(in,identifier,']'))
    if (identifier == "Transform")
      readTransform(newBody,bodyCS,in);
    else if (identifier == "Shape")
      readShape(newBody,bodyCS,in);
    else
      skipToDataEnd(in,'}',true);
}


void FFaBody::readTransform(FFaBody*& newBody, const FaMat34& bodyCS,
                            std::istream& in)

{
  FaVec3 C, T;
  FaVec3 S(1.0,1.0,1.0);
  FaVec3 R(0.0,0.0,1.0);
  FaVec3 SR(0.0,0.0,1.0);
  double thetaR = 0.0;
  double thetaSR = 0.0;
  std::string identifier;

  skipToData(in);
  while (getIdentifier(in,identifier))
    if (identifier == "center")
      in >> C;
    else if (identifier == "translation")
      in >> T;
    else if (identifier == "rotation")
      in >> R >> thetaR;
    else if (identifier == "scale")
      in >> S;
    else if (identifier == "scaleOrientation")
      in >> SR >> thetaSR;
    else if (identifier == "children")
    {
      FaMat34 newCS(bodyCS);
      newCS[VW] += T;
      if (thetaR != 0.0) // TODO
        std::cerr <<"FFaBody::readTransform: rotation not implemented."
                  << std::endl;
      if (thetaSR != 0.0) // TODO
        std::cerr <<"FFaBody::readTransform: scaleOrientation not implemented."
                  << std::endl;
      if (S != FaVec3(1.0,1.0,1.0)) // TODO
        std::cerr <<"FFaBody::readTransform: scale not implemented."
                  << std::endl;
      if (!C.isZero()) // TODO
        std::cerr <<"FFaBody::readTransform: center not implemented."
                  << std::endl;
      readChildren(newBody,newCS,in);
    }
    else
      skipToDataEnd(in,']',true);
}


void FFaBody::readShape(FFaBody*& newBody, const FaMat34& bodyCS,
                        std::istream& in)
{
  char lType;
  std::string identifier, label;

  skipToData(in);
  while (getIdentifier(in,identifier,label,lType))
    if (lType == 'U')
    {
      if (identifier == "geometry")
        std::cerr <<"FFaBody::readShape: Ignoring geometry USE "<< label <<"\n";
    }
    else if (identifier == "geometry")
    {
      getIdentifier(in,identifier);
      if (identifier == "IndexedFaceSet")
      {
        if (!newBody) newBody = new FFaBody();
        newBody->readIndexedFaceSet(in,bodyCS);
      }
      else
        skipToDataEnd(in,'}',true);
    }
    else
      skipToDataEnd(in,'}',true);
}


void FFaBody::readIndexedFaceSet(std::istream& in, const FaMat34& partCS)
{
  std::string identifier;

  skipToData(in);
  while (getIdentifier(in,identifier))
    if (identifier == "coord")
    {
      getIdentifier(in,identifier);
      if (identifier == "Coordinate")
      {
        skipToData(in);
        while (getIdentifier(in,identifier))
          if (identifier == "point")
          {
            skipToData(in,'[');
            this->readCoords(in,partCS,']');
          }
          else
            skipToDataEnd(in);
      }
      else
        skipToDataEnd(in);
    }
    else if (identifier == "coordIndex")
    {
      skipToData(in,'[');
      this->readFaces(in,']');
    }
    else if (identifier == "ccw" ||
             identifier == "convex" ||
             identifier == "colorPerVertex" ||
             identifier == "normalPerVertex" ||
             identifier == "solid")
      skipToData(in,'\n');
    else if (identifier == "colorIndex" ||
             identifier == "normalIndex" ||
             identifier == "textCoordIndex")
      skipToDataEnd(in,']',true);
    else
      skipToDataEnd(in,'}',true);

  startVx = myVertices.size();
}


void FFaBody::readBody(std::istream& in, const FaMat34& partCS)
{
  std::string identifier;

  while (getIdentifier(in,identifier))
  {
    skipToData(in);
    if (identifier == "Coordinates")
      this->readCoords(in,partCS);
    else if (identifier == "Face")
      this->readFace(in);
    else // Unsupported identifier
      skipToDataEnd(in);
  }

  startVx = myVertices.size();
}


void FFaBody::readCoords(std::istream& in, const FaMat34& partCS, char endChar)
{
  FaVec3 vertex;
  char c;
  while (in)
  {
    in >> vertex >> c;
    if (c != ',' && c != ' ') in.putback(c);

    if (in.good())
      myVertices.push_back(partCS*vertex);
    else
      break;
  }

  in.clear();
  skipToDataEnd(in,endChar);
}


void FFaBody::readFace(std::istream& in)
{
  std::string identifier;

  while (getIdentifier(in,identifier))
  {
    skipToData(in);
    if (identifier == "TriangleIndexes")
      this->readFaces(in);
    else if (identifier == "FaceIndices")
      this->readFaces(in);
    else // Unsupported identifier
      skipToDataEnd(in);
  }
}


void FFaBody::readFaces(std::istream& in, char endChar)
{
  int tIdx[16];
  char c;
  for (int i = 0; in && i < 16; i++)
  {
    in >> tIdx[i] >> c;
    if (c != ',' && c != ' ')
      in.putback(c);

    if (!in.good())
      break;
    else if (tIdx[i] < 0)
    {
      if (i == 3)
        this->addFace(startVx+tIdx[0],startVx+tIdx[1],startVx+tIdx[2]);
      else if (i == 4)
        this->addFace(startVx+tIdx[0],startVx+tIdx[1],
                      startVx+tIdx[2],startVx+tIdx[3]);
      else
        std::cerr <<"FFaBody::readFaces: Polygon with "<< i
                  <<" vertices ignored."<< std::endl;
      i = -1;
    }
  }

  in.clear();
  skipToDataEnd(in,endChar);
}


/*!
  This method writes out the body definition on FT's internal CAD format.
*/

bool FFaBody::writeCAD(const std::string& fileName, const FaMat34& partCS) const
{
  std::ofstream os(fileName.c_str());
  if (!os)
  {
    std::cerr <<"FFaBody::writeCAD: Failed to write "<< fileName << std::endl;
    return false;
  }
  size_t i;
  int j, vIdx;
  os <<"Fedem Technology Simplified CAD model\n\nPart {\n  CS {"<< partCS;
  os <<"\n  }\n  Body {\n    Coordinates {";
  for (i = 0; i < this->getNoVertices(); i++)
    os <<"\n      "<< this->getVertex(i);
  os <<"\n    }\n    Face {\n      FaceIndices {";
  for (i = 0; i < this->getNoFaces(); i++)
  {
    os <<"\n       ";
    for (j = 0; (vIdx = this->getFaceVtx(i,j)) >= 0; j++)
      os <<" "<< vIdx;
    os <<" -1";
  }
  os <<"\n      }\n    }\n  }\n}\n";
  return true;
}
