// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#include "FFlVTFWriter.H"
#include "FFlLib/FFlLinkHandler.H"
#include "FFlLib/FFlElementBase.H"
#include "FFlLib/FFlFEParts/FFlNode.H"
#include "FFaLib/FFaAlgebra/FFaVec3.H"
#include "FFaLib/FFaDefinitions/FFaMsg.H"
#ifdef FFL_TIMER
#include "FFaLib/FFaProfiler/FFaProfiler.H"
#endif
#ifdef FT_HAS_VTF
#include "VTFAPI.h"
#include "VTOAPIPropertyIDs.h"
#else
class VTFAFile{};
#endif

#ifdef FFL_TIMER
#define START_TIMER(function) myProfiler->startTimer(function)
#define STOPP_TIMER(function) myProfiler->stopTimer(function)
#else
#define START_TIMER(function)
#define STOPP_TIMER(function)
#endif


// Mapping from Fedem node order to VTF order for the second order elements

const int FFlVTFWriter::T6m[6] = {
  1, 4, 2, 5, 3, 6 };
//1  2  3  4  5  6

const int FFlVTFWriter::Q8m[8] = {
  1, 5, 2, 6, 3, 7, 4, 8 };
//1  2  3  4  5  6  7  8

const int FFlVTFWriter::T10m[10] = {
  1, 5, 2, 6, 3, 7, 8, 9,10, 4 };
//1  2  3  4  5  6  7  8  9 10

const int FFlVTFWriter::P15m[15] = {
  1, 7, 2, 8, 3, 9,13,14,15, 4,10, 5,11, 6,12 };
//1  2  3  4  5  6  7  8  9 10 11 12 13 14 15

const int FFlVTFWriter::H20m[20] = {
  1, 9, 2,10, 3,11, 4,12,17,18,19,20, 5,13, 6,14, 7,15, 8,16 };
//1  2  3  4  5  6  7  8  9 10 11 12 13 14 15 16 17 18 19 20


FFlVTFWriter::FFlVTFWriter(const FFlLinkHandler* link) : FFlWriterBase(link)
{
#ifdef FFL_TIMER
  myProfiler = new FFaProfiler("VTFWriter profiler");
  myProfiler->startTimer("FFlVTFWriter");
#endif

  myFile = NULL;
}


FFlVTFWriter::~FFlVTFWriter()
{
  if (myFile) delete myFile;

#ifdef FFL_TIMER
  myProfiler->stopTimer("FFlVTFWriter");
  myProfiler->report();
  delete myProfiler;
#endif
}


#ifdef FT_HAS_VTF
bool FFlVTFWriter::writeNodes(int iBlockID, bool withID)
#else
bool FFlVTFWriter::writeNodes(int, bool)
#endif
{
  START_TIMER("writeNodes");
  bool ok = false;
  myNodMap.clear();

#ifdef FT_HAS_VTF
  VTFANodeBlock nodeBlock(iBlockID,withID);

  int nnod = myLink->getNodeCount(FFlLinkHandler::FFL_FEM);
  ok = VTFA_SUCCESS(nodeBlock.SetNumNodes(nnod));

  int inod = 0;
  NodesCIter nit;
  for (nit = myLink->nodesBegin(); nit != myLink->nodesEnd() && ok; nit++)
    if ((*nit)->hasDOFs() &&
        (myNodes.empty() || myNodes.find((*nit)->getID()) != myNodes.end()))
    {
      const FaVec3& v = (*nit)->getPos();
      int nID = (*nit)->getID();
      if (withID)
        ok = VTFA_SUCCESS(nodeBlock.AddNode(v.x(),v.y(),v.z(),nID));
      else
      {
        ok = VTFA_SUCCESS(nodeBlock.AddNode(v.x(),v.y(),v.z()));
        myNodMap[nID] = inod++;
      }
    }

  if (VTFA_FAILURE(myFile->WriteBlock(&nodeBlock)))
    ok = false;
#endif

  STOPP_TIMER("writeNodes");
  return ok;
}


