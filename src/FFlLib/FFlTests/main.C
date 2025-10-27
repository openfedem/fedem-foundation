// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#include "FFlLib/FFlInit.H"
#include "FFlLib/FFlLinkHandler.H"
#include "FFlLib/FFlElementBase.H"
#include "FFlLib/FFlGroup.H"
#include "FFlLib/FFlFEParts/FFlNode.H"
#include "FFlLib/FFlIOAdaptors/FFlReaders.H"
#include "FFlLib/FFlIOAdaptors/FFlFedemWriter.H"
#include "FFaLib/FFaAlgebra/FFaUnitCalculator.H"
#include "FFaLib/FFaAlgebra/FFaTensor3.H"
#include <iterator>
#include <cstring>

#ifdef FF_NAMESPACE
using namespace FF_NAMESPACE;
#endif

int convertToFtl(const char* fname);


/*!
  \brief Test/demo program for FFlLib.

  \details
  This function evaluates the link checksum (used to check whether the data
  has been modified compared with previous versions), dumps out a summary of
  the model content including the mass properties, and exports the model to
  the Fedem internal file format (FTL).
*/

int test1 (const FFlLinkHandler& link, int csType, bool newcs = true)
{
  // Evaluate the link checksum
  std::cout <<"Link checksum = "
            << link.calculateChecksum(csType,newcs) << std::endl;

  // Print out a summary of the model to console
  std::cout <<"---\nLink summary follows:"<< std::endl;
  link.dump();

  double     mass;
  FaVec3     CoG;
  FFaTensor3 Inertia;
  link.getMassProperties(mass,CoG,Inertia);
  std::cout <<"\nLink mass    = "<< mass
            <<"\nLink CoG     = "<< CoG
            <<"\nLink inertia = "<< Inertia
            <<"\n"<< std::endl;

  // Export the link to the file "ut1.ftl"
  std::cout <<"Writing ut1.ftl"<< std::endl;
  FFlFedemWriter fedem(&link);
  if (!fedem.write("ut1.ftl"))
    return 2; // Failure

  std::cout <<"Done."<< std::endl;
  return 0; // Success
}


/*!
  \brief Test/demo program for FFlLib.

  \details
  This function applies a unit conversion and exports the converted model to
  the Fedem internal file format (FTL).
*/

int test2 (FFlLinkHandler& link)
{
  // Load unit conversions
  FFaUnitCalculator len("TestCalc");
  len.addConversion("LENGTH",1000.0,"m","mm");
  FFaUnitCalculatorProvider::instance()->addCalculator(len);
  FFaUnitCalculatorProvider::instance()->readCalculatorDefs("units.fcd");
  FFaUnitCalculatorProvider::instance()->printCalculatorDefs("myCalcs.fcd");

  std::vector<std::string> convs;
  FFaUnitCalculatorProvider::instance()->getCalculatorNames(convs);
  std::cout <<"\nAvailable unit conversions:\n";
  std::copy(convs.begin(),convs.end(),
            std::ostream_iterator<std::string>(std::cout,"\n"));

  // Select which one to use
  size_t ical = 0;
  std::cout <<"\nSelect [1-"<< convs.size() <<"] : "<< std::flush;
  std::cin >> ical;
  if (ical < 1 || ical > convs.size())
    return 2;

  // Apply unit conversion to the link data
  const std::string& conv = convs[ical-1];
  const FFaUnitCalculator* uc = FFaUnitCalculatorProvider::instance()->getCalculator(conv);
  if (uc)
    link.convertUnits(uc);
  else
  {
    std::cerr <<"Failed to initialize unit calculator "<< conv << std::endl;
    return 3;
  }

  // Export the link to the file "ut2.ftl"
  std::cout <<"\nWriting ut2.ftl"<< std::endl;
  FFlFedemWriter fedem(&link);
  if (!fedem.write("ut2.ftl"))
    return 4; // Failure

  std::cout <<"Done."<< std::endl;
  return 0; // Success
}


/*!
  \brief Test/demo program for FFlLib.
  \details This function traverses the model writing out some key information.
*/

int test3 (const FFlLinkHandler& link, const std::vector<int>& elms)
{
  // Traverse the elements
  for (ElementsCIter eit = link.elementsBegin(); eit != link.elementsEnd(); ++eit)
  {
    if (!elms.empty()) // Print only the specified elements
      if (std::find(elms.begin(),elms.end(),(*eit)->getID()) == elms.end())
        continue;

    std::cout <<"Element ID "<< (*eit)->getID()
              <<" is of type "<< (*eit)->getTypeName()
              <<" and has "<< (*eit)->getNodeCount() <<" nodes. (";
    for (NodeCIter nit = (*eit)->nodesBegin(); nit != (*eit)->nodesEnd(); ++nit)
      std::cout <<" "<< (*nit)->getID();
    std::cout <<" )"<< std::endl;

    // Traverse the element faces
    std::vector<FFlNode*> nodes;
    for (short int f = 1; (*eit)->getFaceNodes(nodes,f); f++)
    {
      std::cout <<"\tFace "<< f <<":";
      for (FFlNode* node : nodes) std::cout <<" "<< node->getID();
      std::cout <<"\n";
    }

    // Calculate mass and inertia about BBcg
    double     Me;
    FaVec3     Xec;
    FFaTensor3 Ie;
    if ((*eit)->getMassProperties(Me,Xec,Ie))
      std::cout <<"\tElement mass: "<< Me
                <<" CoG = "<< Xec << std::endl;
  }

  std::cout <<"Done."<< std::endl;
  return 0; // Success
}


