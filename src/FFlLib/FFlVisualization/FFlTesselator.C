// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#include "FFlLib/FFlVisualization/FFlTesselator.H"
#include "FFaLib/FFaAlgebra/FFaVec3.H"

#define FDT_EPSILON 1.0e-6f // tolerance for float comparison

FFlTesselator::Vertex* FFlTesselator::headV = NULL;
FFlTesselator::Vertex* FFlTesselator::tailV = NULL;
int FFlTesselator::X = 0;
int FFlTesselator::Y = 0;
int FFlTesselator::Dir = 0;

void (*FFlTesselator::myCallback)(int,int,int) = NULL;
std::vector<IntVec>* FFlTesselator::myTriangles = NULL;


bool FFlTesselator::tesselate(std::vector<IntVec>& shapeIndexes,
                              const std::list<int>& polygon,
                              const std::vector<FaVec3*>& vertexes,
                              const FaVec3& normal)
{
  // Add all vertices along the edge, supply the ID to be
  // sent with callback when a triangle is identified
  headV = tailV = NULL;
  for (int vxId : polygon)
  {
    const FaVec3& v = *vertexes[vxId];
    Vertex* newv = new Vertex(vxId,v.x(),v.y(),v.z());
    if (!headV) headV = newv;
    if (tailV) tailV->next = newv;
    tailV = newv;
  }

  myTriangles = &shapeIndexes;

  enum {OXY,OXZ,OYZ};

  Vertex *v,*stv;

  // Lambda function removing a Vertex from the linked list
  Vertex* (*removeVertex)(Vertex*) = [](Vertex* vtx) -> Vertex*
  {
    Vertex* nxt = vtx->next;
    delete vtx;
    return nxt;
  };

  // Close polygon and start tesselating.
  // The callback is called for each triangle identified.

  int nverts = polygon.size();
  if (nverts > 3)
    {
      // Find best projection

      int projection;
      if (fabs(normal.x()) > fabs(normal.y()))
        projection = fabs(normal.x()) > fabs(normal.z()) ? OYZ : OXY;
      else
        projection = fabs(normal.y()) > fabs(normal.z()) ? OXZ : OXY;

      switch (projection)
        {
        case OYZ:
          X=1;
          Y=2;
          Dir = normal.x() > 0.0 ? 1 : -1;
          break;
        case OXY:
          X=0;
          Y=1;
          Dir = normal.z() > 0.0 ? 1 : -1;
          break;
        case OXZ:
          X=2;
          Y=0;
          Dir = normal.y() > 0.0 ? 1 : -1;
          break;
        }

      // Make loop
      tailV->next=headV;
      Vertex* newLoopStart = headV; // JJS 29.06.99

      v=headV;
      while (nverts > 3)
	if (fabsf(det(v)) <= FDT_EPSILON)
	  {
	    cutTriangle(v);
	    nverts--;
	    newLoopStart = v; // JJS 29.06.99
	  }
	else if (isTriangle(v) && clippable(v))
	  {
	    emitTriangle(v);
	    nverts--;
	    newLoopStart = v; // JJS 29.06.99
	  }
	else
	  {
	    v=v->next;

	    // Added by JJS 29.06.99
	    // If gone around the whole polygon, bail out not touching anything.
	    // Can't tesselate this polygon (eternal loop detected).
	    if (v == newLoopStart)
	      return false;
	  }

      emitTriangle(v);
      stv=v;
      do
        v=removeVertex(v);
      while (v!=stv);
    }
  else
    {
      if (nverts == 3)
        emitTriangle(headV);
      v=headV;
      while (v)
        v=removeVertex(v);
    }

  return true;
}


bool FFlTesselator::pointInTriangle(Vertex *p, Vertex *t)
{
  bool tst=false;

  float x = p->v[X];
  float y = p->v[Y];

  float* v1 = t->v;
  float* v2 = t->next->next->v;

  if ((   ((v1[Y] <= y) && (y < v2[Y]))  // y of point between y of the triangle line
       || ((v2[Y] <= y) && (y < v1[Y]))) // Taking care of v2[Y] < v1[Y]
      && (x < (v2[X] - v1[X]) * (y - v1[Y]) /  (v2[Y] - v1[Y]) + v1[X]) )
    tst = !tst;

  v2=v1;
  v1=t->next->v;

  if ((   ((v1[Y]<=y) && (y<v2[Y]))
       || ((v2[Y]<=y) && (y<v1[Y])))
      && (x < (v2[X] - v1[X]) * (y - v1[Y]) /  (v2[Y] - v1[Y]) + v1[X]) )
    tst = !tst;

  v2=v1;
  v1=t->next->next->v;

  if ((   ((v1[Y]<=y) && (y<v2[Y]))
       || ((v2[Y]<=y) && (y<v1[Y])))
      && (x < (v2[X] - v1[X]) * (y - v1[Y]) /  (v2[Y] - v1[Y]) + v1[X]) )
    tst = !tst;

  return tst;
}


