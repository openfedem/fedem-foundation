// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#include "FFlrLib/FFlrResultResolver.H"
#include "FFlLib/FFlElementBase.H"
#include "FFlLib/FFlFEParts/FFlNode.H"
#include "FFrLib/FFrExtractor.H"
#include "FFrLib/FFrVariableReference.H"
#include <algorithm>
#include <set>

static const FFrEntryVec* ourElmResFields  = NULL;;
static const FFrEntryVec* ourNodeResFields = NULL;;

static std::set<int> empty;

std::map<std::string,int> FFlrResultResolver::errMsg;


/*!
  Constructs an error message when a result item is not found.
  The message is stored in a map counting the number of instances of each error.
*/

static void errorMessage (const std::string& name,
                          const std::string& type,
                          const std::string& className,
                          const std::string& setName, bool inSet = true)
{
  std::string msg = className + " results with name \"" + name + "\"";
  if (type != "" && type != "Any")
    msg += " of type " + type;
  msg += " not found";
  if (inSet)
    msg += " in group \"" + setName + "\"";

  FFlrResultResolver::addMsg(msg);
}


/*!
  Finds all leafs in the RDB tree below \a root, that match \a type as in
  FFrEntryBase::getType() and \a descr as in FFrEntryBase::getDescription()
*/

static void findVariables (std::vector<FFrVariableReference*>& result,
                           const FFrEntryBase* root,
                           const std::string& type, const std::string& descr)
{
  if (!root)
    return;

  if (root->hasDataFields())
    for (FFrEntryBase* field : *root->getDataFields())
      findVariables(result,field,type,descr);

  else if (descr == root->getDescription())
    if (type == root->getType() || type == "" || type == "Any")
      result.push_back((FFrVariableReference*)root);
}


/*!
  Overloaded function taking an array of FFrEntryBase pointers
  together with an array index as arguments.
*/

static void findVariables (std::vector<FFrVariableReference*>& result,
                           const FFrEntryVec& root, int lIndex,
                           const std::string& type, const std::string& descr)
{
  if (lIndex > 0 && (size_t)lIndex <= root.size())
    findVariables(result,root[lIndex-1],type,descr);
}


/*!
  Finds first node in RDB of type \a type, returns NULL if not found.
  The type of \a root is not examined.
  \sa FFrEntryBase::getType()
*/

static FFrEntryBase* findIGByType (const FFrEntryBase* root,
                                   const std::string& type)
{
  if (!root || !root->hasDataFields())
    return NULL;

  // If the type string contains a '+' sign,
  // it means we are searching for multiple result sets
  bool combSet = type.find(" + ") != std::string::npos;

  FFrEntryBase* result = NULL;
  for (FFrEntryBase* field : *root->getDataFields())
    if (field->getType() == type)
      return field;
    else if (combSet && type.find(field->getType()) != std::string::npos)
      return field;
    else if ((result = findIGByType(field,type)))
      break;

  return result;
}


/*!
  A binary search in the field entry base vector of numbered item groups.
*/

static FFrEntryBase* findItem (const FFrEntryVec& fields, int key)
{
  FFrEntryVec::const_iterator it = std::find_if(fields.begin(),fields.end(),
                                                [key](FFrEntryBase* r)
                                                { return r->getUserID() == key; });
  return it == fields.end() ? NULL : *it;
}


/*!
  Returns read operations of all result quantities matching the specification.
*/

static void getLocalReadOps (FFaOperationVec& readOps,
                             const std::string& resClassName,
                             const FFlElementBase* elm, int lNode,
                             const std::string& type,
                             const std::string& variableName,
                             const std::string& resSetName, bool onlyResSet)
{
  static const FFlElementBase* prevElm = NULL;
  static FFrEntryBase* elmRes = NULL;

  if (!ourElmResFields)
    elmRes = NULL; // No element results for current part, silently ignore
  else if (prevElm != elm)
  {
    elmRes = findItem(*ourElmResFields,elm->getID());
    if (!elmRes && empty.insert(elm->getID()).second)
      FFlrResultResolver::addMsg("No results for some " +
                                 elm->getTypeName() + " elements");
  }
  if (!elmRes) return;

  // Find the right result class
  FFrEntryBase* catRoot = findIGByType(elmRes,resClassName);
  if (!catRoot) return;

  // Find the wanted variable references
  std::vector<FFrVariableReference*> resultRefs;
  if (onlyResSet)
  {
    FFrEntryBase* resSetRoot = findIGByType(catRoot,resSetName);
    if (resSetRoot)
      findVariables(resultRefs,*resSetRoot->getDataFields(),lNode,
                    type,variableName);
  }
  else
    for (FFrEntryBase* resSetRoot : *catRoot->getDataFields())
      findVariables(resultRefs,*resSetRoot->getDataFields(),lNode,
                    type,variableName);

  if (resultRefs.empty())
    errorMessage(variableName,type,resClassName,resSetName,onlyResSet);

  // Get read operations from the variable references
  for (FFrVariableReference* varRef : resultRefs)
    readOps.push_back(varRef->getReadOperation());
}


void FFlrResultResolver::addMsg(const std::string& msg)
{
  std::map<std::string,int>::iterator it = errMsg.find(msg);
  if (it == errMsg.end())
    errMsg[msg] = 1;
  else
    it->second++;
}


