// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

/*!
  \file FFaFieldBase.H
  \brief Base class for all model database fields.
*/

#ifndef FFA_FIELD_BASE_H
#define FFA_FIELD_BASE_H

#include <vector>
#include <string>
#include <iostream>

using Strings = std::vector<std::string>; //!< Convenience type alias

class FFaCheckSum;


/*!
  \brief Base class for all model database fields.
*/

class FFaFieldBase
{
  //! \brief Remove implicitly defined copy constructor.
  FFaFieldBase(const FFaFieldBase&) = delete;
  //! \brief Remove implicitly defined assignment operator.
  FFaFieldBase& operator=(const FFaFieldBase&) = delete;

protected:
  //! \brief Empty default constructor.
  FFaFieldBase() {}
  //! \brief Empty destructor.
  virtual ~FFaFieldBase() {}

public:
  //! \brief Copies another database \a field into \a *this.
  virtual FFaFieldBase* copy(const FFaFieldBase* field,
                             bool defaultValues = true) = 0;
  //! \brief Calculates the checksum of the field valiue.
  virtual void calculateChecksum(FFaCheckSum*) const {}
  //! \brief Returns \e true, if this is a numerical data field.
  virtual bool isDataField() const = 0;
  //! \brief Returns \e true, if this field should be printed.
  virtual bool isPrintable() const = 0;
  //! \brief Returns \e true, if the field value equals its default value.
  virtual bool isDefault() const = 0;
  //! \brief Updates the default value of the field.
  virtual void updateDefault() = 0;
  //! \brief Resets the field value to its default value.
  virtual void reset() = 0;

protected:
  //! \brief Writes the field name and value to an output stream.
  virtual void write(std::ostream& os) const = 0;
  //! \brief Reads the field value from an input stream.
  virtual void read(std::istream& is) = 0;

  //! \brief Global output stream operator.
  friend std::ostream& operator<<(std::ostream& os, const FFaFieldBase& field);
  //! \brief Global input stream operator.
  friend std::istream& operator>>(std::istream& is, FFaFieldBase& field);

  // Helper methods used by some FFaField::write/read instances.
  // Defined in the C-file to avoid excessive recompilation when modified.

  //! \brief Writes the string value \a str to the output stream \a os.
  static void writeString(std::ostream& os, const std::string& str,
                          const char* pfx = NULL);
  //! \brief Reads a string value from an input stream.
  static void readString(std::istream& is, std::string& str);
  //! \brief Reads a list of string values from an input stream.
  static void readStrings(std::istream& os, Strings& str);
  //! \brief Reads an integer value from an input stream.
  static bool readInt(std::istream& is, int& val, bool silence = false);
};

#endif
