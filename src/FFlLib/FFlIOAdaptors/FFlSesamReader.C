// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#include "FFlLib/FFlIOAdaptors/FFlSesamReader.H"
#include "FFlLib/FFlIOAdaptors/FFlReaders.H"
#include "FFlLib/FFlLinkHandler.H"
#include "FFlLib/FFlElementBase.H"
#include "FFlLib/FFlFEElementTopSpec.H"
#include "FFlLib/FFlFEParts/FFlNode.H"
#include "FFlLib/FFlFEParts/FFlPMAT.H"
#include "FFlLib/FFlFEParts/FFlPMASS.H"
#include "FFlLib/FFlFEParts/FFlPTHICK.H"
#include "FFlLib/FFlFEParts/FFlPORIENT.H"
#include "FFlLib/FFlFEParts/FFlPBEAMECCENT.H"
#include "FFlLib/FFlFEParts/FFlPBEAMSECTION.H"
#include "FFlLib/FFlFEParts/FFlPBEAMPIN.H"
#include "FFlLib/FFlFEParts/FFlPSPRING.H"
#include "FFlLib/FFlFEParts/FFlPWAVGM.H"
#include "FFlLib/FFlGroup.H"
#include "FFaLib/FFaAlgebra/FFaVec3.H"
#include "FFaLib/FFaAlgebra/FFaMath.H"
#include "FFaLib/FFaDefinitions/FFaMsg.H"
#ifdef FFL_TIMER
#include "FFaLib/FFaProfiler/FFaProfiler.H"
#endif
#include "Admin/FedemAdmin.H"
#include <fstream>
#include <cstring>

#ifndef LINE_LENGTH
#define LINE_LENGTH 128
#endif

#define CREATE_ATTRIBUTE(type,name,id) \
  dynamic_cast<type*>(AttributeFactory::instance()->create(name,id));


FFlSesamReader::FFlSesamReader(FFlLinkHandler* aLink) : FFlReaderBase(aLink)
{
#ifdef FFL_TIMER
  myProfiler = new FFaProfiler("SesamReader profiler");
  myProfiler->startTimer("FFlSesamReader");
#endif
}

FFlSesamReader::~FFlSesamReader()
{
#ifdef FFL_TIMER
  myProfiler->stopTimer("FFlSesamReader");
  myProfiler->report();
  delete myProfiler;
#endif
}


void FFlSesamReader::init()
{
  FFlReaders::instance()->registerReader("SESAM input file","FEM",
                                         FFaDynCB2S(readerCB,const std::string&,FFlLinkHandler*),
                                         FFaDynCB2S(identifierCB,const std::string&,int&),
                                         "SESAM input file reader v1.0",
                                         FedemAdmin::getCopyrightString());
}


void FFlSesamReader::identifierCB(const std::string& fileName, int& isSesamFile)
{
  // Seek for "IDENT" keyword in the file - check the first 100 lines
  if (!fileName.empty())
    isSesamFile = searchKeyword(fileName.c_str(),"IDENT");
}


void FFlSesamReader::readerCB(const std::string& filename, FFlLinkHandler* link)
{
  FFlSesamReader reader(link);
  if (!reader.read(filename) || !link->resolve(FFlReaders::convertToLinear == 2, true))
    link->deleteGeometry(); // parsing failure, delete all link data
  else
    for (NodesCIter nit = link->nodesBegin(); nit != link->nodesEnd(); ++nit)
      if (!(*nit)->hasDOFs() && (*nit)->setExternal(false))
        ListUI <<"\n  ** Switching off the external status for unused node "
               << (*nit)->getID();
}


bool FFlSesamReader::parse (const std::string& filename)
{
  std::ifstream is(filename.c_str());
  if (!is)
  {
    ListUI <<"\n *** Error: Can not open FE data file "<< filename <<"\n";
    return false;
  }

#ifdef FFL_TIMER
  myProfiler->startTimer("parse");
#endif

  char cline[80];
  char* ctmp = NULL;
  RecordMap::iterator fit = myRecs.end();
  for (int iline = 1; is.getline(cline,80); iline++)
    if (strlen(cline) < 1)
      ListUI <<"\n  ** Warning: Blank line "<< iline <<" (ignored).";
    else if (!isspace(cline[0]))
    {
      // This is a new record
      if ((ctmp = strtok(cline," ")))
        fit = myRecs.insert(std::make_pair(ctmp,Records())).first;
      else
        continue; // should never get here, but...

      std::vector<double> fields;
      for (int i = 0; i < 4 && (ctmp = strtok(NULL," ")); i++)
	fields.push_back(atof(ctmp));
      fit->second.push_back(fields);
    }
    else if (fit != myRecs.end())
    {
      if (isspace(cline[8]))
      {
	// This is a continuation of current record
	std::vector<double>& fields = fit->second.back().fields;
	if ((ctmp = strtok(cline," ")))
	  fields.push_back(atof(ctmp));
	for (int i = 1; i < 4 && (ctmp = strtok(NULL," ")); i++)
	  fields.push_back(atof(ctmp));
      }
      else
      {
	// This is a text line (the first 8 chars are assumed blank)
	size_t nchar = strlen(cline);
	while (nchar > 8 && isspace(cline[nchar-1])) --nchar;
	if (nchar > 8)
	  fit->second.back().text.push_back(std::string(cline+8,nchar-8));
      }
    }

#ifdef FFL_DEBUG
  for (fit = myRecs.begin(); fit != myRecs.end(); ++fit)
#if FFL_DEBUG > 3
    for (const Record& record : fit->second)
    {
      std::cout << fit->first;
      for (double field : record.fields) std::cout <<" "<< field;
      for (const std::string& txt : record.text) std::cout <<"\n\t"<< txt;
      std::cout << std::endl;
    }
#else
    std::cout << fit->second.size() <<'\t'<< fit->first << std::endl;
#endif
#endif

#ifdef FFL_TIMER
  myProfiler->stopTimer("parse");
#endif

  return true;
}


