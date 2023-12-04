// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

/*!
  \file testParser.C
  \brief Unit tests for the FE model parsers.
*/

#include "gtest/gtest.h"

#include "FFlLib/FFlLinkHandler.H"
#include "FFlLib/FFlIOAdaptors/FFlReaders.H"
#include "FFlLib/FFlIOAdaptors/FFlAllIOAdaptors.H"
#include "FFlLib/FFlFEParts/FFlAllFEParts.H"
#include "FFaLib/FFaAlgebra/FFaVec3.H"
#include "FFaLib/FFaOS/FFaFortran.H"

static std::string inpdir; //!< Full path of the input file directory


/*!
  \brief Main program for the FE model parser unit test executable.
*/

int main (int argc, char** argv)
{
  // Initialize the google test module.
  // This will remove the gtest-specific values in argv.
  ::testing::InitGoogleTest(&argc,argv);

  // Extract the source directory of the tests
  // to use as prefix for loading the input files
  for (int i = 1; i < argc; i++)
    if (!strncmp(argv[i],"--srcdir=",9))
    {
      inpdir = argv[i]+9;
      std::cout <<"Note: Source directory = "<< inpdir << std::endl;
      if (inpdir.back() != '/') inpdir += '/';
      inpdir += "models/"; // sub-folder with input files
      break;
    }

  FFl::initAllReaders();
  FFl::initAllElements();

  // Invoke the google test driver
  int status = RUN_ALL_TESTS();

  FFl::releaseAllElements();
  FFl::releaseAllReaders();

  return status;
}


/*!
  \brief Creates a unit test for the Nastran element group parser.
*/

TEST(TestFFl,NastranParser)
{
  FFlLinkHandler part;
  ASSERT_GT(FFlReaders::instance()->read(inpdir+"PistonPin.nas",&part),0);
  std::cout <<"Successfully read "<< inpdir <<"PistonPin.nas"<< std::endl;

  // Lambda function for printing an element group
  auto printGroup = [](const FFlGroup* gr)
  {
    std::cout <<"Element group "<< gr->getID() <<" \""<< gr->getName() <<"\":";
    for (const GroupElemRef& elm : *gr) std::cout <<" "<< elm.getID();
    std::cout << std::endl;
  };

  FFlGroup* group = part.getGroup(1);
  ASSERT_TRUE(group != NULL);
  EXPECT_EQ(group->size(),1U);
  printGroup(group);

  group = part.getGroup(2);
  ASSERT_TRUE(group != NULL);
  EXPECT_EQ(group->size(),3U);
  printGroup(group);

  group = part.getGroup(3);
  ASSERT_TRUE(group != NULL);
  EXPECT_EQ(group->size(),10U);
  printGroup(group);

  group = part.getGroup(4);
  ASSERT_TRUE(group != NULL);
  EXPECT_EQ(group->size(),7U);
  printGroup(group);

  group = part.getGroup(5);
  ASSERT_TRUE(group != NULL);
  EXPECT_EQ(group->size(),11U);
  printGroup(group);
}


void ffl_setLink(FFlLinkHandler* part);
SUBROUTINE(ffl_getcoor,FFL_GETCOOR) (double* X, double* Y, double* Z,
                                     const int& iel, int& ierr);

/*!
  \brief Creates a unit test for the SESAM input file parser.
*/

TEST(TestFFl,SesamParser)
{
  FFlReaders::convertToLinear = 1;

  FFlLinkHandler part;
  ASSERT_GT(FFlReaders::instance()->read(inpdir+"Krum-bjelke.FEM",&part),0);
  std::cout <<"Successfully read "<< inpdir <<"Krim-bjelke.FEM"<< std::endl;

  part.dump();
  ffl_setLink(&part);

  int ierr = 0;
  double X[5], Y[5], Z[5];
  for (int iel = 1; iel <= part.getElementCount(); iel++)
  {
    F90_NAME(ffl_getcoor,FFL_GETCOOR) (X,Y,Z,iel,ierr);
    ASSERT_EQ(ierr,0);

    FaVec3 end1(X[0],Y[0],Z[0]);
    FaVec3 end2(X[1],Y[1],Z[1]);
    FaVec3 Zaxis(X[2]-X[0],Y[2]-Y[0],Z[2]-Z[0]);
    std::cout <<"\nElement "<< iel <<": "<< part.getElement(iel-1,true)->getID();
    std::cout <<"\nEnde 1: "<< end1;
    std::cout <<"\nEnde 2: "<< end2;
    std::cout <<"\nZ-akse: "<< Zaxis;
    std::cout << std::endl;

    if (iel == 2)
    {
      EXPECT_NEAR((end2-end1).length(),0.6,1.0e-4);
      EXPECT_NEAR(Zaxis.z(),1.0,1.0e-8);
    }
    else if (iel == 3)
    {
      EXPECT_NEAR((end2-end1).length(),0.85,1.0e-4);
      EXPECT_NEAR(Zaxis.y(),-1.0,1.0e-8);
    }
    else
      EXPECT_NEAR((end2-end1).length(),0.079382,1.0e-4);
  }
}
