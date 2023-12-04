// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#include <fstream>
#include <cstring>
#include <cstdlib>

#include "FFlLib/FFlIOAdaptors/FFlFedemReader.H"
#include "FFlLib/FFlIOAdaptors/FFlReaders.H"
#include "FFlLib/FFlLinkHandler.H"
#include "FFlLib/FFlFEParts/FFlNode.H"
#include "FFlLib/FFlElementBase.H"
#include "FFlLib/FFlLoadBase.H"
#include "FFlLib/FFlAttributeBase.H"
#include "FFlLib/FFlFEAttributeSpec.H"
#include "FFlLib/FFlVisualBase.H"
#include "FFlLib/FFlFieldBase.H"
#include "FFlLib/FFlGroup.H"

#include "FFaLib/FFaDefinitions/FFaMsg.H"
#ifdef FFL_TIMER
#include "FFaLib/FFaProfiler/FFaProfiler.H"
#endif
#include "Admin/FedemAdmin.H"

#define TOKEN_SIZE 128

#ifdef FFL_TIMER
#define START_TIMER(function) myProfiler->startTimer(function)
#define STOPP_TIMER(function) myProfiler->stopTimer(function)
#else
#define START_TIMER(function)
#define STOPP_TIMER(function)
#endif


bool FFlFedemReader::ignoreCheckSum = false;


FFlFedemReader::FFlFedemReader(FFlLinkHandler* aLink) : FFlReaderBase(aLink)
{
#ifdef FFL_TIMER
  myProfiler = new FFaProfiler("FedemReader profiler");
  myProfiler->startTimer("FFlFedemReader");
#endif

  // set up the reader information:

  myFieldResolvers["FTLVERSION"] = FFaDynCB1M(FFlFedemReader, this,
					      resolveVersion, const ftlField&);

  myFieldResolvers["NODE"] = FFaDynCB1M(FFlFedemReader, this,
					resolveNodeField, const ftlField&);

  myFieldResolvers["GROUP"] = FFaDynCB1M(FFlFedemReader, this,
					 resolveGroupField, const ftlField&);

  std::vector<std::string> keys;

  // get element keys:
  ElementFactory::instance()->getKeys(keys);
  for (const std::string& key : keys)
    myFieldResolvers[key] = FFaDynCB1M(FFlFedemReader, this,
				       resolveElementField, const ftlField&);

  // get load keys:
  LoadFactory::instance()->getKeys(keys);
  for (const std::string& key : keys)
    myFieldResolvers[key] = FFaDynCB1M(FFlFedemReader, this,
				       resolveLoadField, const ftlField&);

  // get attribute keys:
  AttributeFactory::instance()->getKeys(keys);
  for (const std::string& key : keys)
    myFieldResolvers[key] = FFaDynCB1M(FFlFedemReader, this,
				       resolveAttributeField, const ftlField&);

  // get visual keys:
  VisualFactory::instance()->getKeys(keys);
  for (const std::string& key : keys)
    myFieldResolvers[key] = FFaDynCB1M(FFlFedemReader, this,
				       resolveVisualField, const ftlField&);

#ifdef FFL_DEBUG
  std::cout <<"Registered field resolvers"<< std::endl;
  for (const FieldResolverMap::value_type& resolver : myFieldResolvers)
    std::cout <<"  "<< resolver.first << std::endl;
#endif

  okAdd = false;
  nErr = version = counter = 0;
  linkChecksum = 0;
}


FFlFedemReader::~FFlFedemReader()
{
#ifdef FFL_TIMER
  myProfiler->stopTimer("FFlFedemReader");
  myProfiler->report();
  delete myProfiler;
#endif
}


void FFlFedemReader::init()
{
  FFlReaders::instance()->registerReader("Fedem Technology Link Data","ftl",
					 FFaDynCB2S(FFlFedemReader::readerCB,const std::string&,FFlLinkHandler*),
					 FFaDynCB2S(FFlFedemReader::identifierCB,const std::string&,int&),
					 "Fedem Technology Link Data reader v1.0",
					 FedemAdmin::getCopyrightString());
}


static int lastFileVersion = 1;

void FFlFedemReader::identifierCB(const std::string& fileName, int& isFtlFile)
{
  if (!fileName.empty())
    isFtlFile = searchKeyword(fileName.c_str(),"FTLVERSION");
  else // return the version number of the last file read
    isFtlFile = lastFileVersion;
}


