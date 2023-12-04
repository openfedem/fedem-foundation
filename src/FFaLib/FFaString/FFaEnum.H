// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

/*!
  \file FFaEnum.H
  \brief Enum variables with text representations.
*/

#ifndef FFA_ENUM_H
#define FFA_ENUM_H

#include <vector>
#include <utility>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <iostream>


/*!
  \brief Class to be used as a enum variable with text representations.

  \details The class contains stream operators for input and output as text,
  and overloaded assignment operators such that its value can be assigned also
  from the text representation.

  Usage:
  \code
  class ClassWithEnum
  {
  public:
    ClassWithEnum() {}

    enum SomeEnumType {
      AONE,
      ATWO,
      ATHREE
    };

    FFaEnumMapping(SomeEnumType) {
      FFaEnumEntry(AONE,  "AONE");
      FFaEnumEntry(ATWO,  "ATWO");
      FFaEnumEntry(ATHREE,"ATHREE");
      FFaEnumEntryEnd;
    };

    // Use either as a normal enum:
    FFaEnum<SomeEnumType,SomeEnumTypeMapping> myEnum;
    // Or within a field:
    FFaField<SomeEnumTypeEnum> myEnumField;
  };

  \endcode
  As you can see, the initialization of the text to enum mappings are to be set
  up using the macros FFaEnumMapping(), FFaEnumEntry() and FFaEnumEntryEnd().
  The FFaEnum class supports all kinds of assigning, so it can be used as a
  normal enum variable.
  \code
   ...
   ClassWithEnum::ClassWithEnum()
   {
     ...
     FFA_FIELD_INIT(myEnumField, AONE, "ENUM_FIELD_NAME");
     ...
   }
   ...
   ClassWithEnum a;
   a.myEnum = ClassWithEnum::ATHREE;
   ClassWithEnum b;
   b.myEnum = a.myEnum;
   a.myEnum = "ATWO";
   ClassWithEnum::SomeEnumType en = a.myEnum;
   int v = a.myEnum;
   std::cout << a.myEnum << std::endl;
   std::cin >> a.myEnum;

   if (b.myEnum == a.myEnum) std::cout <<"a and b are equal\n";
   if (b.myEnum != a.myEnum) std::cout <<"a and b are not equal\n";
  \endcode
*/

template<class EnumType, class ETMapping> class FFaEnum
{
public:
  //! \brief Default constructor.
  FFaEnum(EnumType val = EnumType(0)) : myValue(val) {}
  //! \brief Constructor obtaining its value from a string.
  FFaEnum(const char* val) { *this = val; }

  //! \brief Assignment operator.
  FFaEnum<EnumType,ETMapping>& operator=(const FFaEnum<EnumType,ETMapping>& val)
  {
    if (this != &val)
      this->myValue = val.myValue;
    return *this;
  }

  //! \brief Overloaded assignment operator.
  FFaEnum<EnumType,ETMapping>& operator=(const EnumType& val)
  {
    this->myValue = val;
    return *this;
  }

  //! \brief Overloaded assignment operator.
  FFaEnum<EnumType,ETMapping>& operator=(const int& val)
  {
    this->myValue = EnumType(val);
    return *this;
  }

  //! \brief Overloaded assignment operator.
  FFaEnum<EnumType,ETMapping>& operator=(const char* text)
  {
    if (!text || strlen(text) < 1)
      return *this; // Empty string, don't touch the value

    // Ignore leading and trailing whitespace, if any
    size_t istart = 0, iend = strlen(text)-1;
    while (isspace(text[istart]))
      if (++istart > iend)
      {
        // Blank string, treat as zero
        this->myValue = EnumType(0);
        return *this;
      }

    while (iend > istart && isspace(text[iend]))
      --iend;

    for (const std::pair<EnumType,const char*>& ep : ETMapping::map())
      if (strncmp(ep.second,text+istart,iend-istart+1) == 0)
      {
        // Found a matching string value
        this->myValue = ep.first;
        return *this;
      }

    // No match, check if the string is some integer value
    for (size_t i = istart; i <= iend; i++)
      if (!isdigit(text[i]))
        return *this;

    // Try to convert the integer value to the enum value
    this->myValue = EnumType(atoi(text+istart));
    return *this;
  }

  //! \brief Returns the actual enum value.
  operator EnumType() const { return this->myValue; }

  //! \brief Returns the text representation of the enum value.
  const char* getText() const
  {
    for (const std::pair<EnumType,const char*>& ep : ETMapping::map())
      if (ep.first == this->myValue)
        return ep.second;

    // Fall-back in case of an enum value without matching string value
    static char text[16];
    snprintf(text,16,"%d",this->myValue);
    return text;
  }

  //! \brief Equality operator.
  bool operator==(const FFaEnum<EnumType,ETMapping>& e) const { return this->myValue == e.myValue; }
  //! \brief Overloaded equality operator.
  bool operator==(const EnumType&                    e) const { return this->myValue == e; }

  //! \brief Unequality operator.
  bool operator!=(const FFaEnum<EnumType,ETMapping>& e) const { return this->myValue != e.myValue; }
  //! \brief Overloaded unequality operator.
  bool operator!=(const EnumType&                    e) const { return this->myValue != e; }

private:
  EnumType myValue; //!< The actual enum value
};


//! \brief Global output stream operator.
template<class EnumType, class ETMapping>
std::ostream& operator<<(std::ostream& s, const FFaEnum<EnumType,ETMapping>& e)
{
  return s << e.getText();
}

//! \brief Global input stream operator.
template<class EnumType, class ETMapping>
std::istream& operator>>(std::istream& s, FFaEnum<EnumType,ETMapping>& e)
{
  char enumText[128];
  size_t iend = 0;
  while (iend < 127 && s.get(enumText[iend]))
    if (!isspace(enumText[iend]))
      ++iend;
    else if (iend > 0)
      break;

  if (iend == 0)
    return s; // Empty string, don't touch anything
  else if (enumText[0] == '"' && enumText[iend-1])
  {
    if (iend < 3) return s; // Empty string, don't touch anything
    // Remove the embedding '"'-characters
    enumText[0] = ' ';
    --iend;
  }

  enumText[iend] = '\0';
  e = enumText;
  s.clear();
  return s;
}


//! \brief Defines the start of an enum mapping definition.
#define FFaEnumMapping(EnumType)					\
  class EnumType##Mapping;						\
  typedef FFaEnum<EnumType,EnumType##Mapping> EnumType##Enum;		\
  class EnumType##Mapping						\
  {									\
    typedef std::pair<EnumType,const char*> EnumType##Pair;		\
  public:								\
    static const std::vector<EnumType##Pair> map()			\
    {									\
      std::vector<EnumType##Pair> enumMap;				\
      if (enumMap.empty())

//! \brief Adds one entry to the current enum mapping definition.
#define FFaEnumEntry(EnumValue,EnumText)			\
  enumMap.push_back(std::make_pair(EnumValue,EnumText));

//! \brief Defines the end of an enum mapping definition.
#define FFaEnumEntryEnd				\
  } return enumMap; }

#endif
