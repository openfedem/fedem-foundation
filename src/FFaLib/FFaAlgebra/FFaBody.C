// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#include <algorithm>
#include <set>
#include <map>
#include <cfloat>

#include "FFaLib/FFaAlgebra/FFaBody.H"
#include "FFaLib/FFaAlgebra/FFaMat34.H"


/*!
  Returns the area of a triangle spanned by tree vertices, \a v0, \a v1, \a v2.
  The area is negative if its normal vector is opposite of \a planeNormal.
*/

static double getArea(const FaVec3& planeNormal,
		      const FaVec3& v0, const FaVec3& v1, const FaVec3& v2)
{
  FaVec3 vn = (v1-v0) ^ (v2-v0);
  if (vn * planeNormal < 0.0)
    return -0.5 * vn.length();
  else
    return 0.5 * vn.length();
}


/*!
  Default face constructor.
*/

FaFace::FaFace()
{
  owner = NULL;
  myVertices.resize(3,0);
  iEdge = { 0, 0 };
  IAmBelow = false;
}


/*!
  Constructor defining a face spanned by three or four vertices.
*/

FaFace::FaFace(FFaBody* body, size_t i1, size_t i2, size_t i3, int i4)
{
  owner = body;
  if (i4 < 0)
    myVertices = { i1, i2, i3 };
  else
    myVertices = { i1, i2, i3, (size_t)i4 };
  iEdge = { 0, 0 };
  IAmBelow = false;
#if FFA_DEBUG > 2
  std::cout <<"New face: "<< *this << std::endl;
#endif
}


/*!
  Global stream operator printing the face definition.
*/

std::ostream& operator<<(std::ostream& s, const FaFace& f)
{
  s << f.myVertices.front();
  for (size_t i = 1; i < f.myVertices.size(); i++) s <<','<< f.myVertices[i];
  return s;
}


/*!
  This method determines the intersection between a triangular face and a plane
  defined by a \a normal vector and the distance \a z0 from the origin along the
  global z-axis. If the face is intersected, it is subdivided into two or
  three sub-faces with one or two new vertices.
  \return {-4: Intersection of quadrilateral not implemented}
  \return {-1: The triangle is entirely below the plane}
  \return { 1: The triangle is entirely above the plane}
  \return { 0: The triangle lies in the plane}
  \return { 2: The triangle is intersected and divided in two sub-triangles}
  \return { 3: The triangle is intersected and divided in three sub-triangles}
*/