void FFlFedemReader::readerCB(const std::string& filename, FFlLinkHandler* link)
{
  FFlFedemReader reader(link);
  if (!reader.read(filename))
    link->deleteGeometry(); // parsing failure, delete all link data
  else if (ignoreCheckSum)
    return; // we don't care about any file checksum mismatch

  else if (reader.linkChecksum > 0)
  {
    // Check if the file has been edited manually after it was saved
    lastFileVersion = reader.version > 0 ? reader.version : 1;
    unsigned int newChecksum = link->calculateChecksum(0, reader.version > 5);
    if (reader.linkChecksum == newChecksum) return;

    ListUI <<"\n  ** Warning: FE data file "<< filename
	   <<"\n     may have been edited manually (checksum mismatch)";
    if (FFaMsg::usingDefault())
      std::cout <<"\n     ";
    else
      std::cout << filename <<": checksums ";
    std::cout << reader.linkChecksum <<" (old) "
              << newChecksum <<" (new)"<< std::endl;
  }
}


bool FFlFedemReader::read(std::istream& is)
{
  START_TIMER("read");

  okAdd = true;
  linkChecksum = 0;
  nErr = counter = 0;

  is.seekg(0);
  ftlField field;
  while (okAdd && this->getNextField(is,field))
  {
    FieldResolverMap::iterator it = myFieldResolvers.find(field.label);
    if (it != myFieldResolvers.end())
      it->second.invoke(field);
    else if (!FFlFEAttributeSpec::isObsolete(field.label))
      ListUI <<"\n  ** Warning: Unknown FTL-entry ignored \""
             << field.label <<"\"\n";

    field.label.resize(0);
    field.entries.clear();
    field.refs.clear();
  }

  if (nErr > 0)
    ListUI <<"\n *** Error: A total of "<< nErr
	   <<" syntax and/or topology errors have been detected."
	   <<"\n     The FE data file is corrupt.\n";

  STOPP_TIMER("read");
  return nErr == 0 && okAdd;
}


bool FFlFedemReader::read(const std::string& filename)
{
  std::ifstream fs(filename.c_str());
  if (!fs)
  {
    ListUI <<"\n *** Error: Can not open FE data file "<< filename <<"\n";
    return false;
  }
  else
    return this->read(fs);
}


static void parseError(int nErr, const std::string& label, const std::string& entry)
{
  if (nErr == 1)
    ListUI <<"     The last error was";
  else
    ListUI <<"     The last "<< nErr <<" errors were";
  ListUI <<" found while parsing entry: \""<< label <<"{"<< entry <<" ...}\"\n";
}


void FFlFedemReader::resolveVersion(const ftlField& field)
{
  if (FFlFieldBase::parseNumericField(version,field.entries.front())) return;

  nErr++;
  parseError(1,field.label,field.entries.front());
}


void FFlFedemReader::resolveNodeField(const ftlField& field)
{
  START_TIMER("resolveNodeField");

  int id = 0, state = 0, lErr = nErr;
  double x, y, z;
  if (!FFlFieldBase::parseNumericField(id   ,field.entries[0])) nErr++;
  if (!FFlFieldBase::parseNumericField(state,field.entries[1])) nErr++;
  if (!FFlFieldBase::parseNumericField(x    ,field.entries[2])) nErr++;
  if (!FFlFieldBase::parseNumericField(y    ,field.entries[3])) nErr++;
  if (!FFlFieldBase::parseNumericField(z    ,field.entries[4])) nErr++;

  FFlNode* newNode = new FFlNode(id,x,y,z,state);

  for (const RefFieldMap::value_type& ref : field.refs)
    if (!ref.second.id.empty())
      if (ref.first == "PCOORDSYS") // Local solution coordinate system
        newNode->setLocalSystem(ref.second.id.front());

  okAdd = myLink->addNode(newNode);

  if (nErr > lErr) parseError(nErr-lErr,field.label,field.entries[0]);
  STOPP_TIMER("resolveNodeField");
}


void FFlFedemReader::resolveElementField(const ftlField& field)
{
  START_TIMER("resolveElementField");

  int id = 0, node, lErr = nErr;
  std::vector<std::string>::const_iterator it = field.entries.begin();
  if (!FFlFieldBase::parseNumericField(id,*it++)) nErr++;

  // nodes
  std::vector<int> nodeRefs;
  nodeRefs.reserve(field.entries.size()-1);

  while (it != field.entries.end())
    if (FFlFieldBase::parseNumericField(node,*it++))
      nodeRefs.push_back(node);
    else
      nErr++;

  FFlElementBase* newElem = ElementFactory::instance()->create(field.label,id);
  if (newElem)
  {
    newElem->setNodes(nodeRefs);

    for (const RefFieldMap::value_type& ref : field.refs)
      if (!ref.second.id.empty())
      {
        if (newElem->setVisual(ref.first,ref.second.id.front()))
          continue;
        else if (ref.first == "FE") // reference to other finite element
          newElem->setFElement(ref.second.id.front());
        else if (!newElem->setAttribute(ref.first,ref.second.id.front()))
        {
          nErr++;
          ListUI <<"\n *** Error: Can not resolve reference {"
                 << ref.first <<" "<< ref.second.id.front() <<"}\n";
        }
      }

    okAdd = myLink->addElement(newElem,false);
  }
#ifdef FFL_DEBUG
  else
    std::cout <<"  ** Ignoring element field "<< field.label <<" "<< id
              << std::endl;
#endif

  if (nErr > lErr) parseError(nErr-lErr,field.label,field.entries[0]);
  STOPP_TIMER("resolveElementField");
}


