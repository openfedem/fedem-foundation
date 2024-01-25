// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

/*!
  \file test_FFp.C
  \brief Unit testing for FFpLib.
  \details The purpose of this file is to provide some unit tests for
  various basis functionality in FFpLib.
*/

#include "FFpLib/FFpCurveData/FFpGraph.H"
#include "FFpLib/FFpCurveData/FFpCurve.H"
#include "FFrLib/FFrExtractor.H"
#include "FFrLib/FFrReadOpInit.H"
#include "FFaLib/FFaOperation/FFaBasicOperations.H"
#include "FFaLib/FFaDefinitions/FFaResultDescription.H"
#include <cstring>
#include <iostream>
#include "gtest.h"


std::string srcdir; //!< Full path to the source directory for this test


/*!
  \brief Main program for the unit test executable.
*/

int main (int argc, char** argv)
{
  // Initialize the google test module.
  // This will remove the gtest-specific values in argv.
  ::testing::InitGoogleTest(&argc,argv);

  FFr::initReadOps();
  FFa::initBasicOps();

  // Extract the source directory of the test
  // to use as prefix for loading frs-files
  std::vector<std::string> frsFiles;
  for (int i = 1; i < argc; i++)
    if (!strncmp(argv[i],"--srcdir=",9))
    {
      srcdir = argv[i]+9;
      std::cout <<"Note: Source directory = "<< srcdir << std::endl;
      if (srcdir.back() != '/') srcdir += '/';
      break;
    }

  // Invoke the google test driver
  int status = RUN_ALL_TESTS();

  // Clean up heap memory
  FFrExtractor::releaseMemoryBlocks(true);
  FFr::clearReadOps();

  return status;
}

//! \brief Data type to instantiate particular units tests over.
struct FFpCase
{
  const char* file = NULL; //!< FRS file name, relative to \ref srcdir
  const char* type = NULL; //!< Mechanism object type
  int baseId = 0;          //!< Base Id of object
  const char* varT = NULL; //!< Variable type
  const char* varN = NULL; //!< Variable name
  const char* oper = NULL; //!< Operation creating a single value from variable
  size_t i = 0;   //!< Index of curve point to compare
  double X = 0.0; //!< X-axis reference value
  double Y = 0.0; //!< Y-axis reference value
};

//! \brief Class describing a parameterized unit test instance.
class TestFFp : public testing::Test, public testing::WithParamInterface<FFpCase> {};


/*!
  Create a parameterized test reading an frs-file for plotting.
  GetParam() will be substituted with the actual TestFFp instance.
*/

TEST_P(TestFFp, Load)
{
  ASSERT_FALSE(srcdir.empty());

  // Create a results extractor and load the specified file
  FFrExtractor* extr = new FFrExtractor("RDB reader");
  std::cout <<"   * Opening file "<< srcdir << GetParam().file << std::endl;
  ASSERT_TRUE(extr->addFile(srcdir+GetParam().file,true));

  // Define result quantity to plot
  FFaTimeDescription   timeItem;
  FFaResultDescription resultItem(GetParam().type,GetParam().baseId);
  resultItem.varRefType   =   GetParam().varT;
  resultItem.varDescrPath = { GetParam().varN };
  std::string timeOp("None"), resultOp(GetParam().oper);
  std::cout <<"   * Trying to load curve data for "<< resultItem
            <<" with operation "<< resultOp << std::endl;

  // Initialize the curve axis definitions
  FFpCurve myCurve;
  ASSERT_TRUE(myCurve.initAxis(timeItem,timeOp,0));
  ASSERT_TRUE(myCurve.initAxis(resultItem,resultOp,1));

  // Initialize a single-curve graph object
  FFpGraph rdbCurves(&myCurve);
  rdbCurves.setTimeInterval(0.0,1.0);

  // Read curve data from file
  std::string message;
  bool loadStatus = rdbCurves.loadTemporalData(extr,message);
  if (!message.empty()) std::cout << message << std::endl;
  const std::vector<double>& X = myCurve.getAxisData(0);
  const std::vector<double>& Y = myCurve.getAxisData(1);
  size_t i, n = X.size() < Y.size() ? X.size() : Y.size();
  for (i = 0; i < 10 && i < n; i++)
    std::cout << i+1 <<": "<< X[i] <<" "<< Y[i] << std::endl;
  if (n > 10) std::cout << n <<": "<< X[n-1] <<" "<< Y[n-1] << std::endl;
  ASSERT_TRUE(loadStatus);

  delete extr;

  // Compare curve point with provided reference values
  size_t index = GetParam().i;
  ASSERT_LT(index, X.size());
  ASSERT_LT(index, Y.size());
  EXPECT_NEAR(X[index], GetParam().X, 1.0e-8);
  EXPECT_NEAR(Y[index], GetParam().Y, 1.0e-8);
  std::cout << index+1 <<": "<< X[index] <<" "<< Y[index] << std::endl;
}


//! Instantiate the test over a list of cases
INSTANTIATE_TEST_CASE_P(TestFFp, TestFFp,
                        testing::Values(FFpCase{"th_p_1.frs", "Triad", 14, "TMAT34", "Position matrix", "Position Z", 90, 0.9, 1.13884975 }));
