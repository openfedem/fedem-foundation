// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#include "FFrLib/FFrExtractor.H"
#ifdef FT_HAS_DIRENT
#include <dirent.h>
#endif
#include <string.h>
#include <iostream>
#ifdef FT_HAS_GTEST
#include "gtest.h"
#endif


std::string srcdir; //!< Absolute path to the source directory for this test program

extern size_t loadTest (const std::vector<std::string>& files,
                        int npath = 0, char** vpath = NULL);


#ifdef FT_HAS_DIRENT
bool getFiles (std::vector<std::string>& frsFiles, const char* path)
{
  DIR* dir = opendir(path);
  if (!dir)
  {
    perror(path);
    return false;
  }

  bool ok = true;
  struct dirent* ent;
  while (ok && (ent = readdir(dir)))
    if (ent->d_name[0] != '.')
    {
      std::string fname = std::string(path) + "/" + std::string(ent->d_name);
      if (ent->d_type == DT_DIR)
        ok = getFiles(frsFiles,fname.c_str());
      else if (fname.find(".frs") == fname.size()-4)
        frsFiles.push_back(fname);
    }

  closedir(dir);
  return ok;
}
#endif


/*!
  This main program can both be used to execute the predefined unit tests,
  and run manually by specifying the files to test as command-line arguments.
*/

int main (int argc, char** argv)
{
#ifdef FT_HAS_GTEST
  ::testing::InitGoogleTest(&argc,argv);
#endif

  // Extract the source directory of the tests to use as prefix for loading external files
  int i, status = 0;
  std::vector<std::string> frsFiles;
  for (i = 1; i < argc; i++)
    if (!strncmp(argv[i],"--srcdir=",9))
    {
      srcdir = argv[i]+9;
      std::cout <<"Note: Source directory = "<< srcdir << std::endl;
      if (srcdir.back() != '/') srcdir += '/';
    }
    else if (strstr(argv[i],".frs"))
      frsFiles.push_back(argv[i]);
    else
      break; // The remaining arguments are assumed to define a variable path to search for

  if (!frsFiles.empty()) // Check frs-files specified as command-line arguments
    status = frsFiles.size() - loadTest(frsFiles,argc-i,argv+i);

#ifdef FT_HAS_GTEST
  else if (i == argc) // No other arguments, run the parameterized unit tests
    status = RUN_ALL_TESTS();
#endif

#ifdef FT_HAS_DIRENT
  else while (i < argc)
  {
    // Run on directories specified on command-line
    frsFiles.clear();
    if (getFiles(frsFiles,argv[i++]) && !frsFiles.empty())
    {
      int a;
      status += frsFiles.size() - loadTest(frsFiles);
      std::cout <<"Continue? ";
      std::cin >> a;
    }
  }
#endif

  FFrExtractor::releaseMemoryBlocks(true);
  return status;
}


#ifdef FT_HAS_GTEST

class TestFFr : public testing::Test, public testing::WithParamInterface<const char*> {};

//! Create a parameterized test reading an frs-file, or a directory with frs-files.
TEST_P(TestFFr, Read)
{
  ASSERT_FALSE(srcdir.empty());

  // GetParam() will be substituted with the actual file- or directory name.
  std::string fileName = srcdir + GetParam();

  if (fileName.find(".frs") == fileName.size()-4)
    ASSERT_EQ(loadTest({fileName}),1U);
#ifdef FT_HAS_DIRENT
  else
  {
    std::vector<std::string> frsFiles;
    ASSERT_TRUE(getFiles(frsFiles,fileName.c_str()));
    ASSERT_EQ(frsFiles.size(),loadTest(frsFiles));
  }
#endif
}

//! Instantiate the test over a list of file names
INSTANTIATE_TEST_CASE_P(TestFFr, TestFFr,
                        testing::Values( "response_0001/timehist_prim_0001/th_p_1.frs",
                                         "response_0001/timehist_sec_0001/th_s_2.frs",
                                         "response_0001/eigval_0001/ev_p_3.frs",
                                         "response_0001/timehist_rcy_0001" ));

#endif