void FFlFedemReader::resolveLoadField(const ftlField& field)
{
  START_TIMER("resolveLoadField");

  int id = 0, lErr = nErr;
  std::vector<std::string>::const_iterator it = field.entries.begin();
  if (!FFlFieldBase::parseNumericField(id,*it++)) nErr++;

  FFlLoadBase* load = LoadFactory::instance()->create(field.label,id);
  if (load)
  {
    for (FFlFieldBase* lfield : *load)
      if (!lfield->parse(it,field.entries.end())) nErr++;

    for (const RefFieldMap::value_type& ref : field.refs)
      if (!ref.second.id.empty())
      {
        if (ref.first == "TARGET")
          load->setTarget(ref.second.id);
        else if (!load->setAttribute(ref.first,ref.second.id.front()))
        {
          nErr++;
          ListUI <<"\n *** Error: Can not resolve reference {"
                 << ref.first <<" "<< ref.second.id.front() <<"}\n";
        }
      }

    myLink->addLoad(load);
  }

  if (nErr > lErr) parseError(nErr-lErr,field.label,field.entries[0]);
  STOPP_TIMER("resolveLoadField");
}


void FFlFedemReader::resolveAttributeField(const ftlField& field)
{
  START_TIMER("resolveAttributeField");

  int id = 0, lErr = nErr;
  std::vector<std::string>::const_iterator it = field.entries.begin();
  if (!FFlFieldBase::parseNumericField(id,*it++)) nErr++;

  FFlAttributeBase* attr = AttributeFactory::instance()->create(field.label,id);
  if (attr)
  {
    attr->resize(field.entries.size()-1);
    for (FFlFieldBase* afield : *attr)
      if (!afield->parse(it,field.entries.end())) nErr++;

    for (const RefFieldMap::value_type& ref : field.refs)
      if (ref.first == "NAME" && !ref.second.options.empty())
        attr->setName(ref.second.options.front());
      else if (!ref.second.id.empty())
        if (!attr->setAttribute(ref.first,ref.second.id.front()))
        {
          nErr++;
          ListUI <<"\n *** Error: Can not resolve reference {"
                 << ref.first <<" "<< ref.second.id.front() <<"}\n";
        }

    // Specify field.label as the key to the attribute type map container, to
    // ensure that the ordering is not changed in case it is an obsolete name
    // (this is needed to avoid checksum difference for old files)
    myLink->addAttribute(attr,false,field.label);
  }
#ifdef FFL_DEBUG
  else
    std::cout <<"  ** Ignoring attribute field "<< field.label <<" "<< id
              << std::endl;
#endif

  if (nErr > lErr) parseError(nErr-lErr,field.label,field.entries[0]);
  STOPP_TIMER("resolveAttributeField");
}


void FFlFedemReader::resolveVisualField(const ftlField& field)
{
  START_TIMER("resolveVisualField");

  int id = 0, lErr = nErr;
  std::vector<std::string>::const_iterator it = field.entries.begin();
  if (!FFlFieldBase::parseNumericField(id,*it++)) nErr++;

  FFlVisualBase* vis = VisualFactory::instance()->create(field.label,id);
  if (vis)
  {
    for (FFlFieldBase* vfield : *vis)
      if (!vfield->parse(it,field.entries.end())) nErr++;

    myLink->addVisual(vis);
  }
#ifdef FFL_DEBUG
  else
    std::cout <<"  ** Ignoring visual field "<< field.label <<" "<< id
              << std::endl;
#endif

  if (nErr > lErr) parseError(nErr-lErr,field.label,field.entries[0]);
  STOPP_TIMER("resolveVisualField");
}


