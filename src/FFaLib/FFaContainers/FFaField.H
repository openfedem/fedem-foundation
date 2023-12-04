// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#ifndef FFA_FIELD_H
#define FFA_FIELD_H

#include <set>
#include <sstream>
#include "FFaFieldBase.H"

typedef std::pair<int,double>    IntDouble;
typedef std::vector<IntDouble>   IntDoubles;
typedef std::pair<double,double> Doubles;
typedef std::vector<double>      DoubleVec;
typedef std::pair<int,int>       Ints;
typedef std::vector<int>         IntVec;
typedef std::vector<bool>        BoolVec;
typedef std::set<std::string>    StringSet;


/*!
  \brief Template class for field representation in the Fedem database.

  The class type of the field value must have the logical operators defined
  ( i.e., "==", "<" and ">" ) as well as the assignment operator.
*/

template <typename T1>
class FFaField : public FFaFieldBase
{
  typedef T1 ValueT;

public:
  FFaField() {}
  virtual ~FFaField() {}

  ValueT& operator=(const ValueT& data)
  {
    if (this->isDataField())
      myData = data;

    return myData;
  }

  const ValueT& getValue() const { return myData; }
  ValueT&       getValue()       { return myData; }

  bool setValue(const ValueT& newData, bool checkChanged = true)
  {
    if (checkChanged && myData == newData)
      return false;

    myData = newData;
    return true;
  }

  bool setDefaultValue(const ValueT& defaultVal, bool checkChanged = true)
  {
    if (checkChanged && myDefaultValue == defaultVal)
      return false;

    myDefaultValue = defaultVal;
    return true;
  }

  virtual void reset() { myData = myDefaultValue; }
  virtual void updateDefault() { myDefaultValue = myData; }
  virtual bool isDefault() const { return myData == myDefaultValue; }
  virtual bool isDataField() const { return true; }
  virtual bool isPrintable() const { return true; }

protected:
  virtual FFaFieldBase* copy(const FFaFieldBase* ref, bool defaultValueToo);

  virtual bool equalTo(const std::string& val) const
  {
    ValueT v;
    this->read(val,v);
    return myData == v;
  }

  /* TODO: Implement operator<() for the ValueT class
  virtual bool lessThan(const std::string& val) const
  {
    ValueT v;
    this->read(val,v);
    return myData < v;
  }
  */
  virtual bool lessThan(const std::string&) const { return false; }

  /* TODO: Implement operator>() the ValueT class
  virtual bool greaterThan(const std::string& val) const
  {
    ValueT v;
    this->read(val,v);
    return myData > v;
  }
  */
  virtual bool greaterThan(const std::string&) const { return false; }

  virtual void write(std::ostream& os) const { os << myData; }
  virtual void read(std::istream& is) { this->read(is,myData); }

private:
  void read(std::istream& is, ValueT& v) const { is >> v; }
  void read(const std::string& sval, ValueT& v) const
  {
    std::stringstream s(sval);
    this->read(s,v);
  }

protected:
  ValueT myData;
  ValueT myDefaultValue;
};


/*!
  \brief Subclass used for internal fields that should not be printed.
*/

template <class value>
class FFaNoPrintField : public FFaField<value>
{
public:
  virtual bool isPrintable() const { return false; }

protected:
  virtual void write(std::ostream&) const {}
  virtual void read(std::istream&) {}
};


/*!
  \brief Subclass used for fields to be ignored when the model is copied.
*/

template <class value>
class FFaNoCopyField : public FFaField<value>
{
protected:
  virtual FFaFieldBase* copy(const FFaFieldBase*, bool) { return this; }
};


/*!
  \brief Subclass used for conversion of obsolete fields in old model files.
*/

template <class value>
class FFaObsoleteField : public FFaField<value>
{
public:
  FFaObsoleteField() : isRead(false) {}

  bool wasOnFile() const { return isRead; }

