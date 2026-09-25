// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

/*!
  \file FFaGenericFactory.H
  \brief General object creation factory.
*/

#ifndef FFA_GENERIC_FACTORY_H
#define FFA_GENERIC_FACTORY_H

#include <string>
#include <vector>
#include <map>
#ifdef FFA_DEBUG
#include <iostream>
#endif

#include "FFaLib/FFaPatterns/FFaSingelton.H"
#include "FFaLib/FFaDynCalls/FFaDynCB.H"


/*!
  \brief Template singelton class defining a general object creation factory.
  \details This class is used for managing methods for creating instances of
  a varity of classes, such as finite element types, etc. Call back methods
  creating an instance of a specific type are stored as function pointers in
  a map with the object type name as lookup key.
*/

template <class T, class Key = std::string, class ArgType = int>
class FFaGenericFactory : public FFaSingelton< FFaGenericFactory<T,Key,ArgType> >
{
  //! Type alias for object creation callback
  using CreatorCB  = FFaDynCB2<ArgType,T*&>;
  //! Key --> creator callback mapping
  using CreatorMap = std::map<Key,CreatorCB>;

public:
  //! \brief Registers a new object creation method on \a key.
  bool registerCreator(const Key& key, const CreatorCB& creator)
  {
    return myCreatorMap.emplace(key,creator).second;
  }

  //! \brief Invokes the object creation method for given \a key.
  T* create(const Key& key, ArgType id)
  {
    T* retVal = NULL;
    typename CreatorMap::iterator creator = myCreatorMap.find(key);
    if (creator != myCreatorMap.end())
      creator->second.invoke(id,retVal);
#ifdef FFA_DEBUG
    else
      std::cerr <<"\n *** FFaGenericFactory: No creator defined for "
                << key << std::endl;
#endif
    return retVal;
  }

  //! \brief Gets a list of all registered object creator keys.
  void getKeys(std::vector<Key>& keys) const
  {
    keys.clear();
    keys.reserve(myCreatorMap.size());
    for (const typename CreatorMap::value_type& creator : myCreatorMap)
      keys.push_back(creator.first);
  }

  //! \brief Clears the object creator map \ref myCreatorMap.
  void clear() { myCreatorMap.clear(); this->removeInstance(); }

private:
  CreatorMap myCreatorMap; //!< Object creator callbacks container
};

#endif
