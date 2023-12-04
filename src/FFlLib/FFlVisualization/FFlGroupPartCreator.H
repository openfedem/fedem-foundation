// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#ifndef FFL_GROUP_PART_CREATOR_H
#define FFL_GROUP_PART_CREATOR_H

#include <list>
#include <map>

#include "FFlLib/FFlVisualization/FFlFaceGenerator.H"
#include "FFlLib/FFlVisualization/FFlVisFace.H"
#include "FFaLib/FFaOperation/FFaOperation.H"

class FdFEGroupPart;
class FaVec3;

typedef std::pair<FFlVisFace*,int> FFlVisFaceIdx;
typedef std::pair<FFlVisEdge*,int> FFlVisEdgeIdx;


struct FFlGroupPartData
{
  bool isIndexShape = true;
  bool isLineShape  = false;

  //! Pointer to the visualization of this GroupPart
  FdFEGroupPart* visualModel = NULL;

  //! The visible faces with idx to the start of the results for the face
  std::vector<FFlVisFaceIdx> facePointers;

  //! The hidden faces with idx to the start of the results for the face
  std::vector<FFlVisFaceIdx> hiddenFaces;

  //! The visible edges with idx to the start of the results for the edges
  std::vector<FFlVisEdgeIdx> edgePointers;

  //! The hidden  edges with idx to the start of the results for the edges
  std::vector<FFlVisEdgeIdx> hiddenEdges;

  //! Number of vertexes in the visible edge/face set.
  int nVisiblePrimitiveVertexes = 0;

  //! Faces or line indexes if not using faces or edges
  std::vector< std::vector<int> > shapeIndexes;

  //! Fringe operations for results
  std::vector< FFaOperation<double>* > colorOps;

  //! Extract shape indices for visualization
  void getShapeIndexes(int* idx) const;
};


class FFlGroupPartCreator : public FFlFaceGenerator
{
public:
  FFlGroupPartCreator(FFlLinkHandler* lh);
  virtual ~FFlGroupPartCreator();

  enum GroupPartType {
    RED_OUTLINE_LINES,  OUTLINE_LINES,
    RED_SURFACE_LINES,  SURFACE_LINES,
    RED_SURFACE_FACES,  SURFACE_FACES,
    RED_INTERNAL_LINES, INTERNAL_LINES,
    RED_INTERNAL_FACES, INTERNAL_FACES,
    RED_SPECIAL_LINES,  SPECIAL_LINES,
    NUM_TYPES
  };

  typedef std::map<unsigned short int,FFlGroupPartData*> GroupPartMap;

  // Create internally stored geometry

  void makeLinkParts();
  void deleteShapeIndexes();
  bool recreateSpecialLines(double XZscale = 0.0);
  void updateElementVisibility();

  // Access the internally stored geometry

  const GroupPartMap& getLinkParts() const { return myLinkParts; }
  const GroupPartMap& getSpecialLines() const { return mySpecialLines; }

  FFlFaceGenerator* getFaceGenerator() { return this; }

  // Parameters for the geometrical algorithms

  void   setEdgesParallelAngle(double val)    { myEdgesParallelAngle = val; }
  double getEdgesParallelAngle() const        { return myEdgesParallelAngle;}

  void   setOutlineAngleThreshold(double val) { myOutlineEdgeMinAngle = val; }
  double getOutlineAngleThreshold() const     { return myOutlineEdgeMinAngle;}

  void   setFaceReductionAngle(double val)    { myFaceReductionAngle = val; }
  double getFaceReductionAngle() const        { return myFaceReductionAngle;}

  // Debug printing
  virtual void dump() const;

protected:
  void setEdgeGeomStatus();

  // Link part generation :

  void createLinkFullFaces(FFlGroupPartData& internalFaces,
                           FFlGroupPartData& surfaceFaces);

  void createLinkFullEdges(FFlGroupPartData& internalLines,
                           FFlGroupPartData& surfaceLines,
                           FFlGroupPartData& outlineLines);

  void createLinkReducedFaces(FFlGroupPartData& internalFaces,
                              FFlGroupPartData& surfaceFaces);

  void createLinkReducedEdges(FFlGroupPartData& internalLines,
                              FFlGroupPartData& surfaceLines,
                              FFlGroupPartData& outlineLines);

  void createSpecialLines(double XZscale = 0.0);

  // Convenience for face reduction :

  void expandPolygon       ( std::list<int>& polygon,
                             FFlVisFace&     f,
                             const FaVec3&   normal );

  void joinFacesFromEdge   ( std::list<int>           & polygon,
                             const std::list<int>::iterator& splEdgEndPolyIt,
                             const FFlVisEdgeRef& edgeIt,
                             const FFlVisFace         & previousFace,
                             const bool               & faceIsPositive,
                             const bool               & onlySurfaceFaces,
                             const FaVec3             & normal );

  void insertFaceInPolygon ( std::list<int>           & polygon,
                             const std::list<int>::iterator& splEdgEndPolyIt,
                             const FFlVisFace         * faceToJoin,
                             const VisEdgeRefVecCIter & splEdgeRIt,
                             const bool               & faceToJoinIsPositive );

  void getPolygonFromFace  ( std::list<int>           & polygon,
                             const FFlVisFace         & f,
                             const VisEdgeRefVecCIter & splitEdgeIt,
                             const bool               & faceIsPositive );

private:
  GroupPartMap myLinkParts;    //!< Internal container for stored geometry
  GroupPartMap mySpecialLines; //!< Internal container for special lines

  const std::vector<FaVec3*>& myVertices; //!< Reference to vertex container

  // Algorithm parameters
  double myOutlineEdgeMinAngle;
  double myEdgesParallelAngle;
  double myFaceReductionAngle;

  // Internal control variables
  bool IAmIncludingInOpsDir;
#if defined(win32) || defined(win64)
  int joinFacesFromEdgeMethodCount; // Stack overflow recursivity workaround
#endif
};

#endif