int FaFace::intersect(const FaVec3& normal, double z0, double zeroTol)
{
  const int nVert = myVertices.size();
  if (nVert > 4) return -nVert;

  IAmBelow = false;
  mySubFaces.clear();
  iEdge = { 0, 0 };

  // Check if each vertex is either above, below or on the plane
  int i, sum = 0;
  char status[4] = { 0, 0, 0, 0 };
  double dist[4];
  for (i = 0; i < nVert; i++)
  {
    dist[i] = this->vertex(i) * normal - z0;
    if (dist[i] > zeroTol)
      status[i] = 1;
    else if (dist[i] < -zeroTol)
      status[i] = -1;
    sum += status[i];
  }
#if FFA_DEBUG > 1
  std::cout <<"Intersecting Face "<< *this <<" at "<< z0 <<"\n\t"<< dist[0];
  for (i = 1; i < nVert; i++) std::cout <<" "<< dist[i];
  std::cout <<" ==> "<< sum << std::endl;
#endif

  if (nVert == 4)
  {
    // Check if the quadrilateral face is entirely above or below
    int check = status[0];
    for (i = 1; i < 4 && check > -4; i++)
      if (status[i])
      {
        if (!check)
          check = status[i];
        else if (status[i] != check) // Intersected quadrilateral
          switch (sum) {
          case 0:
            // The plane intersects two opposite edges or vertices
            // ==> divide the quadrilateral into two sub-faces
            return this->quad2Quads(status,dist);
          case  2:
          case -2:
            // The plane intersects two neighbooring edges
            // ==> divide the quadrilateral into one quad and one triangle
            return this->quad2QuadTria(status,dist,sum < 0);
          default: // Other cases not implemented yet
            check = -4;
          }
      }

    if (check == -1) IAmBelow = true;
    return check;
  }

  // Quick exit if the face is either entirely above or below the plane
  if (sum > 1)
    return 1; // entirely above
  else if (sum < -1)
  {
    IAmBelow = true;
    return -1; // entirely below
  }

  // Then check if at least two vertices are on the plane
  if (status[0] * status[1] * status[2] == 0)
  {
    if (sum == 1)
    {
      // Two vertices are on the plane and the third is above.
      // Although this face is not intersected, we still need the edge defined
      // by the two in-plane vertices for the section area calculation
      int i1 = status[0] == 1 ? 1 : (status[1] == 1 ? 2 : 0);
      int i2 = (i1+1)%3;
      iEdge = { myVertices[i1], myVertices[i2] };
      return 1;
    }
    else if (sum == -1)
    {
      // Two vertices are on the plane and the third is below
      IAmBelow = true;
      return -1;
    }
    else if (status[0] == 0 && status[1] == 0 && status[2] == 0)
      return 0; // all three vertices are on the plane
  }

  // Now we know that the triangle actually is intersected
  if (sum == 0)
  {
    // One vertex is on the plane, and the other two on opposite sides of it
    // ==> divide the triangle into two sub-triangles
    int i0 = status[0] == 0 ? 0 : (status[1] == 0 ? 1 : 2);
    int i1 = (i0+1)%3;
    int i2 = (i1+1)%3;
    double xi = dist[i2] / (dist[i2]-dist[i1]);
    size_t newVertex = owner->addVertex(this->vertex(i1)*xi +
					this->vertex(i2)*(1.0-xi));
    mySubFaces.push_back(FaFace(this->getBody(),
				myVertices[i0],myVertices[i1],newVertex));
    mySubFaces.push_back(FaFace(this->getBody(),
				myVertices[i0],newVertex,myVertices[i2]));
    if (status[i1] == 1)
    {
      mySubFaces[1].IAmBelow = true;
      iEdge = { newVertex, myVertices[i0] };
    }
    else // status[i2] == 1
    {
      mySubFaces[0].IAmBelow = true;
      iEdge = { myVertices[i0], newVertex };
    }
  }
  else // abs(sum) == 1
  {
    // The plane intersects two edges
    // ==> divide the triangle into three
    int i0 = status[0] == -sum ? 0 : (status[1] == -sum ? 1 : 2);
    int i1 = (i0+1)%3;
    int i2 = (i1+1)%3;
    double xi1 = dist[i1] / (dist[i1]-dist[i0]);
    double xi2 = dist[i2] / (dist[i2]-dist[i0]);
    size_t newVertex0 = owner->addVertex(this->vertex(i0)*xi1 +
					 this->vertex(i1)*(1.0-xi1));
    size_t newVertex1 = owner->addVertex(this->vertex(i0)*xi2 +
					 this->vertex(i2)*(1.0-xi2));
    mySubFaces.push_back(FaFace(this->getBody(),
				myVertices[i0],newVertex0,newVertex1));
    if (fabs(dist[i1]) > fabs(dist[i2]))
    {
      mySubFaces.push_back(FaFace(this->getBody(),
				  newVertex1,newVertex0,myVertices[i1]));
      mySubFaces.push_back(FaFace(this->getBody(),
				  newVertex1,myVertices[i1],myVertices[i2]));
    }
    else
    {
      mySubFaces.push_back(FaFace(this->getBody(),
				  newVertex1,newVertex0,myVertices[i2]));
      mySubFaces.push_back(FaFace(this->getBody(),
				  newVertex0,myVertices[i1],myVertices[i2]));
    }
    if (status[i0] == 1)
    {
      mySubFaces[1].IAmBelow = mySubFaces[2].IAmBelow = true;
      iEdge = { newVertex0, newVertex1 };
    }
    else // status[i0] == -1
    {
      mySubFaces[0].IAmBelow = true;
      iEdge = { newVertex1, newVertex0 };
    }
  }

  return mySubFaces.size();
}


