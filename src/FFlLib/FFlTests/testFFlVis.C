// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

/*!
  \file testFFlVis.C
  \brief Unit tests for the FFlVisualization module.
*/

#ifdef FT_HAS_GTEST
#include "gtest.h"
#endif
#include "FFlLib/FFlInit.H"
#include "FFlLib/FFlLinkHandler.H"
#include "FFlLib/FFlIOAdaptors/FFlReaders.H"
#include "FFlLib/FFlVisualization/FFlGroupPartCreator.H"
#include <iostream>

#ifdef FF_NAMESPACE
using namespace FF_NAMESPACE;
#endif


namespace
{
  std::string srcdir; //!< Absolute path to the source directory of this test

  //! \brief Loads the FE data file \a fName and creates tesselated geometry.
  std::vector<int> checkModel(const std::string& fName)
  {
    std::cout <<"\n   * Read file "<< fName;

    FFlLinkHandler link;
    if (int stat = FFlReaders::instance()->read(fName,&link); stat <= 0)
      return { stat < 0 ? stat : 1 };

    std::cout <<"\n   * Read done."<< std::endl;
    link.dump(fName);

    FFlGroupPartCreator gpc(&link);
    gpc.makeLinkParts();
    std::cout <<"\n   * Making link parts done.\n"<< std::endl;
    gpc.dump();

    std::vector<int> stat; // Extract some statistics
    for (const FFlGroupPartCreator::GroupPartMap::value_type& gp : gpc.getLinkParts())
    {
      stat.push_back(gp.second->getNoItems());
      stat.push_back(gp.second->getNoVisibleVertices());
    }
    std::cout <<"\n{"<< stat.front();
    for (size_t i = 1; i < stat.size(); i++)
      std::cout <<","<< stat[i];
    std::cout <<"}"<< std::endl;
    return stat;
  }
}


/*!
  \brief Simple test program for the FFlVisualization module.
*/

int main (int argc, char** argv)
{
#ifdef FT_HAS_GTEST
  ::testing::InitGoogleTest(&argc,argv);
#endif

  // Extract the source directory of the tests
  // to use as prefix for loading external files
  std::string fileName;
  for (int i = 1; i < argc; i++)
    if (!strncmp(argv[i],"--srcdir=",9))
    {
      srcdir = argv[i]+9;
      std::cout <<"Note: Source directory = "<< srcdir << std::endl;
      if (srcdir.back() != '/') srcdir += '/';
    }
    else if (argv[i][0] != '-' && fileName.empty())
      fileName = argv[i];

  FFlInit initializer;
  if (!fileName.empty())
    return checkModel(fileName).size();
#ifdef FT_HAS_GTEST
  else
    return RUN_ALL_TESTS(); // Run the parameterized unit tests
#else
  std::cout <<"usage: "<< argv[0] <<" <femfile>\n";
  return 0;
#endif
}


#ifdef FT_HAS_GTEST
namespace
{
  struct Input
  {
    const char* name; std::vector<int> data;
    Input(const char* f, const std::vector<int>& refs) : name(f), data(refs) {}
  };

  //! Output stream operator used by gtest in case of failure.
  std::ostream& operator<<(std::ostream& os, const Input& param)
  {
    os <<"\""<< param.name <<"\":";
    for (int d : param.data) os <<" "<< d;
    return os;
  }

  class TestFFlVis : public testing::Test,
                     public testing::WithParamInterface<Input> {};
}

//! Create a parameterized test reading/checking one file.
TEST_P(TestFFlVis, Tesselate)
{
  ASSERT_FALSE(srcdir.empty());
  EXPECT_EQ(checkModel(srcdir + GetParam().name), GetParam().data);
}

//! Instantiate the test over a list of FE data files.
INSTANTIATE_TEST_CASE_P(Tesselate, TestFFlVis,
                        testing::Values(Input("models/CTRIA03_.nas",
                                              { 4,8,20,40,17,34,38,76,
                                                2,6,32,96,0,0,0,0,0,0,0,0 }),
                                        Input("models/CTRIA06_.nas",
                                              { 4,8,20,40,7,14,14,28,
                                                2,6,8,48,0,0,0,0,0,0,0,0 }),
                                        Input("models/CQUAD04_.nas",
                                              { 4,8,20,40,8,16,22,44,
                                                2,6,16,64,0,0,0,0,0,0,0,0 }),
                                        Input("models/CQUAD08_.nas",
                                              { 4,8,20,40,3,6,6,12,
                                                2,6,4,32,0,0,0,0,0,0,0,0 }),
                                        Input("models/CTETRA04_.nas",
                                              { 22,44,56,112,
                                                155,310,280,560,
                                                17,51,224,672,
                                                459,918,536,1072,
                                                1060,3180,1060,3180 }),
                                        Input("models/CTETRA10_.nas",
                                              { 28,56,56,112,
                                                56,112,112,224,
                                                12,36,56,336,
                                                60,120,120,240,
                                                132,396,132,792 }),
                                        Input("models/CPENTA06_.nas",
                                              { 12,24,56,112,
                                                48,96,172,344,
                                                18,54,120,456,
                                                43,86,92,184,
                                                276,828,180,636 }),
                                        Input("models/CPENTA15_.nas",
                                              { 12,24,56,112,
                                                20,40,76,152,
                                                12,36,36,264,
                                                10,20,44,88,
                                                66,198,42,300 }),
                                        Input("models/CHEXA08_.nas",
                                              { 12,24,56,112,
                                                44,88,168,336,
                                                21,63,112,448,
                                                31,62,94,188,
                                                272,816,136,544 }),
                                        Input("models/CHEXA20_.nas",
                                              { 12,24,56,112,
                                                16,32,56,112,
                                                12,36,28,224,
                                                3,6,6,12,
                                                20,60,10,80 }),
                                        Input("models/Bucket.flm",
                                              { 145,290,271,542,
                                                1707,3414,2043,4086,
                                                153,459,1475,4532,
                                                0,0,0,0,0,0,0,0 }),
                                        Input("models/BucketLink.flm",
                                              { 254,508,292,584,
                                                1134,2268,1232,2464,
                                                713,2139,1016,3048,
                                                1418,2836,1425,2850,
                                                3172,9516,3172,9516 }) ));
#endif
