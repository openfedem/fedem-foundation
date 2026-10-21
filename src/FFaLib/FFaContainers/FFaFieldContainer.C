// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

/*!
  \file FFaFieldContainer.C
  \brief Base class for the FEDEM model database structure.
*/

#include <algorithm>

#include "FFaFieldContainer.H"
#include "FFaFieldBase.H"
#include "FFaReference.H"
#include "FFaReferenceList.H"

//! \cond DO_NOT_DOCUMENT
Fmd_SOURCE_INIT(FcFieldContainer, FFaFieldContainer, FFaFieldContainer);
//! \endcond


FFaFieldContainer::FFaFieldContainer()
{
  Fmd_CONSTRUCTOR_INIT(FFaFieldContainer);
}


FFaFieldContainer::~FFaFieldContainer()
{
  // Tell every container that is referring to me that I am no longer here
  for (FFaReferenceBase* ref : myRefBy)
    ref->zeroOutOrRemoveFromList();
}


bool FFaFieldContainer::erase()
{
  if (!this->eraseOptions())
    return false;

  delete this;
  return true;
}


void FFaFieldContainer::resolve(FindCB& findCB)
{
#ifdef FFA_DEBUG
  std::cout <<"FFaFieldContainer::resolve(): "<< this->getTypeIDName()
            <<" "<< this->getResolvedID() << std::endl;
#endif

  for (FFaReferenceBase* ref : myRefTo)
    ref->resolve(findCB);

  for (FFaReferenceListBase* ref : myRefLists)
    ref->resolve(findCB);
}


void FFaFieldContainer::unresolve()
{
  for (FFaReferenceBase* ref : myRefTo)
    ref->unresolve();

  for (FFaReferenceListBase* ref : myRefLists)
    ref->unresolve();
}


void FFaFieldContainer::updateReferences(int oldAssId, int newAssId)
{
  for (FFaReferenceBase* ref : myRefTo)
    ref->updateAssemblyRef(oldAssId,newAssId);

  for (FFaReferenceListBase* ref : myRefLists)
    ref->updateAssemblyRef(oldAssId,newAssId);
}


void FFaFieldContainer::updateReferences(const IntVec& oldAssId,
                                         const IntVec& newAssId)
{
  for (FFaReferenceBase* ref : myRefTo)
    ref->updateAssemblyRef(oldAssId,newAssId);

  for (FFaReferenceListBase* ref : myRefLists)
    ref->updateAssemblyRef(oldAssId,newAssId);
}


/*!
  The field identifier, which also is the keyword used in the model file,
  is used as key in the \a mapToFill.
*/

void FFaFieldContainer::getFields(FieldMap& mapToFill) const
{
  mapToFill.clear();
  for (const FieldContainerMap::value_type& it : myFields)
    mapToFill.emplace(*(it.first),it.second);
}


FFaFieldBase* FFaFieldContainer::getField(const std::string& fieldName) const
{
  FieldContainerMap::const_iterator it;
  FDictIt strit = FieldContainerDict::instance()->find(fieldName);
  if (strit != FieldContainerDict::instance()->end())
    return (it = myFields.find(strit)) == myFields.end() ? NULL : it->second;
  else
    return NULL;
}


/*!
  The object pointers are accompanied by the context name the references have
  for the referred objects. The context names are used as key in \a mapToFill.
*/

void FFaFieldContainer::getReferredObjs(ObjectMap& mapToFill) const
{
  mapToFill.clear();
  for (FFaReferenceBase* ref : myRefTo)
    mapToFill.emplace(ref->getContextName(),ref->getRef());

  for (FFaReferenceListBase* ref : myRefLists)
  {
    ObjectVec objs;
    ref->getBasePtrs(objs);
    for (FFaFieldContainer* obj : objs)
      mapToFill.emplace(ref->getContextName(),obj);
  }
}


/*!
  The object pointers are accompanied by the context name the references have
  for the referring objects. The context names are used as key in \a mapToFill.
*/

void FFaFieldContainer::getReferringObjs(ObjectMap& mapToFill) const
{
  mapToFill.clear();
  for (FFaReferenceBase* ref : myRefBy)
    mapToFill.emplace(ref->getContextName(),ref->getOwnerFieldContainer());
}


