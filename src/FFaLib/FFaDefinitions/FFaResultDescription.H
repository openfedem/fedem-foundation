// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

/*!
  \file FFaResultDescription.H
  \brief Result variable description.
*/

#ifndef FFA_RESULT_DESCRIPTION_H
#define FFA_RESULT_DESCRIPTION_H

#include <vector>
#include <string>


/*!
  \brief Class representing a result variable description.
  \details This class is used to represent result quantities in a Fedem model.
  it contains q unique identification of the mechanism object the result
  quantity is associated with, the type of result, and a path in the result
  field hierarchy pointing to the unique result variable.
  \sa FFrResultContainer.
  \sa FFrExtractor.
*/

class FFaResultDescription
{
public:
  //! \brief Default constructor.
  FFaResultDescription(const char* name = NULL, int bid = 0, int uid = 0)
    : baseId(bid), userId(uid) { if (name) OGType = name; }
  //! \brief Empty destructor.
  virtual ~FFaResultDescription() {}

  //! \brief Clears the result description, resetting it to default values.
  void clear();
  //! \brief Copies the result path (but leaves the type and ID untouched).
  void copyResult(const FFaResultDescription& other);

  //! \brief Returns a textural description of the result path.
  std::string getText() const;

  //! \brief Returns \e true if this describes a time-dependent result quantity.
  bool isTime() const;
  //! \brief Returns \e true if this describes a beam section result quantity.
  bool isBeamSectionResult() const;
  //! \brief Returns \e true if this result description is empty (no results).
  bool empty() const { return OGType.empty() && varDescrPath.empty(); }

  //! \brief Equality operator.
  bool operator==(const FFaResultDescription& entry) const;

  std::string OGType; //!< Top-level variable type (owner group)
  int         baseId; //!< Unique ID over the entire model
  int         userId; //!< Unique only within owner group

  std::string              varRefType;   //!< Variable type description
  std::vector<std::string> varDescrPath; //!< Below top-level variable
};


/*!
  \brief Sub-class representing the physical time.
*/

class FFaTimeDescription : public FFaResultDescription
{
public:
  //! \brief Default constructor.
  FFaTimeDescription();
  //! \brief Empty destructor.
  virtual ~FFaTimeDescription() {}
};


//! \brief Global output stream operator.
std::ostream& operator<<(std::ostream&, const FFaResultDescription&);
//! \brief Global input stream operator.
std::istream& operator>>(std::istream&, FFaResultDescription&);

#endif