bool FFlSesamReader::read (const std::string& filename)
{
  myRecs.clear();
  if (!this->parse(filename))
    return false;

#ifdef FFL_TIMER
  myProfiler->startTimer("read");
#endif

  // List here entries that are OK, but should not be traversed
  const char* silent[8] = { "DATE", "IDENT", "IEND", "GNODE", "GECCEN",
			    "TDMATER", "TDSECT", "TDSETNAM" };
  const char** silentEnd = silent + 8;

  // Output the date information in this file to the user
  RecordMap::const_iterator fit = myRecs.find("DATE");
  if (fit != myRecs.end() && !fit->second.empty())
  {
    for (const Record& record : fit->second)
      for (const std::string& text : record.text)
        ListUI <<"\n     "<< text;
    ListUI <<"\n";
  }

  Records dummy;
  // Check if we have any eccentricities in this file
  fit = myRecs.find("GECCEN");
  const Records& eccs = fit == myRecs.end() ? dummy : fit->second;
  // Check if we have any direction vectors in this file
  fit = myRecs.find("GUNIVEC");
  const Records& univ = fit == myRecs.end() ? dummy : fit->second;
  // Check if we have any material names in this file
  fit = myRecs.find("TDMATER");
  const Records& mnames = fit == myRecs.end() ? dummy : fit->second;
  // Check if we have any cross section names in this file
  fit = myRecs.find("TDSECT");
  const Records& xnames = fit == myRecs.end() ? dummy : fit->second;
  // Check if we have any set names in this file
  fit = myRecs.find("TDSETNAM");
  const Records& snames = fit == myRecs.end() ? dummy : fit->second;

  // Process the supported data records
  bool ok = true;
  for (fit = myRecs.begin(); fit != myRecs.end() && ok; ++fit)

    if (fit->first == "BELFIX")
      ok = this->readHinges(fit->second);

    else if (fit->first == "GBEAMG")
      ok = this->readBeamSections(fit->second,xnames);

    else if (fit->first == "GCOORD")
      ok = this->readNodes(fit->second);

    else if (fit->first == "GELMNT1")
      ok = this->readElements(fit->second);

    else if (fit->first == "GELREF1")
      ok = this->readElementRefs(fit->second,eccs,univ);

    else if (fit->first == "GELTH")
      ok = this->readThicknesses(fit->second,xnames);

    else if (fit->first == "GSETMEMB")
      ok = this->readGroups(fit->second,snames);

    else if (fit->first == "GUNIVEC")
      ok = this->readUnitVectors(fit->second);

    else if (fit->first == "MGSPRNG")
      ok = this->readGroundSprings(fit->second);

    else if (fit->first == "MISOSEL")
      ok = this->readMaterials(fit->second,mnames);

    else if (fit->first == "BNBCD")
      continue; // Must be processed after the other elements
    else if (fit->first == "BNMASS")
      continue; // Must be processed after the other elements
    else if (fit->first == "BLDEP")
      continue; // Must be processed after the other elements

    else if (std::find(silent,silentEnd,fit->first) == silentEnd)
      ListUI <<"\n  ** Ignoring "<< (int)fit->second.size()
	     <<" unsupported "<< fit->first <<" entries.";

  fit = myRecs.find("BNBCD");
  if (fit != myRecs.end() && ok)
    ok = this->readBCs(fit->second);

  fit = myRecs.find("BNMASS");
  if (fit != myRecs.end() && ok)
    ok = this->readMasses(fit->second);

  fit = myRecs.find("BLDEP");
  if (fit != myRecs.end() && ok)
    ok = this->readLinearDependencies(fit->second);

#ifdef FFL_TIMER
  myProfiler->stopTimer("read");
#endif

  return ok;
}


bool FFlSesamReader::readNodes (const Records& recs)
{
  for (const Record& record : recs)
  {
    const std::vector<double>& fields = record.fields;

    int node_id = (int)fields.front();
#if FFL_DEBUG > 2
    std::cout <<"Reading node "<< node_id <<" "<< fields[1]
	      <<" "<< fields[2] <<" "<< fields[3] << std::endl;
#endif
    FaVec3 X(fields[1],fields[2],fields[3]);
    FFlNode* aNode = new FFlNode(node_id,X.truncate());
    if (!myLink->addNode(aNode))
      return false;
  }

  return true;
}


