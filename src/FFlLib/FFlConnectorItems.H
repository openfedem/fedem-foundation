// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#ifndef FFL_CONNECTOR_ITEMS_H
#define FFL_CONNECTOR_ITEMS_H

#include <vector>
#include <iostream>


/*!
  Class FFlConnectorItems is used by FmTriad to store spider connector
  properties for reading and writing.

  <i>Espen Medbo 2006</i>
*/

class FFlConnectorItems
{
public:
  FFlConnectorItems() {}
  ~FFlConnectorItems() {}

  friend bool operator== (const FFlConnectorItems& a,
			  const FFlConnectorItems& b);

  friend  std::ostream& operator<< (std::ostream& s, const FFlConnectorItems& connector);
  friend  std::istream& operator>> (std::istream& s, FFlConnectorItems& connector);

private:
  static void readStream (std::istream& is, std::vector<int>& data);

public:
  void addNode    (int nodeID)     { this->nodes.push_back(nodeID); }
  void addElement (int elementID)  { this->elements.push_back(elementID); }
  void addProperty(int propertyID) { this->properties.push_back(propertyID); }

  const std::vector<int>& getNodes     () const { return this->nodes; }
  const std::vector<int>& getElements  () const { return this->elements; }
  const std::vector<int>& getProperties() const { return this->properties; }

  bool empty() const
  {
    return this->nodes.empty()
      &&   this->elements.empty()
      &&   this->properties.empty();
  }

  void clear()
  {
    this->nodes.clear();
    this->elements.clear();
    this->properties.clear();
  }

private:
  std::vector<int> nodes;
  std::vector<int> elements;
  std::vector<int> properties;
};

#endif
