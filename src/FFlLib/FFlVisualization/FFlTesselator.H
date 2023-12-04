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
#include <cstddef>

typedef std::vector<int> IntVec;
class FaVec3;


class FFlTesselator
{
  struct Vertex {
    int id;
    float v[3];
    Vertex* next;
    Vertex(int ID, double x, double y, double z)
    { id = ID; v[0] = (float)x; v[1] = (float)y; v[2] = (float)z; next = NULL; }
  };

public:
  static bool tesselate(std::vector<IntVec>& shapeIndexes,
                        const std::list<int>& polygon,
                        const std::vector<FaVec3*>& vertexes,
                        const FaVec3& normal);

  // Set callback to be called for each triangle identified.
  // Callback is invoked with the id for each vertex in the triangle.
  static void setCallback(void (*cb)(int,int,int)) { myCallback = cb; }

private:
  static bool isLinesIntersecting(float p11x, float p11y,
                                  float p12x, float p12y,
                                  float p21x, float p21y,
                                  float p22x, float p22y);
  static bool clippable(Vertex* v);
  static bool isTriangle(Vertex* v);
  static void emitTriangle(Vertex* v);
  static bool pointInTriangle(Vertex* p, Vertex* t);
  static void cutTriangle(Vertex* t);
  static float det(Vertex* t);

  static Vertex* headV;
  static Vertex* tailV;
  static int X,Y,Dir;

  static void (*myCallback)(int v0, int v1, int v2);

  // Reference to vector accumulating triangles
  static std::vector<IntVec>* myTriangles;
};

#endif