bool FFlSesamReader::readBCs (const Records& recs)
{
  myLinearDepDofs.clear();

  for (const Record& record : recs)
  {
    const std::vector<double>& fields = record.fields;

    int status = 0, digit = 1;
    int nodeno = (int)fields[0];
    int ndof   = (int)fields[1];

#if FFL_DEBUG > 1
    std::cout <<"Reading nodal BCs "<< nodeno <<" "<< ndof <<": ";
    for (int i = 0; i < ndof; i++)
      std::cout <<" "<< fields[2+i];
#endif

    FFlNode* node = myLink->getNode(nodeno);
    if (node)
    {
      for (int i = 0; i < ndof; i++, digit *= 2)
      {
        int ifix = (int)fields[2+i];
        if (ifix == 4 && status >= 0)
          status = 1;
        else if (ifix == 1 && status <= 0)
          status -= digit;
        else if (ifix == 3 && status == 0)
          myLinearDepDofs[10*nodeno+i+1] = 0;
        else if (ifix != 0 || status == 1)
          ListUI <<"\n  ** Ignoring FIX"<< i+1 <<"="<< ifix
                 <<" in BNBCD record for node "<< nodeno;
      }
      node->setStatus(status);
    }
    else
      ListUI <<"\n  ** Non-existing node ID "<< nodeno
             <<" in BNBCD record (ignored).";
#if FFL_DEBUG > 1
    std::cout <<" ==> "<< status << std::endl;
#endif
  }

  return true;
}


bool FFlSesamReader::readMasses (const Records& recs)
{
  for (const Record& record : recs)
  {
    const std::vector<double>& fields = record.fields;

    int i, j, k;
    int nodeno = (int)fields[0];
    int ndof   = (int)fields[1];
    for (i = 1+ndof; i > 4; i--)
      if (fields[i] == 0.0)
        ndof = i-2;
    if (ndof > 3) ndof = 6;

#if FFL_DEBUG > 1
    std::cout <<"Reading nodal mass "<< nodeno <<" "<< ndof <<":";
    for (i = 0; i < ndof; i++)
      std::cout <<" "<< fields[2+i];
    std::cout << std::endl;
#endif

    int eID = myLink->getNewElmID();
    FFlElementBase* newElem = ElementFactory::instance()->create("CMASS",eID);
    newElem->setNodes(std::vector<int>(1,nodeno));
    if (!myLink->addElement(newElem))
      return false;

    std::vector<double> MassMat;
    for (j = 0, k = 2; j < ndof; j++, k++)
    {
      for (i = 0; i < j; i++)
        MassMat.push_back(0.0);
      MassMat.push_back(fields[k]);
    }

    FFlPMASS* mass = CREATE_ATTRIBUTE(FFlPMASS,"PMASS",eID);
    mass->M = MassMat;

    newElem->setAttribute(mass);
    if (!myLink->addAttribute(mass))
      return false;
  }

  return true;
}