int FaFace::quad2Quads(const char* status, const double* dist)
{
  int q0 = -1, t0 = -1;
  if (status[0] < 0 && status[1] < 0)
    q0 = 0;
  else if (status[1] < 0 && status[2] < 0)
    q0 = 1;
  else if (status[2] < 0 && status[3] < 0)
    q0 = 2;
  else if (status[3] < 0 && status[0] < 0)
    q0 = 3;
  else if (status[0] == 0 && status[2] == 0)
    t0 = 0;
  else if (status[1] == 0 && status[2] == 0)
    t0 = 1;
  else
    return -4; // Invalid intersection status

  if (q0 >= 0)
  {
    // The plane intersects two opposite edges
    // ==> divide the quadrilateral into two sub-quads
    int q1 = (q0+1)%4;
    int q2 = (q1+1)%4;
    int q3 = (q2+1)%4;
    double xi1 = dist[q3] / (dist[q3]-dist[q0]);
    double xi2 = dist[q2] / (dist[q2]-dist[q1]);
    size_t iV0 = owner->addVertex(this->vertex(q0)*xi1 +
                                  this->vertex(q3)*(1.0-xi1));
    size_t iV1 = owner->addVertex(this->vertex(q1)*xi2 +
                                  this->vertex(q2)*(1.0-xi2));
    mySubFaces.push_back(FaFace(this->getBody(),
                                myVertices[q0],myVertices[q1],iV1,iV0));
    mySubFaces.push_back(FaFace(this->getBody(),
                                iV0,iV1,myVertices[q2],myVertices[q2]));
    mySubFaces[0].IAmBelow = true;
    iEdge = { iV0, iV1 };
  }
  else if (t0 >= 0)
  {
    // The plane intersects two opposite vertices
    // ==> divide the quadrilateral into two triangles
    int t1 = (t0+1)%4;
    int t2 = (t1+1)%4;
    int t3 = (t2+1)%4;
    mySubFaces.push_back(FaFace(this->getBody(),
                                myVertices[t0],myVertices[t1],myVertices[t2]));
    mySubFaces.push_back(FaFace(this->getBody(),
                                myVertices[t0],myVertices[t2],myVertices[t3]));
    mySubFaces[status[t1] < 0 ? 0 : 1].IAmBelow = true;
    iEdge = { myVertices[t0], myVertices[t2] };
  }

  return mySubFaces.size();
}


int FaFace::quad2QuadTria(const char* status, const double* dist, bool oneAbove)
{
  int i, q1 = -1;
  for (i = 0; i < 4 && q1 < 0; i++)
    if (status[i] > 0 && oneAbove)
      q1 = i;
    else if (status[i] < 0 && !oneAbove)
      q1 = i;

  if (q1 < 0)
    return -4; // Invalid intersection status

  int q0 = (q1+3)%4; // vertex before
  int q2 = (q0+2)%4; // vertex after
  int q3 = (q2+1)%4; // opposite vertex
  double xi0 = dist[q0] / (dist[q0]-dist[q1]);
  double xi2 = dist[q2] / (dist[q2]-dist[q1]);
  size_t iV0 = owner->addVertex(this->vertex(q1)*xi0 +
                                this->vertex(q0)*(1.0-xi0));
  size_t iV2 = owner->addVertex(this->vertex(q1)*xi2 +
                                this->vertex(q2)*(1.0-xi2));
  size_t iV3 = owner->addVertex(this->vertex(q0)*0.5 +
                                this->vertex(q3)*0.5);
  mySubFaces.push_back(FaFace(this->getBody(),
                              myVertices[q0],myVertices[q1],myVertices[q2]));
  mySubFaces.push_back(FaFace(this->getBody(),iV0,iV2,iV3,myVertices[q0]));
  mySubFaces.push_back(FaFace(this->getBody(),
                              iV2,myVertices[q2],myVertices[q3],iV3));
  iEdge = { iV0, iV2 };
  if (oneAbove)
    mySubFaces[1].IAmBelow = mySubFaces[2].IAmBelow = true;
  else
    mySubFaces[0].IAmBelow = true;

  return mySubFaces.size();
}


