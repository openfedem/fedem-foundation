// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

/*!
  \file FFaUnitCalculator.H
  \brief Conversion of units for physical property parameters.
*/

#ifndef FFA_UNIT_CALCULATOR_H
#define FFA_UNIT_CALCULATOR_H

#include <string>
#include <vector>
#include <map>

#include "FFaLib/FFaPatterns/FFaSingelton.H"


/*!
  \brief Class holding a set of unit conversions for physical properties.
*/

class FFaUnitCalculator
{
  //! \brief Struct representing a single unit conversion.
  struct SingleUnit
  {
    double factor = 1.0;  //!< Unit conversion factor
    std::string origUnit; //!< Original unit name
    std::string convUnit; //!< Converted-to name
  };

  //! \brief Convenience type alias.
  using SingleUnitMap  = std::map<std::string,SingleUnit>;
  //! \brief Convenience type alias.
  using SingleUnitIter = SingleUnitMap::const_iterator;

public:
  //! \brief Default constructor.
  explicit FFaUnitCalculator(const char* n = NULL)
    : name(n ? n : ""), origGroup("SI"), convGroup("SI") {}

  //! \brief Returns \e true if \a *this is valid, i.e., \ref name is defined.
  bool isValid() const { return !name.empty(); }

  //! \brief Equality operator.
  bool operator==(const FFaUnitCalculator& cal) const;

  //! \brief Returns the name of the unit calculator.
  const std::string& getName()      const { return name; }
  //! \brief Returns the name of the original unit set.
  const std::string& getOrigUnits() const { return origGroup; }
  //! \brief Returns the name of the converted-to unit set.
  const std::string& getConvUnits() const { return convGroup; }

  //! \brief Converts a named physical property.
  template <class T> bool convert(T& value, const std::string& aName) const
  {
    SingleUnitIter it = myConvFactors.find(aName);
    if (it == myConvFactors.end()) return false;

    value *= it->second.factor;
    return true;
  }

  //! \brief Conversion with rounding to \a prec significant digits.
  bool convert(double& value, const std::string& aName, int prec) const;

  //! \brief Reversed conversion for a named physical property.
  template <class T> bool inverse(T& value, const std::string& aName) const
  {
    SingleUnitIter it = myConvFactors.find(aName);
    if (it == myConvFactors.end()) return false;

    value /= it->second.factor;
    return true;
  }

  //! \brief Returns the conversion factor for a named physical property.
  double convFactor(const std::string& aName) const
  {
    SingleUnitIter it = myConvFactors.find(aName);
    return it == myConvFactors.end() ?  0.0 : it->second.factor;
  }

  //! \brief Returns the original unit name for a named physical property.
  std::string origUnitName(const std::string& aName) const
  {
    SingleUnitIter it = myConvFactors.find(aName);
    return it == myConvFactors.end() ? std::string("") : it->second.origUnit;
  }

  //! \brief Returns the converted-to unit name for a named physical property.
  std::string convUnitName(const std::string& aName) const
  {
    SingleUnitIter it = myConvFactors.find(aName);
    return it == myConvFactors.end() ? std::string("") : it->second.convUnit;
  }

  //! \brief Adds a unit conversion to the set of conversion rules.
  //! \param[in] propName Name of the physical property to convert
  //! \param[in] sf Conversion factor for the property parameter
  //! \param[in] origUnit Original unit name for the property parameter
  //! \param[in] convUnit Converted-to unit name for the property parameter
  void addConversion(const std::string& propName, double sf,
		     const std::string& origUnit = "",
		     const std::string& convUnit = "");

private:
  std::string name;      //!< Name of the unit calculator
  std::string origGroup; //!< Name of the original unit set
  std::string convGroup; //!< Name of the converted-to unit set
  SingleUnitMap myConvFactors; //!< Unit converter container

  //! \brief Global printing operator.
  friend std::ostream& operator<< (std::ostream&, const FFaUnitCalculator&);
  //! \brief Global reading operator.
  friend std::istream& operator>> (std::istream&, FFaUnitCalculator&);
};


/*!
  \brief Singelton class holding all unit calculators.
*/

class FFaUnitCalculatorProvider : public FFaSingelton<FFaUnitCalculatorProvider>
{
  //! \brief The constructor is private to allow singelton construction only.
  FFaUnitCalculatorProvider() {}
  //! \brief The destructor is private to allow singelton destruction only.
  virtual ~FFaUnitCalculatorProvider() {}

  //! \brief To allow construction and destruction.
  friend class FFaSingelton<FFaUnitCalculatorProvider>;

public:
  //! \brief Returns a named unit calculator.
  const FFaUnitCalculator* getCalculator(const std::string& calcName) const;
  //! \brief Returns a list of all unit calculators.
  std::vector<const FFaUnitCalculator*> getCalculators() const;

  //! \brief Gets a list of names for all unit calculators.
  void getCalculatorNames(std::vector<std::string>& definedCalcs) const;

  //! \brief Adds a new unit calculator to the container.
  void addCalculator(const FFaUnitCalculator& c) { myCalcs[c.getName()] = c; }

  //! \brief Reads/writes calculator data from file.
  bool readCalculatorDefs(const std::string& filename);
  //! \brief Writes calculator data to file.
  bool printCalculatorDefs(const std::string& filename) const;

private:
  std::map<std::string,FFaUnitCalculator> myCalcs; //!< Calculator container
};

#endif
