// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

/*!
  \file searchTest.C
  \brief Unit testing for FFrLib.
*/

#include "FFrLib/FFrExtractor.H"
#include "FFrLib/FFrVariableReference.H"
#include "FFaLib/FFaDefinitions/FFaResultDescription.H"
#include <iostream>

#include "gtest/gtest.h"

extern std::string srcdir;

extern size_t loadTest (const std::vector<std::string>& files,
                        int npath = 0, char** vpath = NULL);


/*!
  \brief Creates a test that searches for a variable.
*/

TEST(TestFFr, ReadAndSearch)
{
  ASSERT_FALSE(srcdir.empty());

  std::string fileName = srcdir + "response_0001/timehist_sec_0001/th_s_2.frs";
  const char* varpath[2] = { "Triad", "Velocity" };
  ASSERT_EQ(loadTest({fileName},2,const_cast<char**>(varpath)),1U);
}


/*!
  \brief Creates a test that disables and enables frs-files while searching.
*/

TEST(TestFFr, EnableDisable)
{
  ASSERT_FALSE(srcdir.empty());

  std::vector<std::string> files(3);
  files[0] = srcdir + "response_0002/timehist_prim_0001/th_p_1.frs";
  files[1] = srcdir + "response_0002/timehist_sec_0001/th_s_2.frs";
  files[2] = srcdir + "response_0002/freqdomain_0001/fd_p_3.frs";
  FFrExtractor* res = new FFrExtractor("RDB reader");
  std::vector<FFrEntryBase*> entries;

  // Lambda function searching for a variable
  auto&& searchVar = [res,&entries](const FFaResultDescription& descr)
  {
    std::cout <<"\nSearching for "<< descr << std::flush;
    res->search(entries,descr);
    for (FFrEntryBase* entry : entries)
    {
      entry->printPosition(std::cout);
      EXPECT_STREQ(entry->getEntryDescription().getText().c_str(),
                   descr.getText().c_str());
    }
    return entries.size();
  };

  FFaTimeDescription   timeVar;
  FFaResultDescription tposVar("Triad",114,5);
  tposVar.varDescrPath = { "Position matrix" };
  tposVar.varRefType   =   "TMAT34";

  // Add all files to the extractor and search for time and position variables.
  // Also check that the variables found have containers asssociated with them.
  ASSERT_TRUE(res->addFiles(files,false,true));
#ifdef FFR_DEBUG
  res->printHierarchy();
#endif
  ASSERT_EQ(searchVar(timeVar),1U);
  EXPECT_FALSE(entries.front()->isEmpty());
  ASSERT_EQ(searchVar(tposVar),2U);
  EXPECT_FALSE(entries.front()->isEmpty());
  EXPECT_FALSE(entries.back()->isEmpty());

  // Unload the frequency response file and search again
  ASSERT_TRUE(res->removeFiles({files[2]}));
#ifdef FFR_DEBUG
  res->printHierarchy();
#endif
  ASSERT_EQ(searchVar(timeVar),1U);
  EXPECT_FALSE(entries.front()->isEmpty());
  ASSERT_EQ(searchVar(tposVar),1U);
  EXPECT_FALSE(entries.front()->isEmpty());

  // Unload the primary time history response file and search again
  ASSERT_TRUE(res->removeFiles({files[0]}));
#ifdef FFR_DEBUG
  res->printHierarchy();
#endif
  ASSERT_EQ(searchVar(timeVar),1U);
  EXPECT_FALSE(entries.front()->isEmpty());
  ASSERT_EQ(searchVar(tposVar),0U);

  // Reload the frequency response file and search again
  ASSERT_TRUE(res->addFile(files[2],true));
#ifdef FFR_DEBUG
  res->printHierarchy();
#endif
  ASSERT_EQ(searchVar(timeVar),1U);
  EXPECT_FALSE(entries.front()->isEmpty());
  ASSERT_EQ(searchVar(tposVar),1U);
  EXPECT_FALSE(entries.front()->isEmpty());

  // Unload all files and search again
  std::set<std::string> fileSet(files.begin(),files.end());
  ASSERT_TRUE(res->removeFiles(fileSet));
#ifdef FFR_DEBUG
  res->printHierarchy();
#endif
  // The time variable (which is a top-level variable), should still be around
  // but with no container associated with it. The others should be gone.
  ASSERT_EQ(searchVar(timeVar),1U);
  EXPECT_TRUE(entries.front()->isEmpty());
  ASSERT_EQ(searchVar(tposVar),0U);

  std::cout <<"\nFinished. Cleaning up..."<< std::endl;
  delete res;
  FFrExtractor::releaseMemoryBlocks();
  std::cout <<"\nDone."<< std::endl;
}