/*!
  Accumulates area and centroid with contributions from a triangle defined
  by the two internal vertices of this face, the center vertex \a v0, and
  the normal vector \a vn of the face.
*/

double FaFace::accumulateArea(const FaVec3& vn, const FaVec3& v0,
                              FaVec3& Xac) const
{
  FaVec3 v1 = owner->getVertex(iEdge.first);
  FaVec3 v2 = owner->getVertex(iEdge.second);
  double A  = getArea(vn,v0,v1,v2);
  Xac += (v0 + v1 + v2)*A/3.0;
  return A;
}


FaVec3 FaFace::getIntEdgeCoord() const
{
  return owner->getVertex(iEdge.first) + owner->getVertex(iEdge.second);
}


/*!
  Accumulates volume, volume centroid, and optionally volume inertia
  with contributions from a tetrahedron defined by the given four vertices.
*/

static double accVol(FaVec3 v0, FaVec3 v1, FaVec3 v2, FaVec3 v3,
                     FaVec3& Xvc, FFaTensor3* I)
{
  double Vol = (((v1-v0) ^ (v2-v0)) * (v3-v0)) / 6.0;
  FaVec3 Xc  = (v0 + v1 + v2 + v3)/4.0;

  if (I)
  {
    v0 -= Xc;
    v1 -= Xc;
    v2 -= Xc;
    v3 -= Xc;
    FFaTensor3 Iv = FFaTensor3(v0,v2,v1) + FFaTensor3(v0,v1,v3)
                  + FFaTensor3(v1,v2,v3) + FFaTensor3(v0,v3,v2);
    *I += Iv.translateInertia(-v0,Vol);
  }

  Xvc += Xc*Vol;
  return Vol;
}


/*!
  Accumulates volume, volume centroid, and optionally volume inertia
  with contributions from a tetrahedron defined by this face
  and the center vertex \a v0.
*/

double FaFace::accumulateVolume(const FaVec3& v0,
                                FaVec3& Xc, FFaTensor3* I) const
{
  double V = accVol(v0,this->vertex(0),this->vertex(1),this->vertex(2),Xc,I);

  if (myVertices.size() > 3)
    V += accVol(v0,this->vertex(0),this->vertex(2),this->vertex(3),Xc,I);

#if FFA_DEBUG > 1
  std::cout <<"vol("<< *this <<") = "<< V << std::endl;
#endif
  return V;
}


/*!
  Adds a face to the body definition.
*/

size_t FFaBody::addFace(int i1, int i2, int i3, int i4)
{
#if FFA_DEBUG > 2
  std::cout <<"New face: "<< myFaces.size() << std::endl;
#endif
  myFaces.push_back(FaFace(this,i1,i2,i3,i4));
  return myFaces.size()-1;
}


/*!
  Adds a vertex to the body definition.
*/

size_t FFaBody::addVertex(const FaVec3& pos, double tol)
{
  if (tol >= 0.0)
  {
    // Eliminate duplicated vertices
    std::vector<FaVec3>::const_iterator it;
    it = std::find_if(myVertices.begin(),myVertices.end(),
                      [&pos,tol](const FaVec3& x){ return pos.equals(x,tol); });
    if (it != myVertices.end())
    {
      size_t idx = it - myVertices.begin();
#if FFA_DEBUG > 2
      std::cout <<"Existing vertex: "<< idx <<" "<< pos << std::endl;
#endif
      return idx;
    }
  }

#if FFA_DEBUG > 2
  std::cout <<"New vertex: "<< myVertices.size() <<" "<< pos << std::endl;
#endif
  myVertices.push_back(pos);
  return myVertices.size()-1;
}