/*!
  This method gets the list of objects referring to this object
  with the context \a contextName.
  The list is sorted w.r.t. the ID if \a sortOnId is \e true. This is needed
  when the execution order is of importance. If the list is not sorted,
  the order might be different in two consecutive runs of the same model.
  \note The output array \a vecToFill is not cleared on entry.
*/

void FFaFieldContainer::getReferringObjs(ObjectVec& vecToFill,
					 const std::string& contextName,
					 bool sortOnId) const
{
  for (FFaReferenceBase* ref : myRefBy)
    if (ref->getContextName() == contextName)
      vecToFill.push_back(ref->getOwnerFieldContainer());

  if (sortOnId)
    std::sort(vecToFill.begin(),vecToFill.end(),
              [](FFaFieldContainer* c1, FFaFieldContainer* c2)
              {
                if (c1 && c2)
                {
                  if (c1->getTypeID() == c2->getTypeID())
                    return c1->getResolvedID() < c2->getResolvedID();
                  else
                    return c1->getTypeID() < c2->getTypeID();
                }
                return c2 ? true : false;
              });
}



FFaFieldContainer* FFaFieldContainer::getNext(const std::string& context,
                                              bool getFirst) const
{
  static ReferenceSet::const_iterator it = myRefBy.end();

  if (getFirst)
    it = myRefBy.begin();
  else if (it == myRefBy.end())
    return NULL;
  else
    it++;

  while (it != myRefBy.end())
    if (context.empty() || (*it)->getContextName() == context)
      return (*it)->getOwnerFieldContainer();
    else
      it++;

  return NULL;
}


/*!
  This method is used to make some (or all) of the references pointing
  to this object, point to the object \a replacement instead, if specified.

  Only the references with the supplied \a contextName are changed, unless
  \a contextName is empty (then all references to the object will be changed).
*/

void FFaFieldContainer::releaseReferencesToMe(const std::string& contextName,
					      FFaFieldContainer* replacement)
{
  ReferenceSet refBy = myRefBy;
  for (FFaReferenceBase* ref : refBy)
    if (contextName.empty() || ref->getContextName() == contextName)
    {
      ref->setRef(replacement);
      // Make sure the reference is removed from autosizing lists if set to zero
      if (!replacement)
        ref->zeroOutOrRemoveFromList();
    }
}


void FFaFieldContainer::addField(const std::string& identifier,
				 FFaFieldBase* field)
{
  myFields[FieldContainerDict::instance()->insert(identifier).first] = field;
}


void FFaFieldContainer::removeField(const std::string& identifier)
{
  FDictIt strit = FieldContainerDict::instance()->find(identifier);
  if (strit != FieldContainerDict::instance()->end())
    myFields.erase(strit);
}


bool FFaFieldContainer::readField(const std::string& key, std::istream& is,
				  bool datafieldsOnly)
{
  FDictIt strit = FieldContainerDict::instance()->find(key);
  if (strit == FieldContainerDict::instance()->end()) return false;

  FieldContainerMap::iterator it = myFields.find(strit);
  if (it == myFields.end()) return false;
  if (datafieldsOnly && !it->second->isDataField()) return false;

  is >> *it->second;
  return true;
}


/*!
  This method copies all fields, references and reference lists from the
  supplied FFaFieldContainer object \a *other into \a *this.

  The big assumption is that the field containers are of the same type,
  and thus has the same number of references and in the same order.

  If \a fieldsOnly is \e true only the current field values are copied,
  whereas the default values, references and reference list are not copied.

  If \a unresolve is \e true, the references are copied but left unresolved,
  unless the ID is -1.
*/

