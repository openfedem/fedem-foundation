// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#include <cstdarg>
#include <cstring>

#include "FFlOldFLMReader.H"
#include "FFlLib/FFlLinkHandler.H"
#include "FFlLib/FFlIOAdaptors/FFlReaders.H"
#include "FFlLib/FFlFEParts/FFlNode.H"
#include "FFlLib/FFlElementBase.H"
#include "FFlLib/FFlFEParts/FFlPMAT.H"
#include "FFlLib/FFlFEParts/FFlPTHICK.H"
#include "FFlLib/FFlFEParts/FFlPORIENT.H"
#include "FFlLib/FFlFEParts/FFlPBEAMECCENT.H"
#include "FFlLib/FFlFEParts/FFlPBEAMSECTION.H"

#include "FFaLib/FFaDefinitions/FFaMsg.H"
#ifdef FFL_TIMER
#include "FFaLib/FFaProfiler/FFaProfiler.H"
#endif
#include "Admin/FedemAdmin.H"

#ifndef LINE_LENGTH
#define LINE_LENGTH 128
#endif

#define CREATE_ATTRIBUTE(type,name,id) \
  dynamic_cast<type*>(AttributeFactory::instance()->create(name,id));


FFlOldFLMReader::FFlOldFLMReader(FFlLinkHandler* aLink) : FFlReaderBase(aLink)
{
#ifdef FFL_TIMER
  myProfiler = new FFaProfiler("OldFLMReader profiler");
#endif
}

FFlOldFLMReader::~FFlOldFLMReader()
{
#ifdef FFL_TIMER
  myProfiler->report();
  delete myProfiler;
#endif
}


void FFlOldFLMReader::init()
{
  FFlReaders::instance()->registerReader("Fedem Link Model","flm",
					 FFaDynCB2S(FFlOldFLMReader::readerCB,const std::string&,FFlLinkHandler*),
					 FFaDynCB2S(FFlOldFLMReader::identifierCB,const std::string&,int&),
					 "Fedem Link Model reader v1.0",
					 FedemAdmin::getCopyrightString());
}


void FFlOldFLMReader::identifierCB(const std::string& fileName, int& isFlmFile)
{
  // Seek for "LINK" in the file - check the first 100 lines
  if (!fileName.empty())
    isFlmFile = searchKeyword(fileName.c_str(),"LINK");
}


void FFlOldFLMReader::readerCB(const std::string& filename, FFlLinkHandler* link)
{
  FFlOldFLMReader reader(link);
  if (!reader.read(filename))
  {
    // Parsing failure, delete all link data to avoid attempt to resolve
    link->deleteGeometry();
    return;
  }

  // Assign beam properties
  ElementsCIter eit;
  for (eit = link->elementsBegin(); eit != link->elementsEnd(); ++eit)
    if ((*eit)->getCathegory() == FFlTypeInfoSpec::BEAM_ELM)
    {
      const int id = (*eit)->getID();
      if (link->getAttribute("PORIENT",id))
        (*eit)->setAttribute("PORIENT",id);
      if (link->getAttribute("PBEAMECCENT",id))
        (*eit)->setAttribute("PBEAMECCENT",id);
    }

  if (!link->resolve())
  {
    link->deleteGeometry();
    return;
  }

  // Late resolving of FLM-ish beam eccentricity
  FFlPBEAMECCENT* ecc;
  for (eit = link->elementsBegin(); eit != link->elementsEnd(); ++eit)
    if ((*eit)->getCathegory() == FFlTypeInfoSpec::BEAM_ELM)
      if ((ecc = dynamic_cast<FFlPBEAMECCENT*>((*eit)->getAttribute("PBEAMECCENT"))))
      {
        ecc->node1Offset.data() -= (*eit)->getNode(1)->getPos();
        ecc->node2Offset.data() -= (*eit)->getNode(2)->getPos();
      }
}