  virtual bool isPrintable() const { return false; }

protected:
  virtual FFaFieldBase* copy(const FFaFieldBase* ref, bool defaultValueToo)
  {
    if (this != ref && this->isDataField())
    {
      const FFaObsoleteField<value>* cRef = dynamic_cast<const FFaObsoleteField<value>*>(ref);
      if (cRef)
      {
        this->isRead = cRef->isRead;
        this->myData = cRef->myData;
        if (defaultValueToo)
          this->myDefaultValue = cRef->myDefaultValue;
      }
      else
        std::cerr <<"FFaObsoleteField<T>::copy(const FFaFieldBase*,bool)"
                  <<" - mismatch in field type"<< std::endl;
    }
    return this;
  }

  virtual void write(std::ostream&) const {}
  virtual void read(std::istream& is)
  {
    FFaField<value>::read(is);
    isRead = true;
  }

private:
  bool isRead;
};


//
//*********************  IMPLEMENTATION OF MEMBERS  ***************************
//

template <typename T1>
FFaFieldBase* FFaField<T1>::copy(const FFaFieldBase* ref, bool defaultValueToo)
{
  if (this != ref && this -> isDataField())
  {
    const FFaField<ValueT>* cRef = dynamic_cast<const FFaField<ValueT>*>(ref);
    if (cRef)
    {
      this->myData = cRef->myData;
      if (defaultValueToo)
        this->myDefaultValue = cRef->myDefaultValue;
    }
    else
      std::cerr <<"FFaField<T>::copy(const FFaFieldBase*,bool)"
                <<" - mismatch in field types" << std::endl;
  }
  return this;
}


template<>
inline FFaField<unsigned long int>::FFaField()
{
  myData = myDefaultValue = 0;
}

template<>
inline FFaField<unsigned int>::FFaField()
{
  myData = myDefaultValue = 0;
}

template<>
inline FFaField<long int>::FFaField()
{
  myData = myDefaultValue = 0;
}

template<>
inline FFaField<int>::FFaField()
{
  myData = myDefaultValue = 0;
}

template<>
inline FFaField<Ints>::FFaField()
{
  myData = myDefaultValue = std::make_pair(0,0);
}

template<>
inline FFaField<float>::FFaField()
{
  myData = myDefaultValue = 0.0f;
}

template<>
inline FFaField<double>::FFaField()
{
  myData = myDefaultValue = 0.0;
}

template<>
inline FFaField<Doubles>::FFaField()
{
  myData = myDefaultValue = std::make_pair(0.0,0.0);
}

template<>
inline FFaField<bool>::FFaField()
{
  myData = myDefaultValue = false;
}


template<>
inline void FFaField<int>::read(std::istream& is, int& v) const
{
  FFaFieldBase::readInt(is,v);
}


template<>
inline void FFaField<float>::write(std::ostream& os) const
{
  std::streamsize oldPrec = os.precision(7);
  os << myData;
  os.precision(oldPrec);
}


template<>
inline void FFaField<Ints>::write(std::ostream& os) const
{
  os << myData.first <<" "<< myData.second;
}

template<>
inline void FFaField<Ints>::read(std::istream& is, Ints& v) const
{
  if (FFaFieldBase::readInt(is,v.first))
    FFaFieldBase::readInt(is,v.second);
}


template<>
inline bool FFaField<std::string>::isPrintable() const
{
  return !myData.empty();
}

template<>
inline void FFaField<std::string>::write(std::ostream& os) const
{
  FFaFieldBase::writeString(os,myData);
}

template<>
inline void FFaField<std::string>::read(std::istream& is, std::string& v) const
{
  v.clear();
  FFaFieldBase::readString(is,v);
}


template<>
inline bool FFaField<Strings>::isPrintable() const
{
  return !myData.empty();
}

template<>
inline void FFaField<Strings>::write(std::ostream& os) const
{
  FFaFieldBase::writeString(os,myData.front());

  for (size_t i = 1; i < myData.size(); i++)
    FFaFieldBase::writeString(os, myData[i], myData[i].empty() ? " " : "\n\t");
}

template<>
inline void FFaField<Strings>::read(std::istream& is, Strings& v) const
{
  v.clear();
  FFaFieldBase::readStrings(is,v);
}


template<>
inline bool FFaField<StringSet>::isPrintable() const
{
  return !myData.empty();
}

