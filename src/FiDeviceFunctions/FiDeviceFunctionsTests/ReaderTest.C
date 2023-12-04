// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#include "FiDeviceFunctions/FiDeviceFunctionFactory.H"
#include "FiDeviceFunctions/FiCurveASCFile.H"
#include "FiDeviceFunctions/FiASCFile.H"
#include <iostream>
#include <cstring>
#include <cmath>
#ifdef FT_HAS_GTEST
#include "gtest.h"
#endif


std::string srcdir; //!< Absolute path to the source directory of this test


/*!
  \brief Loads data files into a FiDeviceFunctionFactory.
*/

int loadTest (const std::vector<std::string>& files, double* X = nullptr)
{
  char title[128], units[128];
  int handle, errors = 0;
  for (const std::string& fileName : files)
    if ((handle = FiDeviceFunctionFactory::instance()->open(fileName)) >= 0)
    {
      std::cout <<"   * Loaded file \""<< fileName <<"\" OK"<< std::endl;
      for (int a = 0; a < 2; a++)
      {
        // Extract and print out axis information
        FiDeviceFunctionFactory::instance()->getAxisTitle(handle,a,title,128);
        FiDeviceFunctionFactory::instance()->getAxisUnit(handle,a,units,128);
        if (strlen(title) > 0)
          std::cout <<"     "<< char('X'+a) <<"-axis: \""<< title
                    <<"\" ["<< units <<"]"<< std::endl;
      }
      // Extract and print out channel labels
      int chn = 0;
      std::vector<std::string> channels;
      if (FiDeviceFunctionFactory::instance()->getChannelList(handle,channels))
      {
        for (const std::string& channel : channels)
          std::cout <<"     Channel #"<< ++chn <<": "<< channel << std::endl;
        if (channels.size() > 1) chn = 2; // Test evaluation of channel 2
      }
      if (X)
      {
        int stat = 0; // Try to evaluate this function at the value of *X
        *X = FiDeviceFunctionFactory::instance()->getValue(handle,*X,stat,chn);
        if (stat < 0) errors++;
      }
    }
    else
    {
      std::cout <<" *** Failed to load file \""<< fileName <<"\""<< std::endl;
      errors++;
    }

  return errors;
}


/*!
  \brief Loads a data file and tries to integrate it.
*/

double integrateTest (const std::string& fileName, double X)
{
  int handle = FiDeviceFunctionFactory::instance()->open(fileName);
  if (handle > 0)
    std::cout <<"   * Loaded file \""<< fileName <<"\" OK"<< std::endl;
  else
  {
    std::cout <<" *** Failed to load file \""<< fileName <<"\""<< std::endl;
    return -1.0;
  }

  int stat = 1; // Try to integrate this function from 0.0 to X
  double fInt = FiDeviceFunctionFactory::instance()->getValue(handle,X,stat);
  return stat < 0 ? (double)stat : fInt;
}


/*!
  \brief Main program for the FiDeviceFunction unit tests.
  \details This program can both be used to execute the predefined unit tests,
  and run manually by specifying the files to test as command-line arguments.
*/

int main (int argc, char** argv)
{
#ifdef FT_HAS_GTEST
  ::testing::InitGoogleTest(&argc,argv);
#endif

  // Extract the source directory of the tests
  // to use as prefix for loading external files
  std::vector<std::string> files;
  for (int i = 1; i < argc; i++)
    if (!strncmp(argv[i],"--srcdir=",9))
    {
      srcdir = argv[i]+9;
      std::cout <<"Note: Source directory = "<< srcdir << std::endl;
      if (srcdir.back() != '/') srcdir += '/';
    }
    else if (argv[i][0] != '-')
      files.push_back(argv[i]);

  int status = 0;
  if (!files.empty()) // Check files specified as command-line arguments
    status = loadTest(files);
#ifdef FT_HAS_GTEST
  else // Run the parameterized unit tests
    status = RUN_ALL_TESTS();
#endif

  // Print out file information
  FiDeviceFunctionFactory::instance()->dump();
  // Clean up memory
  FiDeviceFunctionFactory::removeInstance();
  return status;
}


#ifdef FT_HAS_GTEST

struct Input
{
  const char* name; double x, y; size_t n = 0;
  Input(const char* f, double X = -9.9, double Y = 0.0,
        size_t N = 0) : name(f), x(X), y(Y), n(N) {}
};