bool FFlOldFLMReader::read (const std::string& filename)
{
  FILE* fp = fopen(filename.c_str(),"r");
  if (!fp)
  {
    ListUI <<"\n *** Error: Can not open FE data file "<< filename <<"\n";
    return false;
  }

#ifdef FFL_TIMER
  myProfiler->startTimer("read");
#endif

  // Find the last LINK block in the file (there should normally be
  // only one, but if there's more than a single LINK specification
  // the last one is valid).

  char line[LINE_LENGTH];
  int nr1 = 0, nr2 = 0;
  int link_id, nr_element_nodes, nr_elements, nr_beams, nr_groups;
  int neval, maxniv, maxit, ngen;
  bool retval = true;

  while (retval && findNextIdentifier(fp,(char*)"LINK",NULL))
  {
    // Parse the link control data block
    if (getLine(line,LINE_LENGTH,fp))
      nr1 = sscanf(line, "%d %d %d %d %d", &link_id,
		   &nr_element_nodes, &nr_elements, &nr_beams, &nr_groups);
    else
      retval = false;

    if (getLine(line,LINE_LENGTH,fp))
      nr2 = sscanf(line, "%d %d %d %d",
		   &neval, &maxniv, &maxit, &ngen);
    else
      retval = false;
  }

  if (nr1 == 0 && nr2 == 0)
  {
    ListUI <<"\n *** Error: Need at least one LINK keyword in flm-file "
           << filename <<"\n";
    retval = false;
  }
  else if (nr1 != 5 || nr2 != 4)
  {
    ListUI <<"\n *** Error: Invalid LINK block in flm-file "
	   << filename <<"\n";
    retval = false;
  }

  // Find and parse all data blocks.

  rewind(fp);
  char* found = NULL;
  while (retval)
  {
    found = findNextIdentifier(fp,(char*)"ELMS",
			       "NODES",
			       "BEAM",
			       "SECTION",
			       "EPROP",
			       "EMAT",
			       "EOF",
			       NULL);
    if (!found)
    {
      ListUI <<"\n  ** Warning: Reached end-of-file without finding EOF keyword"
	     <<"\n              Possibly corrupt flm-file.\n";
      break;
    }
    else if (strcmp(found, "ELMS") == 0)
      retval = this->readElements(fp, nr_elements);
    else if (strcmp(found, "NODES") == 0)
      retval = this->readNodes(fp, nr_element_nodes);
    else if (strcmp(found, "BEAM") == 0)
      retval = this->readBeamData(fp, nr_beams);
    else if (strcmp(found, "SECTION") == 0)
      retval = this->readSectionData(fp, nr_groups);
    else if (strcmp(found, "EPROP") == 0)
      retval = this->readElemProp(fp, nr_groups);
    else if (strcmp(found, "EMAT") == 0)
      retval = this->readElemMat(fp, nr_groups);
    else if (strcmp(found, "EOF") == 0)
      break;
    else
    {
      // If we reach this point, there's a problem with the
      // findNextIdentifier() function.
      std::cerr <<" *** Internal error: Check findNextIdentifier()\n";
      retval = false;
    }
  }

  fclose(fp);
#ifdef FFL_TIMER
  myProfiler->stopTimer("read");
#endif
  return retval;
}


/*!
  Find the next identifier string which matches any one of the given
  character strings. Maximum number of strings in the input is
  256. Returns NULL if none was found, otherwise returns a pointer to
  the character string found.
*/

char* FFlOldFLMReader::findNextIdentifier (FILE* fp, char* first, ...)
{
  va_list vp;
  char  firstLetters[256];
  char* identifiers[256];
  char* currenttext = first;
  char  checkstring[80];

  va_start(vp,first);
  int i;
  for (i = 0; currenttext && i < 256; i++)
  {
    identifiers[i] = currenttext;
    firstLetters[i] = currenttext[0];
    currenttext = va_arg(vp,char*);
  }
  va_end(vp);

  while (fgets(checkstring,80,fp))
    for (int j = 0; j < i; j++)
      if (checkstring[0] == firstLetters[j])
        if (!strncmp(identifiers[j],checkstring,strlen(identifiers[j])))
	  return identifiers[j];

  return NULL;
}