bool FFlSesamReader::readLinearDependencies (const Records& recs)
{
  struct DepDOF
  {
    int node, lDof; double coeff;
    DepDOF(int n, int d, double c) : node(n), lDof(d), coeff(c) {}
  };
  typedef std::vector<DepDOF>         DepDOFs;
  typedef std::map<short int,DepDOFs> MPC;
  typedef std::map<int,MPC>           MPCMap;

  typedef std::vector<double>      DoubleVec;
  typedef std::map<int,DoubleVec>  DoublesMap;
  typedef std::map<int,DoublesMap> DoublesMapMap;

  LSintMap::iterator            dit;
  MPCMap::const_iterator        mit;
  MPC::const_iterator           sdof;
  DoublesMapMap::const_iterator it;
  DoublesMap::const_iterator    jt;

  // Build a temporary MPC container of all linear dependency elements
  MPCMap myMPCs;
  for (const Record& record : recs)
  {
    const DoubleVec& fields = record.fields;

    int nodenum = (int)fields[0];
    int cnod    = (int)fields[1];
    int ndep    = (int)fields[3];
#if FFL_DEBUG > 1
    std::cout <<"Reading nodal linear dependency "
              << nodenum <<" "<< cnod <<" "<< ndep <<":\n";
#endif
    for (int i = 0; i < ndep; i++)
    {
      int sDof = (int)fields[4*i+4];
      int mDof = (int)fields[4*i+5];
      double c =      fields[4*i+6];
#if FFL_DEBUG > 1
      std::cout <<'\t'<< sDof <<" "<< mDof <<" "<< c << std::endl;
#endif
      if ((dit = myLinearDepDofs.find(10*nodenum+sDof)) == myLinearDepDofs.end())
        ListUI <<"\n  ** DOF "<< sDof <<" in node "<< nodenum
               <<" is not defined as linear dependent (ignored).";
      else
      {
        dit->second ++; // Flag as slave DOF
        myMPCs[nodenum][sDof].push_back(DepDOF(cnod,mDof,c));
      }
    }
  }

  // Verify that all linear dependent DOFs has been referred
  for (dit = myLinearDepDofs.begin(); dit != myLinearDepDofs.end(); ++dit)
    if (dit->second == 0)
      ListUI <<"\n  ** Warning: DOF "<< (int)(dit->first)%10 <<" in node "
             << (int)(dit->first)/10 <<" was tagged as linear dependent"
             <<" but not referred in a linear dependency element.";

  // Create WAVGM elements for the multi-point constraints
  for (mit = myMPCs.begin(); mit != myMPCs.end(); ++mit)
  {
    // Find the element nodes
    std::vector<int> nodes(1,mit->first);
    for (sdof = mit->second.begin(); sdof != mit->second.end(); ++sdof)
      for (const DepDOF& dof : sdof->second)
      {
        size_t iNod = 1;
        while (iNod < nodes.size() && dof.node != nodes[iNod]) iNod++;
        if (iNod == nodes.size()) nodes.push_back(dof.node);
      }
#if FFL_DEBUG > 1
    std::cout <<"Element nodes:";
    for (int n : nodes) std::cout <<" "<< n;
    std::cout << std::endl;
#endif

    size_t nRow = 0;
    DoublesMapMap dofWeights;
    for (sdof = mit->second.begin(); sdof != mit->second.end(); ++sdof)
      if (sdof->first > 0 && sdof->first < 7)
      {
        // Find the weight matrix associated with this slave DOF
        DoublesMap& dofWeight = dofWeights[sdof->first];
        for (const DepDOF& dof : sdof->second)
        {
          DoubleVec& weights = dofWeight[dof.lDof];
          weights.resize(nodes.size()-1,0.0);
          for (size_t iNod = 1; iNod < nodes.size(); iNod++)
            if (dof.node == nodes[iNod])
            {
              weights[iNod-1] = dof.coeff;
              break;
            }
        }
        nRow += 6; // Assuming all DOFs in the master node is referred
#if FFL_DEBUG > 1
        std::cout <<"Weight matrix associated with slave dof "<< sdof->first;
        for (jt = dofWeight.begin(); jt != dofWeight.end(); ++jt)
        {
          std::cout <<"\n\t"<< jt->first <<":";
          for (double c : jt->second) std::cout <<" "<< c;
        }
	std::cout << std::endl;
#endif
      }

    int refC = 0;
    size_t indx = 1;
    size_t nMst = nodes.size() - 1;
    int indC[6] = { 0, 0, 0, 0, 0, 0 };
    DoubleVec weights(nRow*nMst,0.0);
    for (it = dofWeights.begin(); it != dofWeights.end(); ++it, indx += 6*nMst)
    {
      refC = 10*refC + it->first; // Compressed slave DOFs identifier
      indC[it->first-1] = indx;   // Index to first weight for this slave DOF
      for (jt = it->second.begin(); jt != it->second.end(); ++jt)
        for (size_t j = 0; j < jt->second.size(); j++)
          weights[indx+6*j+jt->first-2] = jt->second[j];
    }

    int eID = myLink->getNewElmID();
    FFlElementBase* newElem = ElementFactory::instance()->create("WAVGM",eID);
    if (!newElem)
    {
      ListUI <<"\n *** Error: Failure creating WAVGM element "<< eID <<".\n";
      return false;
    }

    newElem->setNodes(nodes);
    if (!myLink->addElement(newElem))
      return false;

    FFlPWAVGM* newAtt = CREATE_ATTRIBUTE(FFlPWAVGM,"PWAVGM",eID);
    newAtt->refC = -refC; // Hack: Negative refC means explicit constraints
    newAtt->weightMatrix.data().swap(weights);
    for (int j = 0; j < 6; j++)
      newAtt->indC[j] = indC[j];

    if (myLink->addAttribute(newAtt))
      newElem->setAttribute(newAtt);
    else
      return false;
  }

  return true;
}


