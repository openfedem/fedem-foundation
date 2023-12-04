// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#ifndef FFL_FEDEM_WRITER_H
#define FFL_FEDEM_WRITER_H

#include "FFlWriterBase.H"
#include <string>
#include <vector>
#include <iostream>


class FFlFedemWriter : public FFlWriterBase
{
public:
  FFlFedemWriter(const FFlLinkHandler* link) : FFlWriterBase(link) {}
  virtual ~FFlFedemWriter() {}

  bool write(const std::string& filename,
	     bool writeExtNodes = true, bool writeChecksum = false,
	     const std::vector<std::string>& metaData = std::vector<std::string>()) const;

protected:
  bool writeMetaData(std::ostream& os, bool writeExtNodes, bool writeChecksum,
		     const std::vector<std::string>& metaData) const;
  bool writeNodeData(std::ostream& os, bool writeExtNodes) const;
  bool writeElementData(std::ostream& os) const;
  bool writeLoadData(std::ostream& os) const;
  bool writeGroupData(std::ostream& os) const;
  bool writeAttributeData(std::ostream& os) const;
  bool writeVisualData(std::ostream& os) const;
};

#endif