/*!
  Read the next line from the input file.
  Ignore blank lines and lines starting with "'" (comment-lines).
*/

int FFlOldFLMReader::getLine (char* buf, const int n, FILE* fp)
{
  while (fgets(buf,n,fp))
  {
    int length = strlen(buf);
    if (length > n-2)
    {
      ListUI <<"\n *** Error: Line too long.\n"<< buf <<"\n";
      return 0;
    }
    int i = 0;
    while (i < length && isspace(buf[i])) i++;
    if (i < length && buf[i] != '\'') return length;
  }

  ListUI <<"\n *** Error: Premature end-of-file encountered."
         <<" FE data file is corrupt.\n";
  return 0;
}


/*!
  Data readers:
*/

bool FFlOldFLMReader::readNodes (FILE* fp, int count)
{
  char line[LINE_LENGTH];
  int node_id, state;
  double x, y, z;

  for (int i = 0; i < count; i++)
    if (!getLine(line,LINE_LENGTH,fp))
      return false;
    else if (sscanf(line,"%d %d %lf %lf %lf",&node_id,&state,&x,&y,&z) != 5)
    {
      ListUI <<"\n *** Error: Can not read nodal coordinates.\n"<< line;
      return false;
    }
    else
    {
#if FFL_DEBUG > 1
      std::cout <<"Reading node "<< node_id <<" "<< state
                <<" "<< x <<" "<< y <<" "<< z << std::endl;
#endif
      FFlNode* aNode = new FFlNode(node_id,x,y,z);
      if (state < 0) aNode->setExternal();
      if (!myLink->addNode(aNode)) return false;
    }

  return true;
}


bool FFlOldFLMReader::readElements (FILE* fp, int count)
{
  // Parse and set up everything for the elements except the node indices

  char line[LINE_LENGTH];
  std::vector<int> data, tmp(8,0);
  int id, j, type, group;

  // node mapping      1 2 3 4 5 6 7 8
  int old2newH8[8] = { 7,8,5,6,3,4,1,2 };

  for (int i = 0; i < count; i++)
  {
    if (!getLine(line,LINE_LENGTH,fp)) return false;
    if (sscanf(line,"%d %d %d",&id,&type,&group) != 3)
    {
      ListUI <<"\n *** Error: Can not read ELEMENT data.\n"<< line;
      return false;
    }
#if FFL_DEBUG > 1
    std::cout <<"Reading element "<< id <<" "<< type <<" "<< group;
#endif

    data.clear();
    bool continuation = false;
    do
    {
      int nchar = getLine(line,LINE_LENGTH,fp);
      if (nchar < 1) return false;

      for (j = 0; j < nchar-1; j++)
      {
	while (isspace(line[j]) && j < nchar) j++;
	if (j == nchar)
	  break;
	else if (line[j] != '&')
	{
	  continuation = false;
	  data.push_back(atoi(&line[j]));
	}
	else
	  continuation = true;

	while (!isspace(line[j])) j++;
      }
    }
    while (continuation);

#if FFL_DEBUG > 1
    for (int j : data) std::cout <<" "<< j;
    std::cout << std::endl;
#endif

    FFlElementBase* newElem = NULL;
    switch(type)
    {
      case FTS_ELEMENT:
	newElem = ElementFactory::instance()->create("TRI3", id);
	newElem->setAttribute("PTHICK", group);
	newElem->setAttribute("PMAT", group);
	break;
      case FQS_ELEMENT:
	newElem = ElementFactory::instance()->create("QUAD4", id);
	newElem->setAttribute("PTHICK", group);
	newElem->setAttribute("PMAT", group);
	break;
      case ITET_ELEMENT:
	newElem = ElementFactory::instance()->create("TET10", id);
	newElem->setAttribute("PMAT", group);
	break;
      case IPRI_ELEMENT:
	newElem = ElementFactory::instance()->create("WEDG15", id);
	newElem->setAttribute("PMAT", group);
	break;
      case IHEX_ELEMENT:
	newElem = ElementFactory::instance()->create("HEX20", id);
	newElem->setAttribute("PMAT", group);
	break;
      case HEXA_ELEMENT:
	newElem = ElementFactory::instance()->create("HEX8", id);
	newElem->setAttribute("PMAT", group);
	break;
      case CSTET_ELEMENT:
	newElem = ElementFactory::instance()->create("TET4", id);
	newElem->setAttribute("PMAT", group);
	break;
      case WEDGE_ELEMENT:
	newElem = ElementFactory::instance()->create("WEDG6", id);
	newElem->setAttribute("PMAT", group);
	break;
      case MASS_ELEMENT:
	newElem = ElementFactory::instance()->create("CMASS", id);
	newElem->setAttribute("PMASS", group);
	break;
      case BEAM_ELEMENT:
	newElem = ElementFactory::instance()->create("BEAM2", id);
	newElem->setAttribute("PMAT", group);
	newElem->setAttribute("PBEAMSECTION", group);
	break;
      case RIGID_BEAM_ELEMENT:
	newElem = ElementFactory::instance()->create("RGD", id);
	break;
      case RIGID_NFOLD_BEAM_ELEMENT:
	newElem = ElementFactory::instance()->create("RGD", id);
	break;
      default:
#ifdef FFL_DEBUG
	std::cout <<"  ** Warning: Unknown element type ignored "
                  << type << std::endl;
#endif
	break;
    }

    if (newElem)
    {
      if (type == HEXA_ELEMENT)
      {
        for (j = 0; j < 8; j++) tmp[j] = data[old2newH8[j]-1];
        newElem->setNodes(tmp);
      }
      else
        newElem->setNodes(data);

      if (!myLink->addElement(newElem)) return false;
    }
  }

  return true;
}