void FFlFedemReader::resolveGroupField(const ftlField& field)
{
  START_TIMER("resolveGroupField");

  int id = 0, lErr = nErr;
  std::vector<std::string>::const_iterator it = field.entries.begin();
  if (!FFlFieldBase::parseNumericField(id,*it++)) nErr++;
  FFlGroup* aGroup = new FFlGroup(id);

  while (it != field.entries.end())
    if (FFlFieldBase::parseNumericField(id,*it++))
      aGroup->addElement(id);
    else
      nErr++;

  for (const RefFieldMap::value_type& ref : field.refs)
    if (ref.first == "NAME" && !ref.second.options.empty())
      aGroup->setName(ref.second.options.front());

  aGroup->sortElements();
  myLink->addGroup(aGroup);

  if (nErr > lErr) parseError(nErr-lErr,field.label,field.entries[0]);
  STOPP_TIMER("resolveGroupField");
}


bool FFlFedemReader::getNextField(std::istream& is, ftlField& fl) const
{
  const char cmt = '#';

  enum {LABEL_SEARCH,
	LABEL_READ,
	LABEL_VALID,
	LABEL_ERROR,

	ENTRY_READ,
	ENTRY_VALID,
	EOF_ERROR,

	REF_READ_LABEL,
	REF_READ_ID,
	REF_READ_ADDOPTS,
	REF_VALID,
	REF_ERROR,

	READ_DONE
  };

  int mode = LABEL_SEARCH;

  RefFieldMap::iterator currentRefIt;

#if FFL_DEBUG > 2
  std::cout <<"===> getNextField\n";
#endif

  char c, str[BUFSIZ];
  while (is.get(c) && !is.eof())
    switch (mode)
    {
      case LABEL_SEARCH:
#if FFL_DEBUG > 2
	std::cout <<"---> Label search ["<< c <<"]\n";
#endif
	// Eat white-spaces
	while (!is.eof() && isspace(c)) is.get(c);

	if (c == cmt)
	{
	  is.getline(str,BUFSIZ,'\n');
	  if (strlen(str) > 15 && std::string(str+1,14) == "File checksum:")
	  {
	    char* csend;
	    linkChecksum = strtoul(str+16,&csend,10);
	  }
	  ++counter;
	}
	else
	{
	  mode = LABEL_READ;
	  is.putback(c);
	}
	break;

      case LABEL_READ:
	fl.label.reserve(TOKEN_SIZE);
	while (isalnum(c) && !is.eof())
	{
	  fl.label += toupper(c);
	  is.get(c);
	}
#if FFL_DEBUG > 2
	std::cout <<"---> Label read \""<< fl.label <<"\"\n";
#endif
	mode = fl.label.empty() ? LABEL_ERROR : LABEL_VALID;

	// Eat white-spaces
	while (isspace(c) && !is.eof()) is.get(c);

	if (c == cmt)
	{
	  is.ignore(BUFSIZ,'\n');
	  ++counter;
	}
	else if (c != '{')
	{
	  mode = LABEL_ERROR;
	  fl.label.resize(0);
	  is.ignore(BUFSIZ,'\n');
	  ++counter;
	}
	break;

      case LABEL_ERROR:
#if FFL_DEBUG > 2
	std::cout <<"---> Label error (line "<< counter <<")";
	if (is.eof()) std::cout <<"EOF";
	std::cout << std::endl;
#endif
	mode = LABEL_SEARCH;
	is.putback(c);
	break;

      case LABEL_VALID:
#if FFL_DEBUG > 2
	std::cout <<"---> Found label \""<< fl.label <<"\"\n";
#endif
	// Translation of obsolete element keywords
	if (fl.label == "FFT3")
	  fl.label = "TRI3";
	else if (fl.label == "FFQ4")
	  fl.label = "QUAD4";

	is.putback(c);
	mode = ENTRY_READ;
	break;

      case ENTRY_READ:
      {
	// Eat white-spaces
	while (isspace(c) && !is.eof()) is.get(c);

	std::string singleEntry;
	singleEntry.reserve(TOKEN_SIZE);
	while (isgraph(c) &&
	       c!=cmt &&
	       c!='{' &&
	       c!='}' &&
	       c!='"' &&
	       !is.eof())
	{
	  if (singleEntry.capacity() < singleEntry.size()+1)
	    singleEntry.reserve(singleEntry.capacity() + TOKEN_SIZE);
	  singleEntry += toupper(c);
	  is.get(c);
	}

	if (c == cmt)
	{
	  is.ignore(BUFSIZ,'\n');
	  ++counter;
	}
	else if (c == '"')
	{
	  is.getline(str,BUFSIZ,'"');
	  singleEntry = str;
	}
	else if (c == '{')
	  mode = REF_READ_LABEL;
	else if (c == '}')
	  mode = ENTRY_VALID;
	else if (is.eof())
	  mode = EOF_ERROR;

	if (!singleEntry.empty())
	  fl.entries.push_back(singleEntry);

#if FFL_DEBUG > 2
	std::cout <<"---> Read entry \""<< singleEntry <<"\"\n";
#endif
	break;
      }

      case ENTRY_VALID:
#if FFL_DEBUG > 2
	std::cout <<"---> Valid entry\n";
#endif
	is.putback(c);
	mode = READ_DONE;
	break;

      case REF_READ_LABEL:
      {
	// Eat white-spaces
	while (isspace(c) && !is.eof()) is.get(c);

	std::string singleEntry;
	singleEntry.reserve(TOKEN_SIZE);
	while (isgraph(c) &&
	       c!=cmt &&
	       c!='{' &&
	       c!='}' &&
	       c!='"' &&
	       !is.eof())
	{
	  singleEntry += toupper(c);
	  is.get(c);
	}
#if FFL_DEBUG > 2
	std::cout <<"---> Read ref label \""<< singleEntry <<"\"\n";
#endif

	if (!singleEntry.empty())
	  currentRefIt = fl.refs.insert(std::make_pair(singleEntry,ftlRefField()));

	if (c == cmt)
	{
	  is.ignore(BUFSIZ,'\n');
	  ++counter;
	}
	else if (c == '"' || c == '{' || c == '}')
	{
	  is.putback(c);
	  mode = REF_ERROR;
	}
	else if (is.eof())
	  mode = EOF_ERROR;
	else
	  mode = REF_READ_ID;
	break;
      }

      case REF_READ_ID:
	// Eat white-spaces
	while (isspace(c) && !is.eof()) is.get(c);

	if (isdigit(c))
	{
	  is.putback(c);
	  int id;
	  is >> id;
#if FFL_DEBUG > 2
	  std::cout <<"---> Read ref ID "<< id << std::endl;
#endif
	  currentRefIt->second.id.push_back(id);
	}
	else if (c == cmt)
	{
	  is.ignore(BUFSIZ,'\n');
	  ++counter;
	}
	else
	{
	  is.putback(c);
	  mode = REF_READ_ADDOPTS;
	}
	break;

      case REF_READ_ADDOPTS:
      {
	// Eat white-spaces
	while (isspace(c) && !is.eof()) is.get(c);

	std::string singleEntry;
	singleEntry.reserve(TOKEN_SIZE);
	while (isgraph(c) &&
	       c!=cmt &&
	       c!='{' &&
	       c!='}' &&
	       c!='"' &&
	       !is.eof())
	{
	  if (singleEntry.capacity() < singleEntry.size()+1)
	    singleEntry.reserve(singleEntry.capacity() + TOKEN_SIZE);
	  singleEntry += toupper(c);
	  is.get(c);
	}

	if (c == cmt)
	{
	  is.ignore(BUFSIZ,'\n');
	  ++counter;
	}
	else if (c == '"')
	{
	  is.getline(str,BUFSIZ,'"');
	  singleEntry = str;
	}
	else if (c == '{')
	{
	  is.putback(c);
	  mode = REF_ERROR;
	}
	else if (c == '}')
	  mode = REF_VALID;
	else if (is.eof())
	  mode = EOF_ERROR;
#if FFL_DEBUG > 2
	std::cout <<"---> Read ref options \""<< singleEntry <<"\"\n";
#endif

	if (!singleEntry.empty())
	  currentRefIt->second.options.push_back(singleEntry);

	break;
      }

      case REF_VALID:
#if FFL_DEBUG > 2
	std::cout <<"---> Valid reference\n";
#endif
	is.putback(c);
	mode = ENTRY_READ;
	break;

      case REF_ERROR:
	ListUI <<"\n *** Error: Unexpected character \'"<< c
	       <<"\' encountered while reading reference ID number."
	       <<"\n     Line "<< ++counter <<": "<< fl.label <<"{";
	for (const std::string& field : fl.entries) ListUI << field <<" ";
	ListUI <<"...}\n";
	const_cast<FFlFedemReader*>(this)->nErr++;
	return false;

      case EOF_ERROR:
	ListUI <<"\n *** Error: Premature end-of-file encountered."
	       <<" FE data file is corrupt.\n";
	const_cast<FFlFedemReader*>(this)->nErr++;
	return false;

      case READ_DONE:
	++counter;
	return true;
    }

  // End-of-file reached
  if (mode != ENTRY_VALID)
    return false;

  // Missing newline character, but this is still OK
#if FFL_DEBUG > 2
  std::cout <<"---> Valid entry\n";
#endif
  ++counter;
  return true;
}