/*!
  Computes the bounding box of the body.
*/

bool FFaBody::computeBoundingBox(FaVec3& minX, FaVec3& maxX) const
{
  if (myVertices.empty()) return false;

  minX = maxX = myVertices.front();
  for (size_t i = 1; i < myVertices.size(); i++)
    for (int j = 0; j < 3; j++)
      if (myVertices[i][j] < minX[j])
        minX[j] = myVertices[i][j];
      else if (myVertices[i][j] > maxX[j])
        maxX[j] = myVertices[i][j];

  return true;
}


/*!
  Computes the total volume, volume centroid, and optionally the volume inertia.
*/

bool FFaBody::computeTotalVolume(double& Vb, FaVec3& C0b, FFaTensor3* Ib) const
{
  char wantVolume = Ib ? 2 : 1;
  if (isVolumeComputed >= wantVolume)
  {
    Vb = myVolume;
    C0b = myCentroid;
    if (Ib) *Ib = myInertia;
    return true;
  }

  Vb  = 0.0;
  C0b = FaVec3();
  if (Ib) *Ib = FFaTensor3();
  if (!isBBoxComputed)
  {
    if (this->computeBoundingBox(myBBox[0],myBBox[1]))
      isBBoxComputed = true;
    else
      return false;
  }

  const double eps = 100.0*DBL_EPSILON;

  // Estimate centroid based on the bounding box
  FaVec3 X0 = 0.5*(myBBox[1]+myBBox[0]);
  // Integrate volume, centroid and inertia by summing tetrahedron contributions
  for (size_t i = 0; i < myFaces.size(); i++)
    Vb += myFaces[i].accumulateVolume(X0,C0b,Ib);
  if (fabs(Vb) < eps) return false;


  C0b /= Vb;
  myVolume = Vb;
  myCentroid = C0b;
  if (Ib)
  {
    // Adjust the inertias according to the real volume centroid
    Ib->translateInertia(X0-C0b,-Vb);
    myInertia = *Ib;
  }

  isVolumeComputed = wantVolume;
  return true;
}


/*!
  Computes the volume and centroid of the portion of the body that is below the
  plane defined by \a normal and \a z0. The area of the intersection surface and
  the associated centroid is also computed, if the body is intersected.
*/

bool FFaBody::computeVolumeBelow(double& Vb, double& As,
				 FaVec3& C0b, FaVec3& C0s,
				 const FaVec3& normal, double z0,
				 double zeroTol)
{
  Vb = As = 0.0;
  myVertices.resize(startVx); // Erase existing additional vertices

  // Loop over all faces and check for intersection,
  // accumulate an estimate of the intersection surface centroid
  FaVec3 X0s;
  double nVs = 0.0;
  size_t i, j, nBelow = 0, nQuad = 0;
  for (i = 0; i < myFaces.size(); i++)
    switch (myFaces[i].intersect(normal,z0,zeroTol))
      {
      case 2: // The face is divided in 2 sub-faces
      case 3: // The face is divided in 3 sub-faces
	X0s += myFaces[i].getIntEdgeCoord();
	nVs += 2.0;
	break;

      case  0: // The face lies in the plane
      case -1: // The face is entirely below the plane
	nBelow++;
	break;

      case -4: // Intersected quadrilateral (unsupported)
	nQuad++;
      }

#if FFA_DEBUG > 1
  std::cout <<"FFaBody::computeVolumeBelow: nVs="<< nVs
            <<" nBelow="<< nBelow << std::endl;
#endif
  if (nQuad > 0)
    std::cerr <<"FFaBody::computeVolumeBelow: Detected "<< nQuad
	      <<" intersected quadrilateral faces,"
	      <<" volume calculation will be inaccurate."<< std::endl;

  if (nBelow == myFaces.size())
    return this->computeTotalVolume(Vb,C0b); // The body is entirely below
  else if (nVs < 2.0)
    return false; // The body is entirely above the plane

  // Integrate the partial volume and intersection surface area
  X0s /= nVs;
  C0b = C0s = FaVec3();
  for (i = 0; i < myFaces.size(); i++)
    if (myFaces[i].isBelow())
      Vb += myFaces[i].accumulateVolume(X0s,C0b,NULL);
    else if (myFaces[i].isIntersected())
    {
      const std::vector<FaFace>& subFaces = myFaces[i].getSubFaces();
      for (j = 0; j < subFaces.size(); j++)
	if (subFaces[j].isBelow())
	  Vb += subFaces[j].accumulateVolume(X0s,C0b,NULL);

      As += myFaces[i].accumulateArea(normal,X0s,C0s);
    }

  C0b /= Vb;
  C0s /= As;
  return true;
}


