// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#include "FFlLib/FFlVisualization/FFlTesselator.H"
#include "FFaLib/FFaAlgebra/FFaVec3.H"
#include <functional>


namespace
{
  //! \brief A struct for instances in a linked list of vertices.
  struct Vertex
  {
    int     id;   //!< Vertex identifier
    float   v[3]; //!< Vertex coordinates
    Vertex* next; //!< Pointer to next vertex in linked lists
    Vertex(int ID, const FaVec3& vtx) : id(ID), next(NULL)
    { for (int i = 0; i < 3; i++) v[i] = static_cast<float>(vtx[i]); }
  };

  //! Coordinate indices of the projection plane used
  int X = 0;
  int Y = 1;

  //! Tolerance for float comparison
  constexpr float FDT_EPSILON = 1.0e-6;

  //! Pointer to vector accumulating triangles
  std::vector<FFlTesselator::IntVec>* myTriangles = NULL;

  //! Callback to be invoked for each triangle identified
  void(*myCallback)(int,int,int) = NULL;

  //! \brief Throw away triangle.
  void cutTriangle(Vertex* t)
  {
    Vertex* tmp = t->next;
    if (tmp)
    {
      t->next = tmp->next;
      delete tmp;
    }
  }

  //! \brief Add triangle to index container.
  void emitTriangle(Vertex *t)
  {
    if (t->next && t->next->next)
    {
      if (myCallback)
        myCallback(t->id,t->next->id,t->next->next->id);

      if (myTriangles)
        myTriangles->push_back({t->id,t->next->id,t->next->next->id});
    }

    cutTriangle(t);
  }

  //! \brief Check if there are no intersection to the triangle \a *tri.
  bool clippable(Vertex* tri)
  {
    // Lambda function comparing two vertices
    bool (*Equal)(Vertex*,Vertex*)=[](Vertex* a, Vertex* b) -> bool
    {
      if (a != b)
        for (int i = 0; i < 3; i++)
          if (fabsf(a->v[i] - b->v[i]) > FDT_EPSILON)
            return false;
      return true;
    };

    // Lambda function checking if a point is inside a triangle.
    bool (*pointInTriangle)(Vertex*,Vertex*)=[](Vertex* p, Vertex* t) -> bool
    {
      const float x = p->v[X];
      const float y = p->v[Y];

      std::function<bool(float*,float*)> checkPoints =
        [x,y](float* v1, float* v2) -> bool
      {
        float y1 = v1[Y];
        float y2 = v2[Y];
        if ( (y1 <= y && y < y2) || // y of point between y of the triangle line
             (y2 <= y && y < y1) )  // Taking care of y2 < y1
          return x < v1[X] + (v2[X]-v1[X]) * (y-y1) / (y2-y1);
        else
          return false;
      };

      bool result = false;

      if (checkPoints(t->v, t->next->next->v))
        result = !result;

      if (checkPoints(t->next->v, t->v))
        result = !result;

      if (checkPoints(t->next->next->v, t->next->v))
        result = !result;

      return result;
    };

    Vertex* vtx = tri->next->next->next;
    do {
      if (!Equal(vtx, tri) &&
          !Equal(vtx, tri->next) &&
          !Equal(vtx, tri->next->next) &&
          pointInTriangle(vtx, tri))
        return false;
      vtx = vtx->next;
    } while (vtx != tri);

    return true;
  }
}


void FFlTesselator::setCallback(void(*cb)(int,int,int))
{
  myCallback = cb;
}


bool FFlTesselator::tesselate(std::vector<IntVec>& shapeIndexes,
                              const std::list<int>& polygon,
                              const std::vector<FaVec3*>& vertexes,
                              const FaVec3& normal)
{
  // Add all vertices along the edge, supply the ID to be
  // sent with callback when a triangle is identified
  Vertex* headV = NULL;
  Vertex* tailV = NULL;
  for (int vxId : polygon)
  {
    Vertex* newv = new Vertex(vxId,*vertexes[vxId]);
    if (!headV) headV = newv;
    if (tailV) tailV->next = newv;
    tailV = newv;
  }
  if (!tailV)
    return false; // empty polygon

  myTriangles = &shapeIndexes;

  // Lambda function removing a Vertex from the linked list
  Vertex* (*removeVertex)(Vertex*) = [](Vertex* vtx) -> Vertex*
  {
    Vertex* nxt = vtx->next;
    delete vtx;
    return nxt;
  };

  // Lambda function returning the determinant of the triangle rooted at v0.
  float (*det)(Vertex*) = [](Vertex* v0) -> float
  {
    Vertex* v1 = v0->next;
    Vertex* v2 = v1->next;
    return ( (v1->v[X] - v0->v[X]) * (v2->v[Y] - v0->v[Y]) -
             (v1->v[Y] - v0->v[Y]) * (v2->v[X] - v0->v[X]) );
  };

  // Close polygon and start tesselating.
  // The callback is called for each triangle identified.

  int nverts = polygon.size();
  if (nverts == 3)
    emitTriangle(headV);

  if (nverts <= 3)
  {
    while (headV)
      headV = removeVertex(headV);
    return true;
  }

  // We have two or more triangles.
  // Find the best projection based on the surface normal.

  enum { OXY, OXZ, OYZ } projection;
  if (fabs(normal.x()) > fabs(normal.y()))
    projection = fabs(normal.x()) > fabs(normal.z()) ? OYZ : OXY;
  else
    projection = fabs(normal.y()) > fabs(normal.z()) ? OXZ : OXY;

  int Dir = 0;
  switch (projection) {
  case OYZ:
    X = 1;
    Y = 2;
    Dir = normal.x() > 0.0 ? 1 : -1;
    break;
  case OXY:
    X = 0;
    Y = 1;
    Dir = normal.z() > 0.0 ? 1 : -1;
    break;
  case OXZ:
    X = 2;
    Y = 0;
    Dir = normal.y() > 0.0 ? 1 : -1;
    break;
  }

  tailV->next = headV; // make a loop in the linked list

  Vertex* v = headV;
  while (nverts > 3)
    if (float dv = det(v); fabsf(dv) <= FDT_EPSILON)
    {
      // Degenerated triangle, ignore
      cutTriangle(v);
      nverts--;
      headV = v; // JJS 29.06.99
    }
    else if (dv*Dir > 0.0f && clippable(v))
    {
      // A valid triangle, add to triangle container
      emitTriangle(v);
      nverts--;
      headV = v; // JJS 29.06.99
    }
    else
    {
      v = v->next;

      // Added by JJS 29.06.99
      // If gone around the whole polygon, bail out not touching anything.
      // Can't tesselate this polygon (eternal loop detected).
      if (v == headV)
        return false;
    }

  // The last triangle
  emitTriangle(v);
  headV = v;
  do
    v = removeVertex(v);
  while (v != headV);

  return true;
}
