// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

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


template <class T, class Key = std::string, class ArgType = int>
class FFaGenericFactory : public FFaSingelton< FFaGenericFactory<T,Key,ArgType> >
{
  using CreatorCB   = FFaDynCB2<ArgType,T*&>;
  using CreatorType = std::map<Key,CreatorCB>;

public:
  FFaGenericFactory() {}
  virtual ~FFaGenericFactory() {}

  bool registerCreator(const Key& key, const CreatorCB& creator)
  {
    return myCreatorMap.insert({ key, creator }).second;
  }

  T* create(const Key& key, ArgType id)
  {
    T* retVal = NULL;
    typename CreatorType::iterator creator = myCreatorMap.find(key);
    if (creator != myCreatorMap.end())
      creator->second.invoke(id,retVal);
#ifdef FFA_DEBUG
    else
      std::cerr <<"\n *** FFaGenericFactory: No creator defined for "
                << key << std::endl;
#endif
    return retVal;
  }

  void getKeys(std::vector<Key>& keys) const
  {
    keys.clear();
    keys.reserve(myCreatorMap.size());
    for (const typename CreatorType::value_type& creator : myCreatorMap)
      keys.push_back(creator.first);
  }

  void clear() { myCreatorMap.clear(); this->removeInstance(); }

private:
  CreatorType myCreatorMap;
};

#endif
