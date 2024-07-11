// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#include "gtest/gtest.h"
#include "FFaLib/FFaDefinitions/FFaVersionNumber.H"
#include "Admin/FedemAdmin.H"


std::ostream& operator<<(std::ostream& os, const FFaVersionNumber& v)
{
  return os << v.getInterpretedString();
}


TEST(TestFFaLib,Version)
{
  FFaVersionNumber v1("R3.5-beta4");
  FFaVersionNumber v2("R4.2.1-alpha3");
  FFaVersionNumber v3("R5.1.2");
  FFaVersionNumber v4("R7.5 (build 12)");

  std::cout << FedemAdmin::getCopyrightString()
            <<"\nVersion: "<< FedemAdmin::getVersion()
            <<"\nBuild date: "<< FedemAdmin::getBuildDate()
            <<"\nBuild year: "<< FedemAdmin::getBuildYear() << std::endl;
  std::cout <<"v1 = "<< v1.getString() << std::endl;
  std::cout <<"v2 = "<< v2.getString() << std::endl;
  std::cout <<"v3 = "<< v3.getString() << std::endl;
  std::cout <<"v4 = "<< v4.getString() << std::endl;

  EXPECT_EQ(v1,FFaVersionNumber(3,5,0,4));
  EXPECT_EQ(v2,FFaVersionNumber(4,2,1,3));
  EXPECT_EQ(v3,FFaVersionNumber(5,1,2));
  EXPECT_EQ(v4,FFaVersionNumber(7,5,0,12));

  EXPECT_LT(v1,v2);
  EXPECT_LT(v2,v3);
  EXPECT_LT(v3,v4);
  EXPECT_GT(v4,v1);

  EXPECT_LT(v2,FFaVersionNumber("R4.2.1"));
  EXPECT_GT(v4,FFaVersionNumber("R7.5"));
  std::cout << FedemAdmin::getCopyrightString() << std::endl;
}
