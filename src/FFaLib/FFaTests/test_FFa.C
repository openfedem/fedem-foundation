// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

/*!
  \file test_FFa.C
  \brief Unit testing for FFaLib.
  \details The purpose of this file is to provide some unit tests for
  various basis functionality in FFaLib.
*/

#include "gtest.h"
#include "FFaLib/FFaOS/FFaFilePath.H"
#include "FFaLib/FFaAlgebra/FFaMath.H"
#include "FFaLib/FFaAlgebra/FFaVec3.H"
#include "FFaLib/FFaAlgebra/FFaCheckSum.H"
#include "FFaLib/FFaString/FFaStringExt.H"
#include <array>

int BodyTest (const std::string& fname, double z0, double z1);

static std::string srcdir; //!< Full path of the source directory of this test


/*!
  \brief Main program for the unit test executable.
*/

int main (int argc, char** argv)
{
  // Initialize the google test module.
  // This will remove the gtest-specific values in argv.
  ::testing::InitGoogleTest(&argc,argv);

  // Extract the source directory of the tests
  // to use as prefix for loading external files
  for (int i = 1; i < argc; i++)
    if (!strncmp(argv[i],"--srcdir=",9))
    {
      srcdir = argv[i]+9;
      std::cout <<"Note: Source directory = "<< srcdir << std::endl;
      if (srcdir.back() != '/') srcdir += '/';
      break;
    }

  // Invoke the google test driver
  return RUN_ALL_TESTS();
}


//! \brief Class describing a parameterized unit test instance for FFaBody.
class TestFFaBody : public testing::Test, public testing::WithParamInterface<const char*> {};


/*!
  Creates a parameterized test reading a geometry file.
  GetParam() will be substituted with the actual file name.
*/

TEST_P(TestFFaBody, Read)
{
  ASSERT_FALSE(srcdir.empty());
  ASSERT_EQ(BodyTest(srcdir+GetParam(),1.0,1.0),0);
}


/*!
  Instantiate the test over a list of file names.
*/

INSTANTIATE_TEST_CASE_P(TestFTC, TestFFaBody,
			testing::Values( "cube3.ftc",
					 "cube4.ftc",
					 "Stift_Kurz.ftc",
					 "Q_Sylindermodell_D10m_L15m.ftc",
					 "T_Sylindermodell_D10m_L15m.ftc" ));


/*!
  Creates some path manipulation tests.
*/

TEST(TestFFa,FilePath)
{
  std::string fileName("C:/Jalla/foo/bar/filnavn.dat");
  std::string relName("../foo/bar/filnavn.dat");

  EXPECT_TRUE(FFaFilePath::hasPath(fileName));
  EXPECT_TRUE(FFaFilePath::hasPath(relName));
  EXPECT_FALSE(FFaFilePath::isRelativePath(fileName));
  EXPECT_TRUE(FFaFilePath::isRelativePath(relName));
  EXPECT_FALSE(FFaFilePath::isExtension(fileName,"data"));
  EXPECT_TRUE(FFaFilePath::isExtension(fileName,"dat"));

  EXPECT_STREQ(FFaFilePath::getPath(fileName,false).c_str(),"C:/Jalla/foo/bar");
  EXPECT_STREQ(FFaFilePath::getPath(fileName,true).c_str(),"C:/Jalla/foo/bar/");
  EXPECT_STREQ(FFaFilePath::getExtension(fileName).c_str(),"dat");
  EXPECT_STREQ(FFaFilePath::getBaseName(fileName).c_str(),"C:/Jalla/foo/bar/filnavn");
  EXPECT_STREQ(FFaFilePath::getBaseName(fileName,true).c_str(),"filnavn");

  std::string mainPath("C:/Jalla/dump");
  FFaFilePath::checkName(mainPath);
  FFaFilePath::checkName(fileName);
  FFaFilePath::checkName(relName);
  EXPECT_STREQ(FFaFilePath::getRelativeFilename(mainPath,fileName).c_str(),relName.c_str());
  EXPECT_STREQ(FFaFilePath::appendFileNameToPath(mainPath,relName).c_str(),fileName.c_str());
#if defined(win32) || defined(win64)
  EXPECT_STREQ(FFaFilePath::getRelativeFilename(".\\foo\\bar",".\\").c_str(),"..\\..\\");
  EXPECT_STREQ(FFaFilePath::getRelativeFilename("foo\\bar","").c_str(),"..\\..\\");
#else
  EXPECT_STREQ(FFaFilePath::getRelativeFilename("./foo/bar","./").c_str(),"../../");
  EXPECT_STREQ(FFaFilePath::getRelativeFilename("foo/bar","").c_str(),"../../");
#endif

  std::string mainPath1("C:/Jalla/dump/");
  std::string fileName1("C:/Jalla/dump/filnavn.dat");
  FFaFilePath::checkName(mainPath1);
  FFaFilePath::checkName(fileName1);
  EXPECT_STREQ(FFaFilePath::appendFileNameToPath(mainPath1,relName).c_str(),fileName.c_str());
  EXPECT_STREQ(FFaFilePath::appendFileNameToPath(mainPath,relName).c_str(),fileName.c_str());
  EXPECT_STREQ(FFaFilePath::appendFileNameToPath(mainPath1,"./filnavn.dat").c_str(),fileName1.c_str());
  EXPECT_STREQ(FFaFilePath::appendFileNameToPath(mainPath,"./filnavn.dat").c_str(),fileName1.c_str());
  EXPECT_STREQ(FFaFilePath::appendFileNameToPath(mainPath,"filnavn.dat").c_str(),fileName1.c_str());
}