bool FFlOldFLMReader::readBeamData (FILE* fp, int count)
{
  char line[LINE_LENGTH];
  std::vector<double> data;

  for (int i = 0; i < count; i++)
  {
    int nchar = getLine(line,LINE_LENGTH,fp);
    if (nchar < 1) return false;

    data.clear();
    for (int j = 0; j < nchar-1; j++)
    {
      while (isspace(line[j]) && j < nchar) j++;
      if (j == nchar) break;
      data.push_back(atof(&line[j]));
      while (!isspace(line[j])) j++;
    }

#if FFL_DEBUG > 1
    std::cout <<"Reading beam data ";
    for (double d : data) std::cout <<" "<< d;
    std::cout << std::endl;
#endif

    int id = (int)data.front();
    if (data.size() > 3)
    {
      FFlPORIENT* ori = CREATE_ATTRIBUTE(FFlPORIENT,"PORIENT",id);
      ori->directionVector = FaVec3(data[1],data[2],data[3]);
      myLink->addAttribute(ori);
    }

    if (data.size() > 9)
    {
      FFlPBEAMECCENT* ecc = CREATE_ATTRIBUTE(FFlPBEAMECCENT,"PBEAMECCENT",id);
      ecc->node1Offset = FaVec3(data[4],data[5],data[6]);
      ecc->node2Offset = FaVec3(data[7],data[8],data[9]);
      myLink->addAttribute(ecc);
    }
  }
  return true;
}