bool FFlSesamReader::readElements (const Records& recs)
{
  IntMap unsup, toLine;
  IntMap::iterator uit;

  const char* BEAM3 = FFlReaders::convertToLinear == 1 ? "BEAM2" : "BEAM3";
  const char* TRI6  = FFlReaders::convertToLinear == 1 ? "TRI3"  : "TRI6";
  const char* QUAD8 = FFlReaders::convertToLinear == 1 ? "QUAD4" : "QUAD8";

  for (const Record& record : recs)
  {
    const std::vector<double>& fields = record.fields;

    int elm_id = (int)fields[1];
    int elmtyp = (int)fields[2];
    std::vector<int> nodes;
    if (fields.size() > 4)
      nodes.reserve(fields.size()-4);
    for (size_t i = 4; i < fields.size(); i++)
      nodes.push_back((int)fields[i]);
#if FFL_DEBUG > 2
    std::cout <<"Reading element "<< elm_id <<" "<< elmtyp;
    for (int n : nodes) std::cout <<" "<< n;
    std::cout << std::endl;
#endif

    unsigned short int nodeInc = 1;
    FFlElementBase* newElem = NULL;
    switch (elmtyp)
      {
      case  2: // BEPS - 2-node 2D beam
      case 15: // BEAS - 2-node 3D beam
	newElem = ElementFactory::instance()->create("BEAM2", elm_id);
	break;
      case 22: // SECB - 3-node subparametric curved beam
        newElem = ElementFactory::instance()->create("BEAM3", elm_id);
        break;
      case 23: // BTSS - 3-node subparametric general curved beam
        newElem = ElementFactory::instance()->create(BEAM3, elm_id);
        if (FFlReaders::convertToLinear == 1) nodeInc = 2;
        break;
      case  3: // CSTA - plane constant strain triangle
      case 25: // FTRS - flat triangular shell
	newElem = ElementFactory::instance()->create("TRI3", elm_id);
	break;
      case  9: // LQUA - plane quadrilateral membrane
      case 24: // FQUS - flat quadrilateral thin shell
	newElem = ElementFactory::instance()->create("QUAD4", elm_id);
	break;
      case  6: // ILST - plane linear strain triangle
        newElem = ElementFactory::instance()->create("TRI6", elm_id);
        break;
      case 26: // SCTS - subparametric curved triangular shell
        newElem = ElementFactory::instance()->create(TRI6, elm_id);
        if (FFlReaders::convertToLinear != 1)
        {
          std::swap(nodes[1],nodes[2]);
          std::swap(nodes[1],nodes[3]);
          std::swap(nodes[3],nodes[4]);
        }
        break;
      case  8: // IQQE - plane quadrilateral serendipity membrane
        newElem = ElementFactory::instance()->create("QUAD8", elm_id);
        break;
      case 28: // SCQS - subparametric curved quadrilateral shell
        newElem = ElementFactory::instance()->create(QUAD8, elm_id);
        if (FFlReaders::convertToLinear == 1) nodeInc = 2;
        break;
      case 31: // ITET - isoparametric tetrahedron
	newElem = ElementFactory::instance()->create("TET10", elm_id);
	break;
      case 30: // IPRI - isoparametric prism
	newElem = ElementFactory::instance()->create("WEDG15", elm_id);
	break;
      case 20: // IHEX - isoparametric hexahedron
	newElem = ElementFactory::instance()->create("HEX20", elm_id);
	break;
      case 33: // TETR - linear hexahedron
	newElem = ElementFactory::instance()->create("TET4", elm_id);
	break;
      case 32: // TPRI - triangular prism
	newElem = ElementFactory::instance()->create("WEDG6", elm_id);
	break;
      case 21: // LHEX - linear hexahedron
	newElem = ElementFactory::instance()->create("HEX8", elm_id);
	break;
      case 18: // GSPR - ground spring
	newElem = ElementFactory::instance()->create("RSPRING", elm_id);
	break;
      case 11: // GMAS - 1-noded mass matrix
	newElem = ElementFactory::instance()->create("CMASS", elm_id);
	break;
      default:
	if ((uit = unsup.find(elmtyp)) == unsup.end())
	  unsup[elmtyp] = 1;
	else
	  uit->second++;
    }

    if (newElem)
    {
      size_t nelnod = newElem->getFEElementTopSpec()->getNodeCount();
      std::vector<int>::const_iterator nit = nodes.begin();
      for (size_t n = 1; n <= nelnod && nit != nodes.end(); n++, nit += nodeInc)
        newElem->setNode(n,*nit);
      if (!myLink->addElement(newElem))
        return false;
      if (nelnod < nodes.size())
      {
        if ((uit = toLine.find(elmtyp)) == toLine.end())
          toLine[elmtyp] = 1;
        else
          uit->second++;
      }
    }
  }

  for (const std::pair<const int,int>& elt : unsup)
    ListUI <<"\n  ** Warning: Ignoring "<< elt.second
           <<" elements of unsupported type "<< elt.first;
  for (const std::pair<const int,int>& elt : toLine)
    ListUI <<"\n  ** Warning: Converting "<< elt.second
           <<" parabolic elements of type "<< elt.first
           <<" to linear elements.";

  return true;
}


