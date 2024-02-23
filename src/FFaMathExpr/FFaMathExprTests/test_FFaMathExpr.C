// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#include "gtest.h"

int eval_expression (const char* mathExpr, const std::vector<double>& args, double& f);


struct TestData
{
  const char*         expr; //!< The math expression to evaluate
  std::vector<double> args; //!< Function arguments
  double              fVal; //!< Expected function value
  TestData(const char* s, const std::vector<double>& x, double f) : expr(s), args(x), fVal(f) {}
};

class TestFFaMathExpr : public testing::Test, public testing::WithParamInterface<TestData> {};


// Create a parameterized test evaluating the provided extression with the provided arguments.
TEST_P(TestFFaMathExpr, Eval)
{
  double funcVal = 0.0;
  ASSERT_EQ(eval_expression(GetParam().expr,GetParam().args,funcVal),0);
  EXPECT_NEAR(funcVal,GetParam().fVal,1.0e-8);
}


// Instantiate the test over a list of expressions and function arguments
INSTANTIATE_TEST_CASE_P(TestExpr, TestFFaMathExpr, //                 x    y    z    t    fVal
                        testing::Values( TestData("2.0*x+y"       ,{ 1.0, 2.5          }, 4.5      ),
                                         TestData("x^2+y*cos(t)-z",{ 1.5, 2.5, 1.0, 0.5}, 3.4439564),
                                         TestData("x%y"           ,{13.6, 6.1          }, 1.4      ) ));

// Create a test to verify proper error handling
TEST(FFaMathExpr, Error)
{
  double funcVal = 0.0;
  ASSERT_EQ(eval_expression("2.0/(1+(x*1.2+3.7)*4.5)",{1.2},funcVal),0);
  ASSERT_NE(eval_expression("123)*4.5)",{0.0},funcVal),0);
}