/*!
  \brief Creates a test loading the possibility files with wild-cards.
*/

TEST(TestFFr, Possibility)
{
  ASSERT_FALSE(srcdir.empty());

  FFrExtractor* res = new FFrExtractor("Possibility reader");
  ASSERT_TRUE(res->addFile(srcdir+"response_pos.frs",true));
  ASSERT_TRUE(res->addFile(srcdir+"stress_pos.frs",true));
#ifdef FFR_DEBUG
  res->printHierarchy();
#endif

  // Set up search paths with wild-cards "*" to get all relevant entries
  FFaResultDescription IGDesc1("Part"), IGDesc2("Part"), IGDesc3("Part");
  IGDesc1.varDescrPath = { "Nodes", "*", "*" };
  IGDesc2.varDescrPath = { "Elements", "*", "*", "Element", "*" };
  IGDesc3.varDescrPath = { "Elements", "*", "*", "Element nodes", "*", "1" };

  // Search the results database for matching entries
  std::vector<FFrEntryBase*> entries;
  res->search(entries,IGDesc1);
  ASSERT_EQ(entries.size(),1U);
  EXPECT_STREQ(entries[0]->getDescription().c_str(),"Dynamic response");
  res->search(entries,IGDesc2);
  ASSERT_EQ(entries.size(),3U);
  EXPECT_STREQ(entries[0]->getDescription().c_str(),"Basic");
  EXPECT_STREQ(entries[1]->getDescription().c_str(),"Top");
  EXPECT_STREQ(entries[2]->getDescription().c_str(),"Bottom");
  res->search(entries,IGDesc3);
  ASSERT_EQ(entries.size(),6U);
  for (FFrEntryBase* entry : entries)
    EXPECT_STREQ(entry->getDescription().c_str(),"1");

  delete res;
  FFrExtractor::releaseMemoryBlocks();
}


/*!
  \brief Creates a test for overlapping frs-files from restart.
*/

TEST(TestFFr, Restart)
{
  ASSERT_FALSE(srcdir.empty());

  std::vector<std::string> files(2);
  files[0] = srcdir + "response_0004/timehist_prim_0001/th_p_1.frs";
  files[1] = srcdir + "response_0004/timehist_prim_0001/th_p_3.frs";
  FFrExtractor* res = new FFrExtractor("RDB reader");

  // Lambda function searching for a unique variable
  auto&& searchVar = [res](const FFaResultDescription& descr) -> FFrVariableReference*
  {
    std::cout <<"\nSearching for "<< descr << std::flush;
    std::vector<FFrEntryBase*> entries;
    res->search(entries,descr);
    EXPECT_EQ(entries.size(),1U);
    FFrVariableReference* var = dynamic_cast<FFrVariableReference*>(entries.front());
    EXPECT_TRUE(var != NULL);
    var->printPosition(std::cout);
    EXPECT_STREQ(var->getEntryDescription().getText().c_str(),descr.getText().c_str());
    return var;
  };

  FFaTimeDescription   timeVar;
  FFaResultDescription tposVar("Triad",17,2);
  tposVar.varDescrPath = { "Position matrix" };
  tposVar.varRefType   =   "TMAT34";

  // Add all files to the extractor and search for time and position variables.
  // Also check the variables found have 2 containers asssociated with them.
  ASSERT_TRUE(res->addFiles(files,false,true));
#ifdef FFR_DEBUG
  res->printHierarchy();
#endif
  EXPECT_EQ(searchVar(timeVar)->containers.size(),2U);
  EXPECT_EQ(searchVar(tposVar)->containers.size(),2U);

  std::cout <<"\nFinished. Cleaning up..."<< std::endl;
  delete res;
  FFrExtractor::releaseMemoryBlocks();
  std::cout <<"\nDone."<< std::endl;
}