/*!
  Saves the loop of vertices defining the current intersection surface.
  The vertex coordinates of the loop are transformed to global coordinates
  using \a cs to facilitate comparison with a previous configuration.
*/

bool FFaBody::saveIntersection(const FaMat34& cs)
{
  myIntLoop.clear();
  myLoopVer.clear();
  myX0s.clear();

  // Find the set of vertex indices that are used by intersection edges
  // and establish the loop connectivity
  size_t i;
  std::set<size_t> loopVertices;
  std::set<size_t>::const_iterator it;
  for (i = 0; i < myFaces.size(); i++)
    if (myFaces[i].isIntersected())
    {
      myIntLoop.push_back(myFaces[i].getIntEdge());
      loopVertices.insert(myIntLoop.back().first);
      loopVertices.insert(myIntLoop.back().second);
    }

  if (myIntLoop.empty()) return false; // no intersection

  // Store the vertex coordinates for those that are used by the loop
  // and find mapping from global vertex index to loop vertex index
  std::map<size_t,size_t> vMap;
  for (i = 0, it = loopVertices.begin(); it != loopVertices.end(); i++, it++)
  {
    vMap[*it] = i;
    myLoopVer.push_back(cs*myVertices[*it]);
    myX0s += myLoopVer.back();
  }
  myX0s /= (double)myLoopVer.size();

  // Replace the global vertex indices by the loop vertex indices
  for (i = 0; i < myIntLoop.size(); i++)
  {
    myIntLoop[i].first  = vMap[myIntLoop[i].first];
    myIntLoop[i].second = vMap[myIntLoop[i].second];
  }

  return true;
}


/*!
  Computes the increment in the intersection area and the associated centroid.
  It is assumed that \a computeVolumeBelow already has been invoked before
  calling this method such that the current face intersections are available.
*/

bool FFaBody::computeIncArea(double& dAs, FaVec3& C0s,
			     const FaVec3& normal, const FaMat34& cs)
{
  dAs = 0.0;
  C0s = FaVec3();
  FaMat34 invCS = cs.inverse();
  FaVec3 X0s = invCS * myX0s;

  // Integrate the current intersection surface area
  size_t i;
  for (i = 0; i < myFaces.size(); i++)
    if (myFaces[i].isIntersected())
      dAs += myFaces[i].accumulateArea(normal,X0s,C0s);

  if (dAs <= 0.0) return false;

  // Subtract the previous area
  for (i = 0; i < myIntLoop.size(); i++)
  {
    FaVec3 v1 = invCS*myLoopVer[myIntLoop[i].first];
    FaVec3 v2 = invCS*myLoopVer[myIntLoop[i].second];
    double area = getArea(normal,X0s,v1,v2);
    dAs -= area;
    C0s -= area*(X0s + v1 + v2)/3.0;
  }

  C0s /= dAs;
  return true;
}
