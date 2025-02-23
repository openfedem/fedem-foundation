// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#ifndef FFA_FIELD_BASE_H
#define FFA_FIELD_BASE_H

#include "FFaQueryBase.H"
#include <vector>
#include <string>
#include <iostream>

typedef std::vector<std::string> Strings;

class FFaCheckSum;


class FFaFieldBase : public FFaQueryBase
{
  // Remove implicitly defined copy constructor and assignment operator
  FFaFieldBase(const FFaFieldBase&) = delete;
  FFaFieldBase& operator=(const FFaFieldBase&) = delete;

public:
  FFaFieldBase() {}
  virtual ~FFaFieldBase() {}

  virtual FFaFieldBase* copy(const FFaFieldBase*, bool = true) = 0;
  virtual void calculateChecksum(FFaCheckSum*) const {}
  virtual bool isDataField() const = 0;
  virtual bool isPrintable() const = 0;
  virtual bool isDefault() const = 0;
  virtual void updateDefault() = 0;
  virtual void reset() = 0;

protected:
  virtual void write(std::ostream&) const = 0;
  virtual void read(std::istream&) = 0;

  friend std::ostream& operator<<(std::ostream&, const FFaFieldBase&);
  friend std::istream& operator>>(std::istream&, FFaFieldBase&);

  // Helper methods used by some FFaField::write/read instances.
  // Defined in the C-file to avoid excessive recompilation when modified.
  static void writeString(std::ostream&, const std::string& str,
                          const char* pfx = NULL);
  static void readString(std::istream&, std::string& str);
  static void readStrings(std::istream&, Strings& str);
  static bool readInt(std::istream&, int& val, bool silence = false);
};

#endif