bool FFlVTFWriter::writeElements(
#ifdef FT_HAS_VTF
				 const std::string& partName,
				 int iBlockID, int iNodeBlockID,
				 bool withID, bool convTo1stOrder
#else
				 const std::string&, int, int, bool, bool
#endif
){
  START_TIMER("writeElements");
  bool ok = false;
  myOrder.clear();
  myNodes.clear();

#ifdef FT_HAS_VTF

  // Mapping from FTL to VTF element types

  static std::map<std::string,int> elmTypes;
  if (elmTypes.empty())
  {
    elmTypes["RGD"]    = VTFA_BEAMS;
    elmTypes["BEAM2"]  = VTFA_BEAMS;
    elmTypes["BEAM3"]  = VTFA_BEAMS_3;
    elmTypes["TRI3"]   = VTFA_TRIANGLES;
    elmTypes["TRI6"]   = VTFA_TRIANGLES_6;
    elmTypes["QUAD4"]  = VTFA_QUADS;
    elmTypes["QUAD8"]  = VTFA_QUADS_8;
    elmTypes["QUAD9"]  = VTFA_QUADS_9;
    elmTypes["TET4"]   = VTFA_TETRAHEDRONS;
    elmTypes["TET10"]  = VTFA_TETRAHEDRONS_10;
    elmTypes["WEDG6"]  = VTFA_PENTAHEDRONS;
    elmTypes["WEDG15"] = VTFA_PENTAHEDRONS_15;
    elmTypes["HEX8"]   = VTFA_HEXAHEDRONS;
    elmTypes["HEX20"]  = VTFA_HEXAHEDRONS_20;
  }

  // Set up vectors of element pointers for each element type

  size_t numEls = 0;
  std::map<std::string,ElementsVec> myElements;
  std::map<std::string,int>::const_iterator et;
  for (et = elmTypes.begin(); et != elmTypes.end(); et++)
    if (numEls = myLink->getElementCount(et->first))
      myElements[et->first].reserve(numEls);

  // WAVGM and RGD elements are the same on the VTF
  numEls = myLink->getElementCount("WAVGM");
  if (numEls > 0)
    myElements["RGD"].reserve(myLink->getElementCount("RGD")+numEls);

  size_t iel = 0;
  ElementsCIter eit;
  std::map<FFlElementBase*,int> elmIndex;
  for (eit = myLink->fElementsBegin(); eit != myLink->fElementsEnd(); eit++)
  {
    elmIndex[*eit] = ++iel;
    std::string curTyp = (*eit)->getTypeName();
    if (curTyp == "WAVGM")
      myElements["RGD"].push_back(*eit);
    else if (elmTypes.find(curTyp) != elmTypes.end())
      myElements[curTyp].push_back(*eit);
  }

  // Now write the elements, type by type,
  // and record the output order in myOrder
  // to facilitate consistent output of element result
  myOrder.resize(iel,0);

  VTFAElementBlock elementBlock(iBlockID,withID,withID);

  ok = true;
  iel = 0;
  std::map<std::string,ElementsVec>::const_iterator etit;
  for (etit = myElements.begin(); etit != myElements.end() && ok; etit++)
    if (etit->first != "RGD")
    {
      numEls = etit->second.size();
      if (numEls < 1) continue;

      int ielTyp = elmTypes[etit->first];
      int nenod  = etit->second.front()->getNodeCount();
      int xelTyp = ielTyp;
      if (convTo1stOrder)
	// Replace second order elements by corresponding first order elements
	switch (ielTyp)
	  {
	    case VTFA_BEAMS_3:
	      nenod  = 2;
	      xelTyp = VTFA_BEAMS;
	      break;
	    case VTFA_TRIANGLES_6:
	      nenod  = 3;
	      xelTyp = VTFA_TRIANGLES;
	      break;
	    case VTFA_QUADS_8:
	    case VTFA_QUADS_9:
	      nenod  = 4;
	      xelTyp = VTFA_QUADS;
	      break;
	    case VTFA_TETRAHEDRONS_10:
	      nenod  = 4;
	      xelTyp = VTFA_TETRAHEDRONS;
	      break;
	    case VTFA_PENTAHEDRONS_15:
	      nenod  = 6;
	      xelTyp = VTFA_PENTAHEDRONS;
	      break;
	    case VTFA_HEXAHEDRONS_20:
	      nenod  = 8;
	      xelTyp = VTFA_HEXAHEDRONS;
	      break;
	  }

      int* mmnpc = new int[numEls*nenod+12];
      int* elmId = new int[numEls];

      NodeCIter n;
      int i, nel = 0;
      int* mnpc = mmnpc;
      for (eit = etit->second.begin(); eit != etit->second.end() && ok; ++eit)
      {
	for (i = 0, n = (*eit)->nodesBegin(); n != (*eit)->nodesEnd(); i++, n++)
	  switch (ielTyp)
	    {
	    case VTFA_TRIANGLES_6:
	      mnpc[T6m[i]-1]  = (*n)->getID();
	      break;
	    case VTFA_QUADS_8:
	      mnpc[Q8m[i]-1]  = (*n)->getID();
	      break;
	    case VTFA_TETRAHEDRONS_10:
	      mnpc[T10m[i]-1] = (*n)->getID();
	      break;
	    case VTFA_PENTAHEDRONS_15:
	      mnpc[P15m[i]-1] = (*n)->getID();
	      break;
	    case VTFA_HEXAHEDRONS_20:
	      mnpc[H20m[i]-1] = (*n)->getID();
	      break;
	    default:
	      mnpc[i] = (*n)->getID();
	    }

	switch (ielTyp)
	  {
	  case VTFA_TRIANGLES:
	  case VTFA_TRIANGLES_6:
	  case VTFA_QUADS:
	  case VTFA_QUADS_8:
	  case VTFA_QUADS_9:
	    // Indicate shell elements by negating the element index
	    myOrder[iel++] = -elmIndex[*eit];
	    break;
	  default:
	    myOrder[iel++] = elmIndex[*eit];
	  }

	if (convTo1stOrder)
	  for (i = 0; i < nenod; i++)
	    myNodes.insert(mnpc[i]);

	elmId[nel++] = (*eit)->getID();
	mnpc += nenod;
      }

      if (iNodeBlockID > 0)
      {
	START_TIMER("AddElements");
	if (withID)
	  ok = VTFA_SUCCESS(elementBlock.AddElements(xelTyp,mmnpc,nel,elmId));
	else
	{
	  for (int* k = mmnpc; k != mnpc; k++) *k = myNodMap[*k];
	  ok = VTFA_SUCCESS(elementBlock.AddElements(xelTyp,mmnpc,nel));
	}
	STOPP_TIMER("AddElements");
      }

      delete[] mmnpc;
      delete[] elmId;
    }

  etit = myElements.find("RGD");
  if (etit != myElements.end())
  {
    // Write a beam element for each leg of the RGD (and WAVGM) elements.
    // Since they are written at the end of the element block they will not
    // be assigned any element results.
    int ielTyp = VTFA_BEAMS;
    int mnpc[2];
    for (eit = etit->second.begin(); eit != etit->second.end() && ok; eit++)
    {
      NodeCIter n = (*eit)->nodesBegin(); // the reference node
      if (!(*n)->hasDOFs()) continue; // ommit this element if node is DOF-less

      myOrder[iel++] = elmIndex[*eit];
      int eID = (*eit)->getID();
      mnpc[0] = (*n)->getID();
      if (convTo1stOrder)
	myNodes.insert(mnpc[0]);
      if (!withID)
	mnpc[0] = myNodMap[mnpc[0]];

      for (++n; n != (*eit)->nodesEnd() && ok; ++n)
	if ((*n)->hasDOFs()) // ommit DOF-less nodes
	{
	  mnpc[1] = (*n)->getID();
	  if (convTo1stOrder)
	    myNodes.insert(mnpc[1]);
	  if (!withID)
	    mnpc[1] = myNodMap[mnpc[1]];
	  if (iNodeBlockID > 0)
	  {
	    START_TIMER("AddElement");
	    if (withID)
	      ok = VTFA_SUCCESS(elementBlock.AddElement(ielTyp,mnpc,eID));
	    else
	      ok = VTFA_SUCCESS(elementBlock.AddElement(ielTyp,mnpc));
	    STOPP_TIMER("AddElement");
	  }
	}
    }
  }

  if (iNodeBlockID > 0)
  {
    elementBlock.SetPartID(iBlockID);
    elementBlock.SetPartName(partName.c_str());
    elementBlock.SetNodeBlockID(iNodeBlockID);
    if (VTFA_FAILURE(myFile->WriteBlock(&elementBlock)))
      ok = false;
  }
#endif

  STOPP_TIMER("writeElements");
  return ok;
}