bool FFlOldFLMReader::readSectionData (FILE* fp, int count)
{
  char line[LINE_LENGTH];
  int group, type;
  double a, iy, iz, it;
  double ky, kz, ys, zs;
  double csp1, csp2, csp3;

  for (int i = 0; i < count; i++)
  {
    if (!getLine(line,LINE_LENGTH,fp)) return false;
    if (sscanf(line,"%d %d %lf %lf %lf %lf",
	       &group, &type, &a, &iy, &iz, &it) != 6)
    {
      ListUI <<"\n *** Error: Can not read SECTION data.\n"<< line;
      return false;
    }
#if FFL_DEBUG > 1
    std::cout <<"Reading section data "
              << group <<" "<< type <<" "<< a
              <<" "<< iy <<" "<< iz <<" "<< it << std::endl;
#endif

    if (!getLine(line,LINE_LENGTH,fp)) return false;
    if (sscanf(line, "%lf %lf %lf %lf", &ky, &kz, &ys, &zs) != 4)
    {
      ListUI <<"\n *** Error: Can not read SECTION data.\n"<< line;
      return false;
    }

    if (!getLine(line,LINE_LENGTH,fp)) return false;
    if (sscanf(line, "%lf %lf %lf", &csp1, &csp2, &csp3) != 3)
    {
      ListUI <<"\n *** Error: Can not read SECTION data.\n"<< line;
      return false;
    }

    // check if this is not a dymmy attribute
    if (iy != 0.0 || iz != 0.0 || it != 0.0 ||
	ky != 0.0 || kz != 0.0 || ys != 0.0 || zs != 0.0)
    {
      // create Section data:
      FFlPBEAMSECTION* newAttribute = CREATE_ATTRIBUTE(FFlPBEAMSECTION,"PBEAMSECTION",group);
      newAttribute->crossSectionArea = a;
      newAttribute->Iy = iy;
      newAttribute->Iz = iz;
      newAttribute->It = it;
      newAttribute->Kxy = ky;
      newAttribute->Kxz = kz;
      newAttribute->Sy = ys;
      newAttribute->Sz = zs;
      myLink->addAttribute(newAttribute);
    }
  }

  return true;
}


bool FFlOldFLMReader::readElemProp (FILE* fp, int count)
{
  char line[LINE_LENGTH];
  std::vector<double> data;
  bool retval = true;

  for (int i = 0; i < count; i++)
  {
    data.clear();
    bool continuation = false;
    do
    {
      int nchar = getLine(line,LINE_LENGTH,fp);
      if (nchar < 1) return false;

      for (int j = 0; j < nchar-1; j++)
      {
	while (isspace(line[j]) && j < nchar) j++;
	if (j == nchar)
	  break;
	else if (line[j] != '&')
	{
	  continuation = false;
	  data.push_back(atof(&line[j]));
	}
	else
	  continuation = true;
	while (!isspace(line[j])) j++;
      }
    }
    while (continuation);

#if FFL_DEBUG > 1
    std::cout <<"Reading element property ";
    for (double d : data) std::cout <<" "<< d;
    std::cout << std::endl;
#endif

    int id = (int)data.front();
    int nrdata = data.size()-1;

    // create data based on count and values
    if (nrdata == 12)
    {
      // create PMASS property
      std::cerr <<" *** Internal error: PMASS property is not implemented yet!\n";
      retval = false;
    }
    else if (nrdata > 0 && data[1] > 0.0)
    {
      // create PTHICK property
      FFlPTHICK* newAttribute = CREATE_ATTRIBUTE(FFlPTHICK,"PTHICK",id);
      newAttribute->thickness = data[1];
      myLink->addAttribute(newAttribute);
    }
  }

  return retval;
}


bool FFlOldFLMReader::readElemMat (FILE* fp, int count)
{
  char line[LINE_LENGTH];
  int group;
  double elasticity, shearing, poissons, density;

  for (int i = 0; i < count; i++)
    if (!getLine(line,LINE_LENGTH,fp))
      return false;
    else if (sscanf(line,"%d %lf %lf %lf %lf",
		    &group, &elasticity, &shearing, &poissons, &density) != 5)
    {
      ListUI <<"\n *** Error: Can not read EMAT data.\n"<< line;
      return false;
    }
    else
    {
#if FFL_DEBUG > 1
      std::cout <<"Reading material data "
                << group <<" "<< elasticity <<" "<< shearing
                <<" "<< poissons <<" "<< density << std::endl;
#endif
      FFlPMAT* newAttribute = CREATE_ATTRIBUTE(FFlPMAT,"PMAT",group);
      newAttribute->youngsModule    = elasticity;
      newAttribute->shearModule     = shearing;
      newAttribute->poissonsRatio   = poissons;
      newAttribute->materialDensity = density;
      myLink->addAttribute(newAttribute);
    }

  return true;
}
