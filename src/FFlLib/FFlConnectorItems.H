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
  \brief Storage of spider connector properties for Triads.
*/

class FFlConnectorItems
{
public:
  friend bool operator==(const FFlConnectorItems& a, const FFlConnectorItems& b);

  friend std::ostream& operator<<(std::ostream& s, const FFlConnectorItems& c);
  friend std::istream& operator>>(std::istream& s, FFlConnectorItems& c);

  void addNode(int nID) { this->nodes.push_back(nID); }
  void addElement(int eID) { this->elements.push_back(eID); }

  const std::vector<int>& getNodes() const { return this->nodes; }
  const std::vector<int>& getElements() const { return this->elements; }

  bool empty() const { return this->nodes.empty() && this->elements.empty(); }

  void clear() { this->nodes.clear(); this->elements.clear(); }

private:
  std::vector<int> nodes;
  std::vector<int> elements;
};

#endif
