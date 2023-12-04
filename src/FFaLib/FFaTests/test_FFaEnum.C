// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#include "gtest/gtest.h"
#include "FFaLib/FFaString/FFaEnum.H"
#include <sstream>


class ClassWithEnum
{
public:
  enum SomeEnumType {
    ZERO = 0,
    AONE = 1,
    ATWO = 2,
    ATHREE = 3
  };

private:
  FFaEnumMapping(SomeEnumType) {
    FFaEnumEntry(ZERO,  "ZERO");
    FFaEnumEntry(AONE,  "AONE");
    FFaEnumEntry(ATWO,  "ATWO");
    FFaEnumEntry(ATHREE,"ATHREE");
    FFaEnumEntryEnd;
  };

public:
  SomeEnumTypeEnum myEnum;
};


TEST(TestFFaLib,Enum)
{
  ClassWithEnum a, b;

  auto&& enumVal = [](const ClassWithEnum& c) -> int
  {
    int ic = c.myEnum;
    std::cout <<"The enum value is "<< c.myEnum <<"="<< ic << std::endl;
    return ic;
  };

  a.myEnum = ClassWithEnum::ATHREE;
  EXPECT_EQ(enumVal(a),3);

  b.myEnum = a.myEnum;
  EXPECT_EQ(enumVal(b),3);

  EXPECT_TRUE(b.myEnum == a.myEnum);

  a.myEnum = "ATWO";
  EXPECT_EQ(enumVal(a),2);
  EXPECT_TRUE(b.myEnum != a.myEnum);

  b.myEnum = " AONE";
  EXPECT_EQ(enumVal(b),1);

  ClassWithEnum::SomeEnumType en = a.myEnum;
  int ia = en;
  EXPECT_EQ(ia,2);

  a.myEnum = 4;
  EXPECT_EQ(enumVal(a),4);

  a.myEnum = "2";
  EXPECT_EQ(enumVal(a),2);

  a.myEnum = "5";
  EXPECT_EQ(enumVal(a),5);

  a.myEnum = "6.2";
  EXPECT_EQ(enumVal(a),5);

  a.myEnum = "AFOUR";
  EXPECT_EQ(enumVal(a),5);

  std::stringstream s0("  ZERO");
  s0 >> a.myEnum;
  EXPECT_EQ(enumVal(a),0);

  std::stringstream s1("1");
  s1 >> a.myEnum;
  EXPECT_EQ(enumVal(a),1);

  std::stringstream s2("ATWO ");
  s2 >> a.myEnum;
  EXPECT_EQ(enumVal(a),2);

  std::stringstream s3(" 3 ");
  s3 >> a.myEnum;
  EXPECT_EQ(enumVal(a),3);

  a.myEnum = 0;
  std::stringstream s4("AONE ATWO ATHREE");
  for (int i = 1; i <= 3; i++)
  {
    s4 >> a.myEnum;
    ASSERT_TRUE(s4 ? true : false);
    EXPECT_EQ(enumVal(a),i);
  }

  std::stringstream s5("\"AONE\"");
  s5 >> a.myEnum;
  EXPECT_EQ(enumVal(a),1);
}
