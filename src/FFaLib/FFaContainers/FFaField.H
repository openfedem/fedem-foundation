// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

/*!
  \file FFaField.H
  \brief Template class for field representation in the FEDEM model database.
*/

#ifndef FFA_FIELD_H
#define FFA_FIELD_H

#include <set>
#include <sstream>
#include "FFaFieldBase.H"

using IntDouble  = std::pair<int,double>;    //!< Convenience type alias
using IntDoubles = std::vector<IntDouble>;   //!< Convenience type alias
using Doubles    = std::pair<double,double>; //!< Convenience type alias
using DoubleVec  = std::vector<double>;      //!< Convenience type alias
using Ints       = std::pair<int,int>;       //!< Convenience type alias
using IntVec     = std::vector<int>;         //!< Convenience type alias
using BoolVec    = std::vector<bool>;        //!< Convenience type alias
using StringSet  = std::set<std::string>;    //!< Convenience type alias


/*!
  \brief Template class for field representation in the FEDEM model database.

  \details The class type of the field value must have the logical operators
  defined (i.e., "==", "<" and ">") as well as the assignment operator.
*/

template <typename ValueT>
class FFaField : public FFaFieldBase
{
public:
  //! \brief Empty default constructor.
  FFaField() {}
  //! \brief Empty destructor.
  virtual ~FFaField() {}

  //! \brief Assignment operator.
  ValueT& operator=(const ValueT& data)
  {
    if (this->isDataField())
      myData = data;

    return myData;
  }

  //! \brief Returns a const reference to #myData.
  const ValueT& getValue() const { return myData; }
  //! \brief Returns a reference to #myData (for update).
  ValueT& getValue() { return myData; }

  //! \brief Assigns a new value to the field.
  //! \param[in] newData The new field value to be assigned
  //! \param[in] checkChanged If \e true,
  //! notify if \a newData differs from #myData
  //! \return \e true if \a newData is different from #myData
  //! when \a checkChanges is \e true
  bool setValue(const ValueT& newData, bool checkChanged = true)
  {
    if (checkChanged && myData == newData)
      return false;

    myData = newData;
    return true;
  }

  //! \brief Assigns the default value of the field.
  //! \param[in] defaultVal The new default field value to be assigned
  //! \param[in] checkChanged If \e true,
  //!  notify if \a defaultVal differs from #myDefaultValue
  //! \return \e true if \a defaultVal is different from #myDefaultValue
  //! when \a checkChanges is \e true
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
  virtual FFaFieldBase* copy(const FFaFieldBase* field, bool defaultValueToo)
  {
    if (this == field || !this->isDataField())
      return this;

    const FFaField<ValueT>* ref = dynamic_cast<const FFaField<ValueT>*>(field);
    if (ref)
    {
      this->myData = ref->myData;
      if (defaultValueToo)
        this->myDefaultValue = ref->myDefaultValue;
    }
    else
      std::cerr <<"FFaField<T>::copy(const FFaFieldBase*,bool)"
                <<" - mismatch in field types" << std::endl;

    return this;
  }

  virtual void write(std::ostream& os) const { os << myData; }
  virtual void read(std::istream& is) { this->read(is,myData); }

private:
  //! \brief Reads the field value from an input stream.
  void read(std::istream& is, ValueT& v) const { is >> v; }
  //! \brief Reads the field value from a std::string.
  void read(const std::string& sval, ValueT& v) const
  {
    std::stringstream s(sval);
    this->read(s,v);
  }

protected:
  ValueT myData;         //!< Current field value
  ValueT myDefaultValue; //!< Default field value
};


/*!
  \brief Subclass used for internal fields that should not be printed.
*/

template <class valueT>
class FFaNoPrintField : public FFaField<valueT>
{
public:
  //! \brief Returns \e false, since this field should never be printed.
  virtual bool isPrintable() const { return false; }

protected:
  //! \brief Overridden to do nothing.
  virtual void write(std::ostream&) const {}
  //! \brief Overridden to do nothing.
  virtual void read(std::istream&) {}
};


/*!
  \brief Subclass used for fields to be ignored when the model is copied.
*/

template <class valueT>
class FFaNoCopyField : public FFaField<valueT>
{
protected:
  //! \brief Overridden to return \a *this instead of a copy.
  virtual FFaFieldBase* copy(const FFaFieldBase*, bool) { return this; }
};


/*!
  \brief Subclass used for conversion of obsolete fields in old model files.
*/

template <class T>
class FFaObsoleteField : public FFaField<T>
{
public:
  //! \brief Default constructor.
  FFaObsoleteField() : isRead(false) {}

  //! \brief Returns \e true if this field was found in the model field.
  bool wasOnFile() const { return isRead; }

  //! \brief Returns \e false, since this field should never be printed.
  virtual bool isPrintable() const { return false; }

protected:
  virtual FFaFieldBase* copy(const FFaFieldBase* field, bool defaultValueToo)
  {
    if (this == field || !this->isDataField())
      return this;

    const FFaObsoleteField<T>* ref = dynamic_cast<const FFaObsoleteField<T>*>(field);
    if (ref)
    {
      this->isRead = ref->isRead;
      this->myData = ref->myData;
      if (defaultValueToo)
        this->myDefaultValue = ref->myDefaultValue;
    }
    else
      std::cerr <<"FFaObsoleteField<T>::copy(const FFaFieldBase*,bool)"
                <<" - mismatch in field type"<< std::endl;

    return this;
  }

  //! \brief Overridden to do nothing.
  virtual void write(std::ostream&) const {}
  //! \brief Overridden to set the #isRead flag.
  virtual void read(std::istream& is)
  {
    this->FFaField<T>::read(is);
    isRead = true;
  }

private:
  bool isRead; //!< Flag telling if this field was found in the model file
};


//! \cond TEMPLATE_SPECIALIZATIONS

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
  myData = myDefaultValue = { 0, 0 };
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
  myData = myDefaultValue = { 0.0, 0.0 };
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


template<>
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
        v.emplace_back(i,d);
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

//! \endcond
#endif
