// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

/*!
  \file FFaFieldContainer.H
  \brief Base class for the FEDEM model database structure.
*/

#ifndef FFA_FIELD_CONTAINER_H
#define FFA_FIELD_CONTAINER_H

#include <map>
#include <set>
#include <list>
#include <vector>
#include <string>
#include "FFaLib/FFaPatterns/FFaSingelton.H"
#include "FFaLib/FFaTypeCheck/FFaTypeCheck.H"
#include "FFaLib/FFaDynCalls/FFaDynCB.H"

class FFaFieldBase;
class FFaReferenceBase;
class FFaReferenceListBase;


/*!
  \brief Base class for the FEDEM model database structure.

  \details This class contains the core of the model database structure.
  It manages FFaField, FFaReference and FFaReferenceList members in derived
  subclasses, and provides automated I/O, single and multiple guarded pointers.
*/

class FFaFieldContainer
{
  Fmd_BASE_HEADER_INIT();

  //! Convenience type alias
  using IntVec    = std::vector<int>;
  //! Convenience type alias
  using FieldMap  = std::map<std::string,FFaFieldBase*>;
  //! Convenience type alias
  using ObjectMap = std::multimap<std::string,FFaFieldContainer*>;
  //! Convenience type alias
  using ObjectVec = std::vector<FFaFieldContainer*>;

public:
  //! \brief The default constructor does nothing.
  FFaFieldContainer();
  //! \brief The destructor clears the references to *this in #myRefBy.
  virtual ~FFaFieldContainer();

  //! \brief Deletes *this assuming it is heap-allocated.
  bool erase();

  // Field browsing

  //! \brief Builds a map of all fields in this FFaFieldContainer.
  void getFields(FieldMap& mapToFill) const;
  //! \brief Returns the named field in this FFaFieldContainer.
  FFaFieldBase* getField(const std::string& fieldName) const;

  // Topology browsing

  //! \brief Builds a map of all FFaFieldContainer objects referred to by *this.
  void getReferredObjs (ObjectMap& mapToFill) const;
  //! \brief Builds a map of all FFaFieldContainer objects referring to *this.
  void getReferringObjs(ObjectMap& mapToFill) const;
  //! \brief Builds a list of FFaFieldContainer objects referring to *this.
  void getReferringObjs(ObjectVec& vecToFill,
                        const std::string& contextName,
                        bool sortOnId = false) const;

  //! \brief Builds a list of objects of class \a T referring to *this.
  template<class T>
  void getReferringObjs(std::vector<T*>& vecToFill,
                        const std::string& context = "",
                        bool sortOnId = false) const
  {
    T* ptr;
    if (sortOnId)
    {
      // Return the objects in sorted order w.r.t. their user ID
      ObjectVec allRefs;
      this->getReferringObjs(allRefs,context,sortOnId);
      for (FFaFieldContainer* ref : allRefs)
        if ((ptr = dynamic_cast<T*>(ref)))
          vecToFill.push_back(ptr);
    }
    else
    {
      // Return the objects in arbitrary order
      // (might be different in two runs of the same problem).
      FFaFieldContainer* ref = this->getNext(context,true);
      for (ptr = NULL; ref; ref = this->getNext(context))
        if ((ptr = dynamic_cast<T*>(ref)))
          vecToFill.push_back(ptr);
    }
  }

  //! \brief Checks if there are any references of type \a T to this object.
  //! \details The pointer to the first object, if any, is returned in \a ptr.
  template<class T>
  bool hasReferringObjs(T*& ptr, const std::string& context = "") const
  {
    FFaFieldContainer* ref = this->getNext(context,true);
    for (ptr = NULL; ref; ref = this->getNext(context))
      if ((ptr = dynamic_cast<T*>(ref)))
        return true;
    return false;
  }

  //! \brief Directly removes references to this object.
  void releaseReferencesToMe(const std::string& contextName = "",
                             FFaFieldContainer* replacement = NULL);

  //! Convenience alias for resolve call-backs.
  using FindCB = FFaDynCB4<FFaFieldContainer*&,int,int,const IntVec&>;

  //! \brief Resolves all FFaReferences and FFaReferenceLists in this object.
  //! \sa FFaReferenceBase::resolve
  //! \sa FFaReferenceListBase::resolve
  void resolve(FindCB& findCB);
  //! \brief Unresolves all FFaReferences and FFaReferenceLists in this object.
  //! \sa FFaReferenceBase::unresolve
  //! \sa FFaReferenceListBase::unresolve
  void unresolve();

  //! \brief Initializes the fields after resolve.
  virtual void initAfterResolve() {}

  //! \brief Returns the user Id of *this, for use in I/O and resolving.
  virtual int getResolvedID() const { return -1; }
  //! \brief Gets the assembly path of *this, for use in I/O and resolving.
  virtual void getResolvedAssemblyID(IntVec&) const {}

  //! \brief Updates the assembly ID in the references in this object.
  void updateReferences(int oldAssId, int newAssId);
  //! \brief Updates the assembly ID in the references in this object.
  void updateReferences(const IntVec& oldAssId, const IntVec& newAssID);