bool FFlSesamReader::readElementRefs (const Records& recs,
                                      const Records& eccs, const Records& univ)
{
  // Lambda function for extracting a referenced vector from a set of records
  auto&& getVector = [](FaVec3& v, const Records& records, int iref) -> bool
  {
    for (const Record& rec : records)
      if ((int)rec.fields.front() == iref)
      {
        for (short int i = 1; i <= 3; i++)
          v(i) = rec.fields[i];
        return true;
      }
    return false;
  };

  // Find the highest element ID
  int newID = 0;
  for (const Record& rec : recs)
  {
    int elmno = (int)rec.fields.front();
    if (elmno > newID) newID = elmno;
  }

  // Collect set of unit vector ID
  std::set<int> univecID, usedUvecs;
  for (const Record& rec : univ)
    univecID.insert(rec.fields.front());

  int unSupported = 0;
  for (const Record& record : recs)
  {
    const std::vector<double>& fields = record.fields;

    int elmno = (int)fields.front();
    FFlElementBase* elm = myLink->getElement(elmno);
    if (!elm)
    {
      ListUI <<"\n *** Error: Non-existing element "<< elmno <<" referred.\n";
      return false;
    }

    int matno = (int)fields[1];
    int geono = (int)fields[8];
    int fixno = (int)fields[9];
    int eccno = (int)fields[10];
    int trano = (int)fields[11];

    // Find number of element nodes from the number of fields.
    // Have to do this way in case of conversion to linear elements.
    int j = 12, nno = 0;
    if (geono < 0) nno++;
    if (fixno < 0) nno++;
    if (eccno < 0) nno++;
    if (trano < 0) nno++;
    int nelnod = nno > 0 ? (fields.size()-12)/nno : 0;

    if (matno > 0)
    {
      if (elm->getTypeName() == "RSPRING")
        elm->setAttribute("PSPRING",matno);
      else
        elm->setAttribute("PMAT",matno);
    }

    if (geono < 0)
    {
      geono = (int)fields[j];
      for (int i = 1; i < nelnod; i++)
        if ((int)fields[i+j] != geono)
        {
          ListUI <<"\n  ** Warning: Element "<< elmno
                 <<" has non-constant geometry properties,"
                 <<" using that of element node 1 only.";
          break;
        }
      j += nelnod;
    }
    if (geono > 0)
      switch (elm->getCathegory()) {
      case FFlTypeInfoSpec::BEAM_ELM:
	elm->setAttribute("PBEAMSECTION",geono);
	break;
      case FFlTypeInfoSpec::SHELL_ELM:
	elm->setAttribute("PTHICK",geono);
	break;
      default:
	break;
      }

    if (fixno && elm->getCathegory() == FFlTypeInfoSpec::BEAM_ELM)
    {
      int fixn2 = fixno;
      if (fixno < 0)
      {
        fixno = (int)fields[j];
        fixn2 = (int)fields[j+nelnod-1];
        if (nelnod > 2 && fields[j+1] > 0.0)
        {
          ListUI <<"\n  ** Warning: Parabolic beam element "<< elmno
                 <<" refers to fixation record BELFIX "<< fields[j+1]
                 <<" at its center node";
          unSupported++;
          fixno = fixn2 = -3;
        }
        j += nelnod;
      }

      IntMap::const_iterator p1 = myHinges.find(fixno);
      IntMap::const_iterator p2 = myHinges.find(fixn2);
      if (fixno > 0 && p1 == myHinges.end())
      {
        ListUI <<"\n  ** Warning: Beam element "<< elmno
               <<" refers to undefined fixation record BELFIX "<< fixno;
        unSupported++;
      }
      else if (fixn2 > 0 && p2 == myHinges.end())
      {
        ListUI <<"\n  ** Warning: Beam element "<< elmno
               <<" refers to undefined fixation record BELFIX "<< fixn2;
        unSupported++;
      }
      else if (fixno > 0 || fixn2 > 0)
      {
        if (fixno > 0 && fixn2 > 0)
          fixno = fixno*myHinges.rbegin()->second + fixn2;
        else if (fixno == 0)
          fixno = fixn2;
        elm->setAttribute("PBEAMPIN",fixno);
        if (!myLink->getAttribute("PBEAMPIN",fixno))
        {
          FFlPBEAMPIN* pin = CREATE_ATTRIBUTE(FFlPBEAMPIN,"PBEAMPIN",fixno);
          if (p1 != myHinges.end()) pin->PA = p1->second;
          if (p2 != myHinges.end()) pin->PB = p2->second;
          if (!myLink->addAttribute(pin))
            return false;
        }
      }
    }
    else if (fixno)
    {
      ListUI <<"\n  ** Warning: Element "<< elmno
             <<" refers to fixation record(s) BELFIX";
      if (fixno > 0)
        ListUI <<" "<< fixno;
      else for (int i = 0; i < nelnod; i++)
        ListUI <<" "<< fields[j++];
      unSupported++;
    }

    if (eccno && elm->getCathegory() == FFlTypeInfoSpec::BEAM_ELM)
    {
      std::vector<int> missing;
      std::array<FaVec3,3> E;
      if (eccno > 0)
      {
        if (!getVector(E[0],eccs,eccno))
          missing.push_back(eccno);
      }
      else
      {
        for (int k = 0; k < nelnod; j++, k++)
          if (k < 3 && fields[j] > 0.0)
            if (!getVector(E[k],eccs,(int)fields[j]))
              missing.push_back((int)fields[j]);
      }
      if (!missing.empty())
      {
        ListUI <<"\n *** Error: Missing GECCEN record(s)";
        for (int iecc : missing) ListUI <<" "<< iecc;
        ListUI <<" referred by element "<< elmno <<"\n";
        return false;
      }

      const double eps = 1.0e-8;
      FFlPBEAMECCENT* ecc = CREATE_ATTRIBUTE(FFlPBEAMECCENT,"PBEAMECCENT",elmno);
      ecc->node1Offset = E[0].truncate(eps);
      if (eccno > 0)
        ecc->node2Offset = E[0];
      else if (nelnod == 2 || elm->getNodeCount() > 2)
        ecc->node2Offset = E[1].truncate(eps);
      else
        ecc->node2Offset = E[2].truncate(eps);
      if (elm->getNodeCount() > 2)
      {
        ecc->resize(9); // 3 vectors a 3 values
        ecc->node3Offset = eccno > 0 ? E[0] : E[2].truncate(eps);
      }
      if (!elm->setAttribute(ecc) || !myLink->addAttribute(ecc))
        return false;
    }
    else if (eccno)
    {
      ListUI <<"\n  ** Warning: Element "<< elmno
             <<" refers to eccentricity record(s) GECCEN";
      if (eccno > 0)
        ListUI <<" "<< eccno;
      else for (int i = 0; i < nelnod; i++)
        ListUI <<" "<< fields[j++];
      unSupported++;
    }

    if (trano < 0)
    {
      trano = (int)fields[j];
      for (int i = 1; i < nelnod; i++)
        if ((int)fields[i+j] != trano)
        {
          trano = -1;
          if (elm->getCathegory() == FFlTypeInfoSpec::BEAM_ELM && nelnod == 3)
          {
            // Parabolic beam element with non-uniform direction vector
            ListUI <<"\n   * Note: Parabolic beam element "<< elmno
                   <<" has a varying Z-axis direction vector: ";
            FaVec3 Uvec;
            const double eps = 1.0e-9;
            FFlPORIENT3* ori = CREATE_ATTRIBUTE(FFlPORIENT3,"PORIENT3",elmno);
            std::vector<int> missing;
            for (int k = 0; k < nelnod; k++)
            {
              int itran = (int)fields[j+k];
              ListUI <<" "<< itran;
              if (getVector(Uvec,univ,itran))
                ori->directionVector[k].setValue(Uvec.normalize(eps).round(10));
              else
                missing.push_back(itran);
            }
            if (!missing.empty())
            {
              ListUI <<"\n *** Error: Missing GUNIVEC record(s)";
              for (int itran : missing) ListUI <<" "<< itran;
              ListUI <<" referred by element "<< elmno <<"\n";
              delete ori;
              return false;
            }
            FFlAttributeBase* newAtt = ori;
            if (elm->getNodeCount() == 2)
            {
              if (univecID.find(elmno) != univecID.end())
                elmno = ++newID; // Find a unused ID value
              // Use mean direction vector when replaced by two-noded beam
              FFlPORIENT* or2 = CREATE_ATTRIBUTE(FFlPORIENT,"PORIENT",elmno);
              Uvec = ori->directionVector[0].getValue() + ori->directionVector[2].getValue();
              or2->directionVector.setValue(Uvec.normalize(eps).round(10));
              newAtt = or2;
              delete ori;
            }
            if (!elm->setAttribute(newAtt) || !myLink->addAttribute(newAtt))
              return false;
          }
          else
            ListUI <<"\n  ** Warning: Element "<< elmno
                   <<" has non-constant transformation properties,"
                   <<" using that of element node 1 only.";
          break;
        }
    }
    if (trano > 0)
    {
      elm->setAttribute("PORIENT",trano);
      usedUvecs.insert(trano);
    }
  }

  if (usedUvecs.size() < univecID.size())
  {
    // Remove the unused GUNIVEC records
    Records& gunivecs = const_cast<Records&>(univ);
    for (Records::iterator it = gunivecs.begin(); it != gunivecs.end();)
      if (usedUvecs.find((int)it->fields.front()) == usedUvecs.end())
        it = gunivecs.erase(it);
      else
        ++it;
  }

  if (unSupported > 0)
    ListUI <<"\n  ** A total of "<< unSupported
           <<" unsupported element data references was detected (ignored).\n";

  return true;
}