/*!
  Checks wether two line segments intersect
  Based on the vector equation :
  P11 + d1*P1 = P21 + d2*P2
  where
  P11 is start point of line one
  P21 is start point of line two
  P1, and P2 are vectors along the line segment
  d1 and d2 are "distances" to the intersection
*/

bool FFlTesselator::isLinesIntersecting(float p11x, float p11y,
					float p12x, float p12y,
					float p21x, float p21y,
					float p22x, float p22y)
{
  // Calculate the vectors along the line

  float p1x = p12x-p11x;
  float p1y = p12y-p11y;
  float p2x = p22x-p21x;
  float p2y = p22y-p21y;

  // Calculate the coefficients that makes
  // p1*d1 == p2*d2 == intersection point

  if (fabsf(p1x*p2y - p1y*p2x) <= FDT_EPSILON)
    return false; // Should calculate point-line intersection but ...

  // Lines are neither degenerated nor parallel
  float d1 = (p21x*p2y - p11x*p2y + p2x*p11y - p2x*p21y) / (p1x*p2y - p1y*p2x);
  float d2;
  if (fabsf(p2y) > FDT_EPSILON)
    d2 = (p11y - p21y + d1*p1y) / p2y;
  else if (fabsf(p2x) > FDT_EPSILON)
    d2 = (p11x - p21x + d1*p1x) / p2x;
  else
    return false;

  // If coefficients indicates that the intersection are within
  // both lines return true ( found intersection )
  return (FDT_EPSILON < d1 && d1 < 1.0f-FDT_EPSILON &&
          FDT_EPSILON < d2 && d2 < 1.0f-FDT_EPSILON);
}


/*!
  Check if v points to a legal triangle (normal is right way).
*/

bool FFlTesselator::isTriangle(Vertex* v)
{
  return det(v)*Dir > 0.0f;
}


/*!
  Check if there are no intersection to the triangle pointed to by tri.
  Assumes at least 4 vertexes in polygon.
*/

bool FFlTesselator::clippable(Vertex* tri)
{
  Vertex* vtx = tri->next->next->next;

#if 1
  // Lambda function comparing two vertices
  bool (*Equal)(Vertex*,Vertex*)=[](Vertex* a, Vertex* b) -> bool
  {
    if (a != b)
      for (int i = 0; i < 3; i++)
        if (fabsf(a->v[i] - b->v[i]) > FDT_EPSILON)
          return false;
    return true;
  };

  do {
    if (!Equal(vtx, tri) &&
        !Equal(vtx, tri->next) &&
        !Equal(vtx, tri->next->next) &&
        pointInTriangle(vtx, tri))
      return false;

    vtx = vtx->next;
  } while (vtx != tri);

#else
  if (pointInTriangle(vtx, tri))
    return false;

  if (vtx->next == tri)
    return true;

  vtx = vtx->next;

  while (vtx->next != tri)
    {
      if ( FFlTesselator::isLinesIntersecting
           ( tri->v[X]            , tri->v[Y],
             tri->next->next->v[X], tri->next->next->v[Y],
             vtx->v[X]            , vtx->v[Y],
             vtx->next->v[X]      , vtx->next->v[Y]))
        return false;
      vtx = vtx->next;
    }

  if (pointInTriangle(vtx, tri))
    return false;
#endif

  return true;
}


void FFlTesselator::emitTriangle(Vertex *t)
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


/*!
  Throw away triangle.
*/

void FFlTesselator::cutTriangle(Vertex* t)
{
  Vertex* tmp = t->next;
  if (tmp) {
    t->next = tmp->next;
    delete tmp;
  }
}


float FFlTesselator::det(Vertex* v0)
{
  Vertex* v1 = v0->next;
  Vertex* v2 = v1->next;
  return ( (v1->v[X] - v0->v[X]) * (v2->v[Y] - v0->v[Y]) -
	   (v1->v[Y] - v0->v[Y]) * (v2->v[X] - v0->v[X]) );
}