  //! \brief Reads the value of a field named \a key from an input stream.
  bool readField(const std::string& key, std::istream& is,
		 bool datafieldsOnly = false);

  //! \brief Copies all fields and refernces from another object into \a *this.
  bool copy(const FFaFieldContainer* other,
	    bool fieldsOnly = false, bool unresolve = false);
  //! \brief Copies all field values from another object into \a *this.
  bool copyFields(const FFaFieldContainer* obj) { return this->copy(obj,true); }

  //! \brief Resets fields to their default value.
  bool resetFields(const FFaFieldContainer* obj);

protected:
  virtual bool eraseOptions() { return true; }

  //! \brief Adds a reference to the internal book keeping of this object.
  void addRef(FFaReferenceBase* ref);
  //! \brief Adds a reference list to the internal book keeping of this object.
  void addRefList(FFaReferenceListBase* list);

  //! \brief Adds a field to the internal book keeping of this object.
  void addField(const std::string& identifier, FFaFieldBase* field);
  //! \brief Removes the field from the internal book keeping of this object.
  void removeField(const std::string& identifier);

  // The read/writable fields

  //! Convenience type alias, a set of text strings used as a dictionay
  using FDict   = std::set<std::string>;
  //! Convenience type alias, a const_iterator pointing into a dictionary
  using FDictIt = FDict::const_iterator;

  //! \brief Functor for comparison of dictionary entries.
  struct FDictLess
  {
    //! \brief Returns \e true is \a *s1 is lexicographically less than \a *s2.
    bool operator()(FDictIt s1, FDictIt s2) const { return *s1 < *s2; }
  };
  //! Convenience type alias, a field container map
  using FieldContainerMap  = std::map<FDictIt,FFaFieldBase*,FDictLess>;
  //! Convenience type alias, a heap-allocated dictionary singelton
  using FieldContainerDict = FFaSingelton<FDict,FFaFieldContainer>;

  FieldContainerMap myFields; //!< All data fields of this object

public:
  //! \brief Removes the heap-allocated field name dictionary.
  static void removeDictInstance() { FieldContainerDict::removeInstance(); }

private:
  friend class FFaReferenceBase;

  //! \brief Marks this object as referred to by another object.
  //! \details Used by FFaReferenceBase only.
  void insertInRefBy  (FFaReferenceBase* ref);
  //! \brief Unmarks this object as no longer referred to by another object.
  //! \details Used by FFaReferenceBase only.
  void deleteFromRefBy(FFaReferenceBase* ref);

  //! \brief Helper for template functions to loop over all objects in #myRefBy.
  FFaFieldContainer* getNext(const std::string& context,
                             bool getFirst = false) const;

  // The internal book keeping of who is pointing to whom

  //! Convenience type alias
  using ReferenceSet = std::vector<FFaReferenceBase*>;
  //! List of all objects referring to *this
  ReferenceSet                     myRefBy;
  //! List of all objects *this is referring to
  std::list<FFaReferenceBase*>     myRefTo;
  //! List of all listed objects *this is referring to
  std::list<FFaReferenceListBase*> myRefLists;
};


//! \brief Adds a named field to \a *this with specified default value.
#define FFA_FIELD_INIT(field, defaultValue, identifier)			\
  this->addField(identifier, &field);					\
  field.setValue(defaultValue,false);					\
  field.setDefaultValue(field.getValue(),false)

//! \brief Adds a names field to \a *obj with specified default value.
#define FFA_OBSOLETE_FIELD_INIT(field, defaultValue, identifier, obj)	\
  obj->addField(identifier, &field);					\
  field.setValue(defaultValue,false);					\
  field.setDefaultValue(field.getValue(),false)

//! \brief Adds a named field to \a *this.
#define FFA_FIELD_DEFAULT_INIT(field, identifier)	\
  this->addField(identifier, &field)

//! \brief Adds a named field to \a *obj.
#define FFA_OBSOLETE_FIELD_DEFAULT_INIT(field, identifier, obj)	\
  obj->addField(identifier, &field)

//! \brief Adds a reference to \a *this.
#define FFA_REFERENCE_INIT(reference)		\
  this->addRef(&reference);			\
  reference.setContextName(#reference)

//! \brief Adds a reference list to \a *this.
#define FFA_REFERENCELIST_INIT(referenceList)	\
  this->addRefList(&referenceList);		\
  referenceList.setContextName(#referenceList)

//! \brief Adds a named reference field to \a *this.
#define FFA_REFERENCE_FIELD_INIT(field, reference, identifier) \
  FFA_FIELD_INIT(field, &reference, identifier);	       \
  FFA_REFERENCE_INIT(reference)

//! \brief Adds a named reference list field to \a *this.
#define FFA_REFERENCELIST_FIELD_INIT(field, referenceList, identifier) \
  FFA_FIELD_INIT(field, &referenceList, identifier);		       \
  FFA_REFERENCELIST_INIT(referenceList)

//! \brief Removes a named field from \a *obj.
#define FFA_OBSOLETE_FIELD_REMOVE(identifier, obj)	\
  obj->removeField(identifier)

#endif