//! Output stream operator used by gtest in case of failure.
std::ostream& operator<<(std::ostream& os, const Input& param)
{
  return os <<"\""<< param.name <<"\" "<< param.x <<" "<< param.y;
}


class TestFiDF : public testing::Test, public testing::WithParamInterface<Input> {};

//! Create a parameterized test reading one file.
TEST_P(TestFiDF, Read)
{
  ASSERT_FALSE(srcdir.empty());
  // Get evaluation point, if any.
  double fv = GetParam().x;
  double* X = fv < 0.0 ? nullptr : &fv;
  // Read the file and try to evaluate at given point X
  ASSERT_EQ(loadTest({srcdir + GetParam().name}, X),0);
  if (X)
  {
    EXPECT_FLOAT_EQ(fv,GetParam().y);
  }
}

//! Instantiate the test over a list of file names.
INSTANTIATE_TEST_CASE_P(Read, TestFiDF,
                        testing::Values(Input("data/onepoint.dat",0.1,1.0),
                                        Input("data/twopoints.dat",0.1,1.2),
                                        Input("data/fivepoints.dat",0.15,1.35),
                                        Input("data/24H_Encoder_Replicator_DOS.asc",42049.64,2.45),
                                        "data/kerb_lhfx.dac",
                                        "data/kerb_lhfy.dac",
                                        "data/kerb_lhfz.dac",
                                        "data/kerb_lhrx.dac",
                                        "data/kerb_lhry.dac",
                                        "data/kerb_lhrz.dac",
                                        "data/01_32402_33052.asc",
                                        "data/comma-separated.dat",
                                        Input("data/extfuncvalues.csv",0.2,2.7),
                                        Input("data/extfuncval1.csv",0.2,2.7),
                                        Input("data/extfuncval2.csv",0.2,2.7)));


class TestCurveASCII : public testing::Test, public testing::WithParamInterface<Input> {};

//! Create a parameterized test for FiCurveASCFile.
TEST_P(TestCurveASCII, Read)
{
  ASSERT_FALSE(srcdir.empty());
  FiCurveASCFile curve((srcdir + GetParam().name).c_str());
  ASSERT_TRUE(curve.open());
  double X = GetParam().x;
  double Y = GetParam().y;
  size_t N = GetParam().n;
  std::vector<double> x, y;
  curve.getRawData(x,y,0.0,0.0,0);
  ASSERT_EQ(x.size(),N);
  ASSERT_EQ(y.size(),N);
  double xmin = x.front();
  double xmax = x.back();
  for (size_t i = 0; i < N; i++)
  {
    if (x[i] > xmax) xmax = x[i];
    if (x[i] < xmin) xmin = x[i];
    if (fabs(x[i]-X) <= 1.0e-8)
    {
      EXPECT_NEAR(y[i],Y,1.0e-8);
      break;
    }
    if (i < 10)
      std::cout << x[i] <<" "<< y[i] << std::endl;
  }
  std::cout <<"xmin="<< xmin <<", xmax="<< xmax << std::endl;
  EXPECT_GT(xmax,xmin);
}

//! Instantiate the test over a list of file names.
INSTANTIATE_TEST_CASE_P(Read, TestCurveASCII,
                        testing::Values(Input("data/Brake_pressure.asc",0.319,0.590149042,200)));


//! Create another test reading a file and integrating it.
TEST(TestFiDF, Integrate)
{
  ASSERT_FALSE(srcdir.empty());

  const char* fileName[4] = { "data/03_03_FrontShock_ReboundStopFJTest.asc",
                              "data/03_03_FrontShock_ReboundStopFJTest1.asc",
                              "data/03_03_FrontShock_ReboundStopFJTest2.asc",
                              "data/03_03_FrontShock_ReboundStopFJTest3.asc" };

  std::vector<double> x({ 0.05, 0.1, 0.2 });
  std::vector<double> f({ 0.0, 15276.96, 94291750.0 });

  // Read the file and try to integrate from zero to the given points
  for (size_t i = 0; i < 3; i++)
  {
    EXPECT_FLOAT_EQ(integrateTest(srcdir+fileName[0],-x[i]),-f[i]);
    EXPECT_FLOAT_EQ(integrateTest(srcdir+fileName[1],-x[i]),-f[i]);
    EXPECT_FLOAT_EQ(integrateTest(srcdir+fileName[2], x[i]), f[i]);
    EXPECT_FLOAT_EQ(integrateTest(srcdir+fileName[3], x[i]), f[i]);
  }
}