bool FFaFieldContainer::copy(const FFaFieldContainer* other,
                             bool fieldsOnly, bool unresolve)
{
  if (!other) return false;

#ifdef FFA_DEBUG
  std::cout <<"\nFFaFieldContainer::copy(): "
            << other->getTypeIDName() <<" ["<< other->getResolvedID() <<"] --> "
            << this->getTypeIDName()  <<" ["<< this->getResolvedID()  <<"]";
#endif

  bool foundAll = true;
  FieldContainerMap::iterator it;
  for (const FieldContainerMap::value_type& field : other->myFields)
    if ((it = myFields.find(field.first)) == myFields.end())
    {
      foundAll = false;
#ifdef FFA_DEBUG
      std::cout <<"\n\t"<< *field.first <<" skipped";
#endif
    }
    else
    {
      it->second->copy(field.second,!fieldsOnly);
#ifdef FFA_DEBUG
      std::cout <<"\n\t"<< *field.first <<" copied";
#endif
    }

#ifdef FFA_DEBUG
  std::cout << std::endl;
#endif

  if (fieldsOnly) return foundAll;

  if (myRefTo.size() != other->myRefTo.size())
    foundAll = false;

  std::list<FFaReferenceBase*>::iterator rIt1;
  std::list<FFaReferenceBase*>::const_iterator rIt2;

  for (rIt1 = myRefTo.begin(), rIt2 = other->myRefTo.begin();
       rIt1 != myRefTo.end() && rIt2 != other->myRefTo.end();
       rIt1++, rIt2++)
    if (*rIt2)
      (*rIt1)->copy(**rIt2, unresolve && (*rIt2)->getRefID() != -1);
    else
      (*rIt1)->setRef(NULL);

  if (myRefLists.size() != other->myRefLists.size())
    foundAll = false;

  std::list<FFaReferenceListBase*>::iterator rlIt1;
  std::list<FFaReferenceListBase*>::const_iterator rlIt2;

  for (rlIt1 = myRefLists.begin(), rlIt2 = other->myRefLists.begin();
       rlIt1 != myRefLists.end() && rlIt2 != other->myRefLists.end();
       rlIt1++, rlIt2++)
    (*rlIt1)->copy(*rlIt2,unresolve);

  return foundAll;
}


/*!
  Only the fields that are present in the supplied container \a *other are reset.
  This method therefore inverts the effect of the copy() method
  (with \a fieldsOnly = \e true).
*/

bool FFaFieldContainer::resetFields(const FFaFieldContainer* other)
{
  if (!other) return false;

  bool foundAll = true;
  FieldContainerMap::iterator it;
  for (const FieldContainerMap::value_type& field : other->myFields)
    if ((it = myFields.find(field.first)) == myFields.end())
      foundAll = false;
    else
      it->second->reset();

  return foundAll;
}


void FFaFieldContainer::addRef(FFaReferenceBase* ref)
{
  if (!ref) return;

  ref->setOwnerFieldContainer(this);
  myRefTo.push_back(ref);
}


void FFaFieldContainer::addRefList(FFaReferenceListBase* refL)
{
  if (!refL) return;

  refL->setOwnerFieldContainer(this);
  myRefLists.push_back(refL);
}


/*!
  This method inserts the supplied reference into the book keeping to notify
  this object that it is pointed to by the reference.
  A part of the core of the FFaReference system.
*/

void FFaFieldContainer::insertInRefBy(FFaReferenceBase* ref)
{
  if (!ref) return;

  if (std::find(myRefBy.begin(),myRefBy.end(),ref) == myRefBy.end()) {
    myRefBy.push_back(ref);
    return;
  }

  std::cerr <<" *** FFaFieldContainer::insertInRefBy(): "
            <<"This has already been notified that the reference (";
  ref->write(std::cerr);
  std::cerr <<") is pointing to this."<< std::endl;
}


/*!
  This method removes the supplied reference from book keeping to notify
  this object that it is no longer pointed to by the reference.
  A part of the core of the FFaReference system.
*/

void FFaFieldContainer::deleteFromRefBy(FFaReferenceBase* ref)
{
  if (!ref) return;

  ReferenceSet::iterator it = std::find(myRefBy.begin(),myRefBy.end(),ref);
  if (it != myRefBy.end()) {
    myRefBy.erase(it);
    return;
  }

  std::cerr <<"FFaFieldContainer::deleteFromRefBy: The reference (";
  ref->write(std::cerr);
  std::cerr <<") said to refer to this has no entry in this."<< std::endl;
}