template<>
inline void FFaField<StringSet>::write(std::ostream& os) const
{
  StringSet::const_iterator it = myData.begin();
  FFaFieldBase::writeString(os,*it);

  for (++it; it != myData.end(); ++it)
    FFaFieldBase::writeString(os, *it, it->empty() ? " " : "\n\t");
}

template<>
inline void FFaField<StringSet>::read(std::istream& is, StringSet& v) const
{
  Strings tmp;
  FFaFieldBase::readStrings(is,tmp);

  v.clear();
  v.insert(tmp.begin(),tmp.end());
}


template<>
inline void FFaField<Doubles>::write(std::ostream& os) const
{
  os << myData.first <<" "<< myData.second;
}

template<>
inline void FFaField<Doubles>::read(std::istream& is, Doubles& v) const
{
  if (is) is >> v.first;
  if (is) is >> v.second;
}


template<>
inline bool FFaField<DoubleVec>::isPrintable() const
{
  return !myData.empty();
}

template<>
inline void FFaField<DoubleVec>::write(std::ostream& os) const
{
  for (size_t i = 0; i < myData.size(); i++)
  {
    // write up to 6 numbers on a row
    if (i) os << (i%6 ? " " : "\n\t");
    os << myData[i];
  }
}

template<>
inline void FFaField<DoubleVec>::read(std::istream& is, DoubleVec& v) const
{
  v.clear();

  double d;
  while (is)
  {
    is >> d;
    if (is)
      v.push_back(d);
  }

  is.clear();
}


template <>
inline bool FFaField<IntVec>::isPrintable() const
{
  return !myData.empty();
}

template<>
inline void FFaField<IntVec>::write(std::ostream& os) const
{
  for (size_t i = 0; i < myData.size(); i++)
  {
    // write up to 10 numbers on a row
    if (i) os << (i%10 ? " " : "\n\t");
    os << myData[i];
  }
}

template<>
inline void FFaField<IntVec>::read(std::istream& is, IntVec& v) const
{
  v.clear();

  int i;
  while (is)
    if (FFaFieldBase::readInt(is,i,true))
      v.push_back(i);

  is.clear();
}


template<>
inline bool FFaField<BoolVec>::isPrintable() const
{
  return !myData.empty();
}

template<>
inline void FFaField<BoolVec>::write(std::ostream& os) const
{
  for (size_t i = 0; i < myData.size(); i++)
  {
    // write up to 20 values on a row
    if (i) os << (i%20 ? " " : "\n\t");
    os << (myData[i] ? 1 : 0);
  }
}

template<>
inline void FFaField<BoolVec>::read(std::istream& is, BoolVec& v) const
{
  v.clear();

  int i;
  while (is)
  {
    is >> i;
    if (is)
      v.push_back(i > 0 ? true : false);
  }

  is.clear();
}


template<>
inline bool FFaField<IntDoubles>::isPrintable() const
{
  return !myData.empty();
}

template<>
inline void FFaField<IntDoubles>::write(std::ostream& os) const
{
  for (size_t i = 0; i < myData.size(); i++)
  {
    // write up to 6 pairs on a row
    if (i) os << (i%6 ? "  " : "\n\t");
    os << myData[i].first <<" "<< myData[i].second;
  }
}

template<>
inline void FFaField<IntDoubles>::read(std::istream& is, IntDoubles& v) const
{
  v.clear();

  int i;
  double d;
  while (is)
    if (FFaFieldBase::readInt(is,i,true))
    {
      is >> d;
      if (is)
        v.push_back(std::make_pair(i,d));
    }

  is.clear();
}


template<>
inline void FFaField<bool>::write(std::ostream& os) const
{
  os << (myData ? "true" : "false");
}

template<>
inline void FFaField<bool>::read(std::istream& is, bool& v) const
{
  char c;

  // skip whitespaces
  while (is.get(c) && isspace(c));

  // check if we have a numerical bool representation
  if (isdigit(c))
    v = (c == '0' ? false : true);
  else
  {
    // we have alpha or end sign
    std::string tmpStr;
    while (is && isalpha(c))
    {
      tmpStr += toupper(c);
      is.get(c);
    }
    is.putback(c); // putting back end sign, space or whatever

    v = (tmpStr == "YES" || tmpStr == "TRUE") ? true : false;
  }
}

#endif