//! \brief Data type to instantiate particular unit tests over.
typedef std::array<double,5> Case;

//! \brief Class describing a parameterized unit test instance for cubicSolve().
class TestFFaCubic : public testing::Test, public testing::WithParamInterface<Case> {};


/*!
  Creates a parameterized test solving a cubic equation.
  GetParam() will be substituted with the actual equation parameters.
*/

TEST_P(TestFFaCubic, Solve)
{
  const double eps = 1.0e-12;

  double A = GetParam()[0];
  double B = GetParam()[1];
  double C = GetParam()[2];
  double X = GetParam()[3];
  double D = ((A*X + B)*X + C)*X;
  std::cout <<"Solving: ";
  if (A != 0.0) std::cout << A <<"*x^3 + ";
  if (B != 0.0) std::cout << B <<"*x^2 + ";
  if (C != 0.0) std::cout << C <<"*x - ";
  double sol[3];
  std::cout << D <<" = 0"<< std::endl;
  int nSol = FFa::cubicSolve(A,B,C,-D,sol);
  ASSERT_GT(nSol,0);
  bool found = false;
  int oldpre = std::cout.precision(15);
  for (int i = 0; i < nSol; i++)
  {
    std::cout <<"Solution "<< i+1 <<": "<< sol[i] << std::endl;
    EXPECT_FALSE(std::isnan(sol[i]));
    if (fabs(sol[i]-X) < eps) found = true;
  }
  if (!found)
    std::cout <<" *** Did not find "<< X <<" as solution.\n";
  std::cout.precision(oldpre);
  EXPECT_TRUE(found);
}


/*!
  Instantiate the test over some parameters.
*/

INSTANTIATE_TEST_CASE_P(TestSolve, TestFFaCubic,
                        //                     A    B    C    X
                        testing::Values(Case{ 0.0, 0.0, 2.5, 1.2 },
                                        Case{ 0.0, 0.1, 2.3, 4.5 },
                                        Case{ 0.1, 2.3, 4.5, 6.7 },
                                        Case{ 1.2, 2.3, 3.4, 4.5 }));


//! \brief Data type to instantiate particular unit tests over.
typedef std::array<double,8> CaseII;

//! \brief Class describing a parameterized unit test instance for bilinearSolve().
class TestFFaBilin : public testing::Test, public testing::WithParamInterface<CaseII> {};


/*!
  Creates a parameterized test solving a bilinear equation system.
  GetParam() will be substituted with the actual equation parameters.
*/