/*!
  \brief Test/demo program for FFlLib.
  \details This function defines a group of elements within a specified sphere.
*/

int test4 (FFlLinkHandler& link, int argc, char** argv)
{
  FaVec3 X0, X1;
  double R = 1.0;
  bool shrnk = false;
  for (int i = 0; i < argc; i++)
    if (i == 0)
      R = atof(argv[i]);
    else if (i <= 3)
      X0[i-1] = atof(argv[i]);
    else if (i == 4)
    {
      X1[1] = atof(argv[i]);
      X1[0] = X0[2];
      X0[2] = X0[1];
      X0[1] = X0[0];
      X0[0] = R;
      R = 0.0;
    }
    else if (i == 5)
      X1[2] = atof(argv[i]);
    else if (i == 6 && !strncmp(argv[i],"-shr",4))
      shrnk = true;

  std::cout <<"Searching for elements within";
  if (R > 0.0)
    std::cout <<" R = "<< R <<" of point "<< X0 << std::endl;
  else
    std::cout <<" X0 = "<< X0 <<", X1 = "<< X1 << std::endl;

  // Lambda function checking if a point is in the group domain
  auto&& inDomain = [R,X0,X1](const FaVec3& XC)
  {
    if (R > 0.0)
      return (XC-X0).length() <= R;
    else for (int j = 0; j < 3; j++)
      if (XC[j] < X0[j] || XC[j] > X1[j])
        return false;
    return true;
  };

  // Traverse the elements
  FFlGroup* newGroup = new FFlGroup(1234, R > 0.0 ? "Sphere domain":"Box domain");
  for (ElementsCIter eit = link.elementsBegin(); eit != link.elementsEnd(); ++eit)
    if (inDomain((*eit)->getNodeCenter()))
      newGroup->addElement(*eit);

  newGroup->sortElements();
  std::cout <<"Found "<< newGroup->size() <<" elements."<< std::endl;

  FFlLinkHandler* output = &link;
  if (shrnk)
  {
    // Write out the element group as a new part
    output = new FFlLinkHandler(*newGroup);
    delete newGroup;
  }
  else
    link.addGroup(newGroup);

  // Export the link to the file "ut4.ftl"
  std::cout <<"Writing ut4.ftl"<< std::endl;
  FFlFedemWriter fedem(output);
  if (!fedem.write("ut4.ftl"))
    return 2; // Failure

  if (shrnk) delete output;
  std::cout <<"Done."<< std::endl;
  return 0; // Success
}


/*!
  \brief Main program for FFlLib tests.

  \details
  The various tests for different features are implemented as functions,
  that might be selected through the second command-line option.
*/

int main (int argc, char** argv)
{
  FFlInit initializer; // Initialize the FE library
  FFlLinkHandler link; // Container for a FE link
  std::vector<int> el; // List of specific elements to visit

  // Read and interpret the link data file
  if (argc > 1)
  {
    if (argc > 2 && strcmp(argv[argc-1],"linear") == 0)
    {
      FFlReaders::convertToLinear = 1;
      --argc;
    }
    else if (argc > 2 && strncmp(argv[argc-1],"subdiv",6) == 0)
    {
      FFlReaders::convertToLinear = 2;
      --argc;
    }
    if (FFlReaders::instance()->read(argv[1],&link) > 0)
      std::cout <<"Read done.\n---"<< std::endl;
    else
      return convertToFtl(argv[1]);
  }

  switch (argc > 2 ? atoi(argv[2]) : (argc > 1)) {
  case 0: return test1(link, FFl::CS_NOGROUPINFO | FFl::CS_NOSTRCINFO | FFl::CS_NOVISUALINFO);
  case 1: return test1(link, 0, argc > 3 ? !strncmp(argv[3],"new",3) : false);
  case 2: return test2(link);
  case 3:
    for (int i = 3; i < argc; i++)
      el.push_back(atoi(argv[i]));
    return test3(link,el);
  case 4: return test4(link, argc-3, argv+3);
  case 5:
    for (int i = 3; i+2 < argc; i += 3)
    {
      FaVec3 X(atof(argv[i]),atof(argv[i+1]),atof(argv[i+2]));
      std::vector<FFlTypeInfoSpec::Cathegory> shell{FFlTypeInfoSpec::SHELL_ELM};
      if (FFlElementBase* e = link.findClosestElement(X,shell); e)
      {
        std::cout <<"The closest element to point "<< X <<" is\t"<< e->getID();
        double     Me;
        FFaTensor3 Ie;
        FaVec3     Xc;
        if (e->getMassProperties(Me,Xc,Ie))
          std::cout <<", CoG = "<< Xc;
        std::cout << std::endl;
      }
    }
    return 0;
  }

  std::cout <<"usage: "<< argv[0] <<" <linkfile> [num] [newCS] [linear|subdivide]\n";
  return 0;
}