bool FFlSesamReader::readHinges (const Records& recs)
{
  for (const Record& record : recs)
  {
    const std::vector<double>& fields = record.fields;

    int fixno = (int)fields[0];
    int opt   = (int)fields[1];
    int trano = (int)fields[2];
#if FFL_DEBUG > 1
    std::cout <<"Reading hinge "<< fixno <<" "<< opt <<" "<< trano << std::endl;
#endif
    if (opt != 1 || trano != 0)
    {
      ListUI <<"\n  ** Warning: Only OPT=1 and TRANO=0 is supported."
             <<"\n              BELFIX "<< fixno <<" "<< opt <<" "<< trano
             <<" is ignored.";
      continue;
    }

    int pinflag = 0;
    for (size_t i = 4; i < 10 && i < fields.size(); i++)
    {
      double a = fields[i];
      int ifix = round(a);
      if (fabs(a) > 1.0e-12 && fabs(a-1.0) > 1.0e-12)
        ListUI <<"\n  ** Warning: Fixation degree "<< a
               <<" is rounded to "<< ifix
               <<". Only values 0 and 1 is supported.";
      if (ifix < 1)
        pinflag = 10*pinflag + i-3;
    }

    if (pinflag > 0)
      myHinges[fixno] = pinflag;
  }

  return true;
}


bool FFlSesamReader::readBeamSections (const Records& recs,
                                       const Records& names)
{
  for (const Record& record : recs)
  {
    const std::vector<double>& fields = record.fields;

    int geono = (int)fields.front();
#if FFL_DEBUG > 1
    std::cout <<"Reading beam section "<< geono <<" ";
#endif

    FFlPBEAMSECTION* newAttribute = CREATE_ATTRIBUTE(FFlPBEAMSECTION,"PBEAMSECTION",geono);

    const std::string& name = findName(names,geono);
    if (!name.empty())
    {
#if FFL_DEBUG > 1
      std::cout <<"\""<< name <<"\" ";
#endif
      newAttribute->setName(name);
    }

    double A = fields[2];
#if FFL_DEBUG > 1
    std::cout << A
	      <<" "<< fields[3]  <<" "<< fields[4] <<" "<< fields[5]
	      <<" "<< fields[10] <<" "<< fields[11]
	      <<" "<< fields[12] <<" "<< fields[13] << std::endl;
#endif

    newAttribute->crossSectionArea = A;
    newAttribute->It = fields[3];
    newAttribute->Iz = fields[4]; // Iyy = \int{z^2}dA
    newAttribute->Iy = fields[5]; // Izz = \int{y^2}dA
    newAttribute->Kxy = round(fields[10]/A,10); // use 10 significant digits
    newAttribute->Kxz = round(fields[11]/A,10); // use 10 significant digits
    newAttribute->Sy = fields[12];
    newAttribute->Sz = fields[13];

    if (!myLink->addAttribute(newAttribute))
      return false;
  }

  return true;
}


