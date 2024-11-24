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

#include "FFlLib/FFlInit.H"
#include "FFlLib/FFlLinkHandler.H"
#include "FFlLib/FFlElementBase.H"
#include "FFlLib/FFlAttributeBase.H"
#include "FFlLib/FFlField.H"
#include "FFlLib/FFlGroup.H"
#include "FFlLib/FFlIOAdaptors/FFlReaders.H"
#include "FFaLib/FFaAlgebra/FFaVec3.H"
#include "FFaLib/FFaOS/FFaFortran.H"

using DoubleVec = std::vector<double>; //!< Convenience alias

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
  \brief Convenience function initializing \a part from \a fileName
*/

static void readPart (FFlLinkHandler& part, const char* fileName)
{
  ASSERT_GT(FFlReaders::instance()->read(inpdir+ fileName,&part),0);
  std::cout <<"\nSuccessfully read "<< inpdir << fileName << std::endl;
}


/*!
  \brief Creates a unit test for the Nastran element group parser.
*/

TEST(TestFFl,NastranParser)
{
  FFlLinkHandler part;
  readPart(part,"PistonPin.nas");

  // Lambda function for printing an element group
  auto&& printGroup = [](const FFlGroup* gr)
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


/*!
  \brief Creates a unit test for parsing of tapered beams.
*/

TEST(TestFFl,TaperedBeams)
{
  FFlLinkHandler part;
  readPart(part,"PBEAM-test.nas");

  size_t i = 0;
  std::vector<DoubleVec> values(part.getAttributeCount("PBEAMSECTION"));
  for (const AttributeMap::value_type& att : part.getAttributes("PBEAMSECTION"))
  {
    DoubleVec& fields = values[i++];
    for (FFlFieldBase* field : *att.second)
      fields.push_back(static_cast<FFlField<double>*>(field)->getValue());
    std::cout <<"Property "<< att.first <<":";
    for (double d : fields) std::cout <<" "<< d;
    std::cout << std::endl;
  }

  for (i = 1; i < values.size(); i++)
    for (size_t j = 0; j < values.front().size(); j++)
      EXPECT_NEAR(values.front()[j],values[i][j],1.0e-8);
}


/*!
  \brief Creates a unit test for parsing various cross section types.
*/

TEST(TestFFl,BeamCrossSections)
{
  FFlLinkHandler part, partB;
  readPart(part,"PBEAML-test.nas");
  readPart(partB,"RectangularBeam.nas");

  for (const AttributeMap::value_type& att : partB.getAttributes("PBEAMSECTION"))
    part.addAttribute(att.second->clone());

  size_t i = 0;
  std::vector<DoubleVec> values(part.getAttributeCount("PBEAMSECTION"));
  for (const AttributeMap::value_type& att : part.getAttributes("PBEAMSECTION"))
  {
    DoubleVec& fields = values[i++];
    for (FFlFieldBase* field : *att.second)
      fields.push_back(static_cast<FFlField<double>*>(field)->getValue());
    std::cout <<"PBEAMSECTION "<< att.first <<":";
    for (double d : fields) std::cout <<" "<< d;
    std::cout << std::endl;
  }
}


/*!
  \brief Creates a unit test for parsing Nastran MPC entries.
*/

TEST(TestFFl,MPC)
{
  FFlLinkHandler partA, partB;
  readPart(partA,"MPC-test.nas");
  readPart(partB,"MPC_RGD_Test.nas");
  EXPECT_EQ(partA.getElementCount("WAVGM"),2);
  EXPECT_EQ(partB.getElementCount("WAVGM"),5);
}


/*!
  \brief Creates a unit test for the SESAM input file parser.
*/

TEST(TestFFl,SesamParser)
{
  FFlReaders::convertToLinear = 1;

  FFlLinkHandler part;
  readPart(part,"Krum-bjelke.FEM");
  part.dump();

  int iel = 0;
  double X[5], Y[5], Z[5];
  for (ElementsCIter eit = part.elementsBegin(); eit != part.elementsEnd(); ++eit)
  {
    ASSERT_EQ((*eit)->getNodalCoor(X,Y,Z),0);

    FaVec3 end1(X[0],Y[0],Z[0]);
    FaVec3 end2(X[1],Y[1],Z[1]);
    FaVec3 Zaxis(X[2]-X[0],Y[2]-Y[0],Z[2]-Z[0]);
    std::cout <<"\nElement "<< iel++ <<": "<< (*eit)->getID();
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