#ifdef FT_HAS_VTF
bool FFlVTFWriter::writeGeometry(const int* pGeometryParts, int iNumParts)
#else
bool FFlVTFWriter::writeGeometry(const int*, int)
#endif
{
  START_TIMER("writeGeometry");
  bool ok = false;

#ifdef FT_HAS_VTF
  VTFAGeometryBlock geoBlock;

  if (VTFA_FAILURE(geoBlock.SetGeometryElementBlocks(pGeometryParts,iNumParts)))
    ok = false;
  else
    ok = true;

  if (VTFA_FAILURE(myFile->WriteBlock(&geoBlock)))
    ok = false;
#endif

  STOPP_TIMER("writeGeometry");
  return ok;
}


#ifdef FT_HAS_VTF
bool FFlVTFWriter::writeProperties(int iBlockID)
#else
bool FFlVTFWriter::writeProperties(int)
#endif
{
  START_TIMER("writeProperties");
  bool ok = false;

#ifdef FT_HAS_VTF
  VTFAPropertiesBlockSimple frameProp(VT_CT_FRAME_GENERATOR_SETTINGS);
  frameProp.AddInt(VT_PI_FG_STATE_IDS,1);
  if (VTFA_FAILURE(myFile->WriteBlock(&frameProp)))
    ok = false;
  else
    ok = true;

  VTFAPropertiesBlockSimple partAttr(VT_CT_PART_ATTRIBUTES);
  partAttr.SetPartID(iBlockID);
  partAttr.AddBool(VT_PB_PA_MESH,1);
  if (VTFA_FAILURE(myFile->WriteBlock(&partAttr)))
    ok = false;
#endif

  STOPP_TIMER("writeProperties");
  return ok;
}