bool FFlSesamReader::readThicknesses (const Records& recs,
                                      const Records& names)
{
  for (const Record& record : recs)
  {
    const std::vector<double>& fields = record.fields;

    int geono = (int)fields.front();
#if FFL_DEBUG > 1
    std::cout <<"Reading shell thickness "<< geono <<" ";
#endif

    FFlPTHICK* newAttribute = CREATE_ATTRIBUTE(FFlPTHICK,"PTHICK",geono);

    const std::string& name = findName(names,geono);
    if (!name.empty())
    {
#if FFL_DEBUG > 1
      std::cout <<"\""<< name <<"\" ";
#endif
      newAttribute->setName(name);
    }

#if FFL_DEBUG > 1
    std::cout << fields[1] << std::endl;
#endif

    newAttribute->thickness = fields[1];

    if (!myLink->addAttribute(newAttribute))
      return false;
  }

  return true;
}


bool FFlSesamReader::readMaterials (const Records& recs,
                                    const Records& names)
{
  for (const Record& record : recs)
  {
    const std::vector<double>& fields = record.fields;

    int matno = (int)fields.front();
#if FFL_DEBUG > 1
    std::cout <<"Reading material "<< matno <<" ";
#endif

    FFlPMAT* newAttribute = CREATE_ATTRIBUTE(FFlPMAT,"PMAT",matno);

    const std::string& name = findName(names,matno);
    if (!name.empty())
    {
#if FFL_DEBUG > 1
      std::cout <<"\""<< name <<"\" ";
#endif
      newAttribute->setName(name);
    }

    double E = fields[1];
    double nu = fields[2];
    double rho = fields[3];
    double G = E/(2.0+nu+nu);
#if FFL_DEBUG > 1
    std::cout << E <<" "<< G <<" "<< nu <<" "<< rho << std::endl;
#endif

    newAttribute->youngsModule    = E;
    newAttribute->shearModule     = round(G,10); // use 10 significant digits
    newAttribute->poissonsRatio   = nu;
    newAttribute->materialDensity = rho;

    if (!myLink->addAttribute(newAttribute))
      return false;
  }

  return true;
}


bool FFlSesamReader::readGroundSprings (const Records& recs)
{
  for (const Record& record : recs)
  {
    const std::vector<double>& fields = record.fields;

    int matno = (int)fields[0];
    int ndof  = (int)fields[1];
#if FFL_DEBUG > 1
    std::cout <<"Reading ground spring "<< matno <<" "<< ndof <<": ";
#endif

    FFlPSPRING* newAttribute = CREATE_ATTRIBUTE(FFlPSPRING,"PSPRING",matno);

    // Raad column-wise
    double K[6][6];
    int i, j, k = 2;
    for (j = 0; j < ndof; j++)
      for (i = j; i < ndof; i++, k++)
      {
#if FFL_DEBUG > 1
        std::cout <<" "<< fields[k];
#endif
        if (i < 6 && j < 6)
          K[i][j] = fields[k];
      }
#if FFL_DEBUG > 1
    std::cout << std::endl;
#endif

    // Store row-wise
    for (i = k = 0; i < 6; i++)
      for (j = 0; j <= i; j++)
        newAttribute->K[k++] = i < ndof && j < ndof ? K[i][j] : 0.0;

    if (!myLink->addAttribute(newAttribute))
      return false;
  }

  return true;
}


bool FFlSesamReader::readUnitVectors (const Records& recs)
{
  for (const Record& record : recs)
  {
    const std::vector<double>& fields = record.fields;

    int transno = (int)fields.front();
    FaVec3 uVec(fields[1],fields[2],fields[3]);
#if FFL_DEBUG > 1
    std::cout <<"Reading unit vector "<< transno <<" "<< uVec << std::endl;
#endif

    FFlPORIENT* newAtt = CREATE_ATTRIBUTE(FFlPORIENT,"PORIENT",transno);
    newAtt->directionVector.setValue(uVec.normalize(1.0e-9).round(10));

    if (!myLink->addAttribute(newAtt))
      return false;
  }

  return true;
}


bool FFlSesamReader::readGroups (const Records& recs,
                                 const Records& names)
{
  for (const Record& record : recs)
  {
    if (record.fields[3] != 2.0) continue; // Ignore node sets
    const std::vector<double>& fields = record.fields;

    int nfield = (int)fields[0];
    int isref  = (int)fields[1];
    FFlGroup* group = myLink->getGroup(isref);
    if (!group)
    {
      const std::string& name = findName(names,isref);
      if (!name.empty())
      {
#if FFL_DEBUG > 1
	std::cout <<"Reading new element group "<< isref <<": "
		  << nfield <<" \""<< name <<"\""<< std::endl;
#endif
	group = new FFlGroup(isref,name);
      }
      else
      {
#if FFL_DEBUG > 1
	std::cout <<"Reading new unnamed element group "<< isref <<": "
		  << nfield << std::endl;
#endif
	group = new FFlGroup(isref);
      }
      if (!myLink->addGroup(group))
	group = NULL;
    }
#if FFL_DEBUG > 1
    else
      std::cout <<"Reading element group "<< isref <<": "<< nfield << std::endl;
#endif

    if (group)
      for (int i = 5; i < nfield && (size_t)i < fields.size(); i++)
        group->addElement((int)fields[i]);
  }

  return true;
}


const std::string& FFlSesamReader::findName (const Records& names, int ID)
{
  Records::const_iterator nit = std::find_if(names.begin(),names.end(),
                                             [ID](const Record& rec)
                                             {
                                               return rec.fields[1] == ID;
                                             });
  if (nit != names.end()) return nit->text.front();

  static std::string empty;
  return empty;
}