//! Create another test reading an external function values file.
TEST(TestFiDF, ReadExtFunc)
{
  ASSERT_FALSE(srcdir.empty());

  FT_FILE fd = FT_open((srcdir+"data/extfuncvalues.dat").c_str(),FT_rb);
  ASSERT_NE(fd,static_cast<FT_FILE>(0));

  std::vector<std::string> header;
  FiASCFile::readHeader(fd,header);
  ASSERT_EQ(header.size(),5U);
  for (const std::string& ch : header) std::cout <<"\t"<< ch;
  std::cout << std::endl;

  std::vector<double> values;
  FiASCFile::readNext(fd,{1,3,5},values);
  for (double v : values) std::cout <<"\t"<< v;
  std::cout << std::endl;
  ASSERT_EQ(values.size(),3U);
  EXPECT_EQ(values[0],1.1);
  EXPECT_EQ(values[1],1.3);
  EXPECT_EQ(values[2],1.5);
  FiASCFile::readNext(fd,{2,4},values);
  for (double v : values) std::cout <<"\t"<< v;
  std::cout << std::endl;
  ASSERT_EQ(values.size(),2U);
  EXPECT_EQ(values[0],2.2);
  EXPECT_EQ(values[1],2.4);
  FiASCFile::readNext(fd,{3},values);
  for (double v : values) std::cout <<"\t"<< v;
  std::cout << std::endl;
  ASSERT_EQ(values.size(),1U);
  EXPECT_EQ(values[0],3.3);
  FT_close(fd);

  FiDeviceFunctionFactory* df = FiDeviceFunctionFactory::instance();
  ASSERT_EQ(df->open("jalla1",EXT_FUNC,IO_READ,1),0);
  ASSERT_EQ(df->open("jalla2",EXT_FUNC,IO_READ,2),0);
  ASSERT_EQ(df->open("jalla3",EXT_FUNC,IO_READ,3),0);
  ASSERT_TRUE(df->initExtFuncFromFile((srcdir+"data/extfuncvalues.csv").c_str(),
                                      "<Func1,Func3,Func4>"));
  df->updateExtFuncFromFile();
  EXPECT_EQ(df->getExtFunc(1),1.1);
  EXPECT_EQ(df->getExtFunc(2),1.3);
  EXPECT_EQ(df->getExtFunc(3),1.4);
  df->updateExtFuncFromFile();
  EXPECT_EQ(df->getExtFunc(1),2.1);
  EXPECT_EQ(df->getExtFunc(2),2.3);
  EXPECT_EQ(df->getExtFunc(3),2.4);
  ASSERT_TRUE(df->initExtFuncFromFile((srcdir+"data/extfuncval1.csv").c_str(),
                                      "<Func1,Func3,Func4>"));
  df->updateExtFuncFromFile();
  EXPECT_EQ(df->getExtFunc(1),1.1);
  EXPECT_EQ(df->getExtFunc(2),1.3);
  EXPECT_EQ(df->getExtFunc(3),1.4);
  df->updateExtFuncFromFile();
  EXPECT_EQ(df->getExtFunc(1),2.1);
  EXPECT_EQ(df->getExtFunc(2),2.3);
  EXPECT_EQ(df->getExtFunc(3),2.4);
  ASSERT_TRUE(df->initExtFuncFromFile((srcdir+"data/extfuncval2.csv").c_str(),
                                      "<Func1,Func3,Func4>"));
  df->updateExtFuncFromFile();
  EXPECT_EQ(df->getExtFunc(1),1.1);
  EXPECT_EQ(df->getExtFunc(2),1.3);
  EXPECT_EQ(df->getExtFunc(3),1.4);
  df->updateExtFuncFromFile();
  EXPECT_EQ(df->getExtFunc(1),2.1);
  EXPECT_EQ(df->getExtFunc(2),2.3);
  EXPECT_EQ(df->getExtFunc(3),2.4);
}


//! Create another test reading the same channel twice.
TEST(TestFiDF, ReadTwice)
{
  ASSERT_FALSE(srcdir.empty());

  std::string fileName = srcdir + "data/01_32402_33052.asc";
  FiDeviceFunctionFactory* df = FiDeviceFunctionFactory::instance();
  ASSERT_GT(df->open(fileName.c_str(),UNKNOWN_FILE,IO_READ,3),0);
  ASSERT_GT(df->open(fileName.c_str(),UNKNOWN_FILE,IO_READ,3),0);
}

#endif