char FFlrResultResolver::setLinkInFocus(int baseId, FFrExtractor* rdb,
                                        const char kind)
{
  errMsg.clear();
  empty.clear();

  if (kind != 'n')
    ourElmResFields = findFEResults(baseId, rdb, "Elements");
  else
    ourElmResFields = NULL;

  if (kind != 'e')
    ourNodeResFields = findFEResults(baseId, rdb, "Nodes");
  else
    ourNodeResFields = NULL;

  if (ourElmResFields && ourNodeResFields)
    return 'b';
  else if (ourElmResFields)
    return 'e';
  else if (ourNodeResFields)
    return 'n';

  return false;
}


void FFlrResultResolver::clearLinkInFocus()
{
  ourElmResFields = ourNodeResFields = NULL;
  errMsg.clear();
  empty.clear();
}


/*!
  Returns the read operation for the Position matrix result of an Object.
*/

FFaOperationBase*
FFlrResultResolver::findPosition(const std::string& oType, int baseId,
                                 FFrExtractor* rdb)
{
  FFrEntryBase* entry = rdb->findVar(oType,baseId,"Position matrix");
  if (entry && entry->isVarRef())
    return static_cast<FFrVariableReference*>(entry)->getReadOperation();

  return NULL;
}


/*!
  Returns the main group of results in the FE part referred to by \a baseId.
  The main group of results is typically the collection of node results,
  or the collection of element results.
*/

const FFrEntryVec*
FFlrResultResolver::findFEResults(int baseId, FFrExtractor* rdb,
                                  const std::string& FEResultName)
{
  if (!rdb) return NULL;

  FFrEntryBase* resVar = rdb->findVar("Part",baseId,FEResultName);
  if (!resVar) return NULL;

  // FE results item group found

  if (resVar->hasDataFields())
    return resVar->getDataFields(); // Found FE result fields
  else
    return NULL; // Found FFrEntry with descriptor, but it is not an item group
}



void FFlrResultResolver::getElmNodeReadOps(FFaOperationVec& readOps,
                                           const FFlElementBase* elm, int lNode,
                                           const std::string& type,
                                           const std::string& variableName,
                                           const std::string& resSetName,
                                           bool onlyResSetMatch)
{
  getLocalReadOps(readOps, "Element nodes", elm, lNode,
                  type, variableName, resSetName, onlyResSetMatch);
}


void FFlrResultResolver::getEvalPReadOps(FFaOperationVec& readOps,
                                         const FFlElementBase* elm, int lNode,
                                         const std::string& type,
                                         const std::string& variableName,
                                         const std::string& resSetName,
                                         bool onlyResSetMatch)
{
  getLocalReadOps(readOps, "Evaluation points", elm, lNode,
                  type, variableName, resSetName, onlyResSetMatch);
}


void FFlrResultResolver::getElmReadOps(FFaOperationVec& readOps,
                                       const FFlElementBase* elm,
                                       const std::string& type,
                                       const std::string& variableName,
                                       const std::string& resSetName,
                                       bool onlyResSetMatch)
{
  if (!ourElmResFields) return;

  FFrEntryBase* elmRes = findIGByType(findItem(*ourElmResFields,elm->getID()),"Element");
  FFrEntryBase* resSetRoot = onlyResSetMatch ? findIGByType(elmRes,resSetName) : elmRes;

  // Find the wanted variable references
  std::vector<FFrVariableReference*> resultRefs;
  findVariables(resultRefs,resSetRoot,type,variableName);

  if (resultRefs.empty())
    errorMessage(variableName,type,"Elements",resSetName,onlyResSetMatch);

  // Get read operations from the variable references
  for (FFrVariableReference* varRef : resultRefs)
    readOps.push_back(varRef->getReadOperation());
}


void FFlrResultResolver::getNodeReadOps(FFaOperationVec& readOps,
                                        const FFlNode* node,
                                        const std::string& type,
                                        const std::string& variableName,
                                        const std::string& resSetName,
                                        bool onlyResSetMatch)
{
  if (!ourNodeResFields) return;

  FFrEntryBase* nodeRes = findItem(*ourNodeResFields,node->getID());
  FFrEntryBase* resSetRoot = onlyResSetMatch ? findIGByType(nodeRes,resSetName) : nodeRes;

  // Find the wanted variable references
  std::vector<FFrVariableReference*> resultRefs;
  findVariables(resultRefs,resSetRoot,type,variableName);

  if (resultRefs.empty())
    errorMessage(variableName,type,"Nodes",resSetName,onlyResSetMatch);

  // Get read operations from the variable references
  for (FFrVariableReference* varRef : resultRefs)
    readOps.push_back(varRef->getReadOperation());
}


FFaOperationBase*
FFlrResultResolver::getNodeReadOp(const FFrEntryBase* resNode,
                                  const std::string& type,
                                  const std::string& variableName,
                                  const std::string& resSetName)
{
  FFrEntryBase* resSetRoot = findIGByType(resNode,resSetName);

  // Find the wanted variable references
  std::vector<FFrVariableReference*> resultRefs;
  findVariables(resultRefs,resSetRoot,type,variableName);

  // Get read operation from the variable reference
  if (resultRefs.size() == 1)
    return resultRefs.front()->getReadOperation();
  else if (resultRefs.empty())
  {
    errorMessage(variableName,type,"Nodes",resSetName);
    return NULL;
  }

  // If more than one variable matches the search criterion
  // use the one associated with the most recent result container
  unsigned int stamp, latest = 0;
  FFrVariableReference* result = NULL;
  for (FFrVariableReference* var : resultRefs)
    if ((stamp = var->getTimeStamp()) > latest)
    {
      result = var;
      latest = stamp;
    }

  return result ? result->getReadOperation() : NULL;
}