bool FFlVTFWriter::write(
#ifdef FT_HAS_VTF
			 const std::string& filename,
			 const std::string& partname,
			 int ID, int type
#else
			 const std::string&, const std::string&, int, int
#endif
){
  START_TIMER("write");

#ifdef FT_HAS_VTF
  // Some constants for file format and output filename
  bool bExpressFile = type == 2;
  bool bBinaryFile  = type >= 1;

  // Create the VTF file object
  myFile = new VTFAFile();

  // Enable debug info to stderr/console
  myFile->SetOutputDebugError(1);

  if (type < 0)
  {
    if (VTFA_FAILURE(myFile->AppendFile(filename.c_str())))
      return showError("Error appending to VTF file");
  }
  else
  {
    if (bExpressFile)
    {
      // Create the Express VTF file
      // Available only in the commercial version of the VTF Express API
      int iVendorCode = 884625072; // FEDEM vendor code (keep this confidential)

      // Specify to use compression
      int iResult = myFile->CreateExpressFile(filename.c_str(),iVendorCode,1);
      if (iResult == VTFAERR_CANNOT_CREATE_EXPRESS_FILE)
	// Cannot create Express file, must use the GLview VTF writer library
	bExpressFile = false;
      else if (VTFA_FAILURE(iResult))
	return showError("Error creating Express VTF file");
    }

    if (!bExpressFile)
    {
      // Create the Open VTF file, specifying binary file output
      // These formats are always available
      if (VTFA_FAILURE(myFile->CreateVTFFile(filename.c_str(),bBinaryFile)))
	return showError("Error creating VTF file");
    }

    if (!writeGeometry(&ID,1))
      return showError("Error writing geometry");
  }

  if (!writeNodes(ID))
    return showError("Error writing node block",ID);

  if (!writeElements(partname,ID,ID))
    return showError("Error writing element block",ID);

  if (bExpressFile)
    if (!writeProperties(ID))
      return showError("Error writing properties for part",ID);

  if (VTFA_FAILURE(myFile->CloseFile()))
    return showError("Error closing VTF file");

  delete myFile;
  myFile = NULL;
  STOPP_TIMER("write");
  return true;
#else
  return showError("VTF output is not available in this version");
#endif
}


bool FFlVTFWriter::write(
#ifdef FT_HAS_VTF
			 VTFAFile& file, const std::string& partname, int ID,
			 std::vector<int>* outputOrder,
			 std::vector<int>* firstOrderNodes
#else
			 VTFAFile&, const std::string&, int,
                         std::vector<int>*, std::vector<int>*
#endif
){
  START_TIMER("write");

#ifdef FT_HAS_VTF
  if (myFile) delete myFile;
  myFile = &file;

  // A negative ID is used to flag without node and element ID
  bool withID = ID > 0;
  if (ID < 0) ID = -ID;

  // If we are not using node ID's and want to convert to a first-order model,
  // we need to invoke writeElements first only to compute the myNodes array,
  // because the node indices then need to be computed (in writeNodes) before
  // actually writing the elements
  if (!withID && firstOrderNodes)
    writeElements(partname,ID,0,withID,true);

  if ((!withID || !firstOrderNodes) && !writeNodes(ID,withID))
    return showError("Error writing node block",ID);

  if (!writeElements(partname,ID,ID,withID,firstOrderNodes))
    return showError("Error writing element block",ID);

  if (withID && firstOrderNodes && !writeNodes(ID,withID))
    return showError("Error writing node block",ID);

  if (outputOrder)
    outputOrder->assign(myOrder.begin(),myOrder.end());

  if (firstOrderNodes)
    firstOrderNodes->assign(myNodes.begin(),myNodes.end());

  myFile = NULL;
  STOPP_TIMER("write");
  return true;
#else
  return showError("VTF output is not available in this version");
#endif
}


bool FFlVTFWriter::showError(const char* msg, int ID)
{
  ListUI <<" *** "<< msg;
  if (ID) ListUI <<" "<< ID;
  ListUI << "\n";
  STOPP_TIMER("write");
  return false;
}