TEST_P(TestFFaBilin, Solve)
{
  const double eps = 1.0e-6;

  double x = GetParam()[6];
  double y = GetParam()[7];
  double A[4], B[4];
  A[0] = GetParam()[0];
  A[1] = GetParam()[1];
  A[2] = GetParam()[2];
  A[3] = A[0]*x*y + A[1]*x + A[2]*y;
  B[0] = GetParam()[3];
  B[1] = GetParam()[4];
  B[2] = GetParam()[5];
  B[3] = B[0]*x*y + B[1]*x + B[2]*y;
  std::cout <<"Solving: "
            << A[0] <<"*x*y + "<< A[1] <<"*x + "<< A[2] <<"*y = "<< A[3] <<"\n         "
            << B[0] <<"*x*y + "<< B[1] <<"*x + "<< B[2] <<"*y = "<< B[3] <<"\n";
  double X[2], Y[2];
  int nSol = FFa::bilinearSolve(A,B,X,Y);
  ASSERT_GT(nSol,0);
  bool found = false;
  int oldpre = std::cout.precision(15);
  for (int i = 0; i < nSol; i++)
  {
    std::cout <<"Solution "<< i+1 <<": "<< X[i] <<" "<< Y[i] << std::endl;
    EXPECT_FALSE(std::isnan(X[i]) || std::isnan(Y[i]));
    if (fabs(X[i]-x) < eps && fabs(Y[i]-y) < eps) found = true;
  }
  if (!found)
    std::cout <<" *** Did not find "<< x <<" "<< y <<" as solution.\n";
  std::cout.precision(oldpre);
  EXPECT_TRUE(found);
}


/*!
  Instantiate the test over some parameters.
*/

INSTANTIATE_TEST_CASE_P(TestSolve, TestFFaBilin,
                        //                      A0   A1   A2   B0   B1   B2    X    Y
                        testing::Values(CaseII{ 1.2, 2.3, 3.4, 0.1, 1.2, 2.3, 0.5, 0.6 },
                                        CaseII{-1.5, 0.3, 8.1, 0.4,-1.1, 3.7,-0.3, 0.8 } ));


/*!
  Creates some checksum calculation unit tests.
*/

TEST(TestFFa,CheckSum)
{
  FaVec3 A(-0.005,1.9899936829,2.59123456789);
  FaVec3 B(-0.005,1.989993683 ,2.591234568);
  FFaCheckSum csA, csB;
  // Test new checksum algorithm for doubles,
  // rounding to the given number of significant digits
  csA.add(A,10);
  csB.add(B,10);
  std::cout <<"Checksum with 10 significant digits "<< csA.getCurrent() << std::endl;
  ASSERT_EQ(csA.getCurrent(),csB.getCurrent());
  csA.add(A,3);
  csB.add(B,3);
  std::cout <<"Checksum after 3 significant digits "<< csA.getCurrent() << std::endl;
  csA.add(A,1);
  csB.add(A,1);
  std::cout <<"Checksum after 1 significant digit "<< csA.getCurrent() << std::endl;
  ASSERT_EQ(csA.getCurrent(),csB.getCurrent());
  // Test old checksum algorithm for doubles, casting to float.
  // This has shown to be a less stable approach.
  csA.add(A);
  csB.add(B);
  ASSERT_EQ(csA.getCurrent(),csB.getCurrent());
  // Testing numbers larger than 10.0
  A *= 1000.0;
  B *= 1000.0;
  csA.add(A,10);
  csB.add(B,10);
  std::cout <<"Checksum with 10 significant digits "<< csA.getCurrent() << std::endl;
  ASSERT_EQ(csA.getCurrent(),csB.getCurrent());
  csA.add(A);
  csB.add(B);
  ASSERT_EQ(csA.getCurrent(),csB.getCurrent());
}


/*!
  Creates some string extension unit tests.
*/

TEST(TestFFa,String)
{
  char FixDof[6] = "#FixX";
  FixDof[4] += 1;
  std::cout <<"FixDof: \""<< FixDof <<"\""<< std::endl;
  EXPECT_FALSE(FFaString("jalla #FixX").hasSubString(FixDof));
  EXPECT_TRUE(FFaString("peder #FixY").hasSubString(FixDof));
}
