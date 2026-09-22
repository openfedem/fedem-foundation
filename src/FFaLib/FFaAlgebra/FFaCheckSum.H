// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

/*!
  \file FFaCheckSum.H
  \brief CheckSum calculation.
*/

#ifndef FFA_CHECK_SUM_H
#define FFA_CHECK_SUM_H

#include <vector>
#include <string>
#include <iostream>

class FaVec3;


/*!
  \brief Class for holding and calculation of checksum value of objects.
  \details The checksum calculation is based on a CRC32 (32-bit Cyclic
  Redundancy Check) algorithm.
*/

class FFaCheckSum
{
public:
  //! \brief The default constructor initializes \ref crc32_table.
  FFaCheckSum();
  //! \brief The default copy constructor is deleted.
  FFaCheckSum(const FFaCheckSum&) = delete;
  //! \brief The destructor deletes the heap-allocated member \ref crc32_table.
  ~FFaCheckSum() { delete[] crc32_table; }

  //! \brief Assignment operator.
  FFaCheckSum& operator=(const FFaCheckSum& cs);
  //! \brief Equality operator,
  bool operator==(const FFaCheckSum& cs) const;

  //! \brief Initializes \ref checksum and \ref addval.
  void reset() { checksum = addval = 0; }
  //! \brief Returns the current checksum value.
  unsigned int getCurrent() const { return checksum; }

  //! \brief Adds the checksum of a std::string value to \ref checksum.
  void add(const std::string& e) { for (char c : e) checksum += doCRC(c); }
  //! \brief Adds the checksum of a char value to \ref checksum.
  void add(char e)               { checksum += doCRC(e); }
  //! \brief Adds the checksum of an unsigned char value to \ref checksum.
  void add(unsigned char e)      { checksum += doCRC(e); }
  //! \brief Adds the checksum of a short int value to \ref checksum.
  void add(short int e)          { checksum += doCRC(e); }
  //! \brief Adds the checksum of an unsigned short int value to \ref checksum.
  void add(unsigned short int e) { checksum += doCRC(e); }
  //! \brief Adds the checksum of an int value to \ref checksum.
  void add(int e)                { checksum += doCRC(e); }
  //! \brief Adds the checksum of an unsigned int value to \ref checksum.
  void add(unsigned int e)       { checksum += doCRC(e); }
  //! \brief Adds the checksum of a float value to \ref checksum.
  void add(float e);
  //! \brief Adds the checksum of a double value to \ref checksum.
  void add(double e, int precision = 0);
  //! \brief Adds the checksum of a FaVec3 value to \ref checksum.
  void add(const FaVec3& e, int precision = 0);

  //! \brief Adds the checksum of a std::vector value to \ref checksum.
  template<class T> void add(const std::vector<T>& v)
  {
    for (const T& e : v) this->add(e);
  }

private:
  //! \brief Calculates the checksum value of a variable casted to unsigned int.
  unsigned int doCRC(unsigned int data);

  //! \brief Printing operator.
  friend std::ostream& operator<<(std::ostream& s, const FFaCheckSum& cs);
  //! \brief Reading operator.
  friend std::istream& operator>>(std::istream& s, FFaCheckSum& cs);

  unsigned int* crc32_table; //!< 32-bit Cyclic Redundancy Check hashing values
  unsigned int  checksum; //!< Current checksum value
  unsigned int  addval; //!< Incremented and added to data on each doCRC() call
  bool          isBigEndian; //!< Determines the Endianness of the computer
};

#if FFA_DEBUG > 2
template<> void FFaCheckSum::add(const std::vector<double>& v);
#endif
#endif
