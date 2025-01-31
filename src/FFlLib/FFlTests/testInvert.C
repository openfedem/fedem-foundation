// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

/*!
  \file testInvert.C
  \brief Unit tests for element searching and mapping invertion.
*/

#include "gtest.h"
#include "FFlLib/FFlLinkHandler.H"
#include "FFlLib/FFlFEParts/FFlShells.H"
#include "FFlLib/FFlFEParts/FFlNode.H"
#include "FFaLib/FFaAlgebra/FFaVec3.H"
#include <array>

#ifdef FF_NAMESPACE
using namespace FF_NAMESPACE;
#endif


/*!
  \brief This class creates a shell FE model with some random geometry.
*/

class RandomShell : public FFlLinkHandler
{
public:
  //! \brief The default constructor creates a hard-coded mesh.
  RandomShell()
  {
    // Lambda function generating a QUAD4-element
    auto&& addQuad = [this](int eid, int n1, int n2, int n3, int n4)
    {
      FFlElementBase* elm = new FFlQUAD4(eid);
      elm->setNode(1,n1);
      elm->setNode(2,n2);
      elm->setNode(3,n3);
      elm->setNode(4,n4);
      this->addElement(elm);
    };

    // Lambda function generating a TRI3-element
    auto&& addTria = [this](int eid, int n1, int n2, int n3)
    {
      FFlElementBase* elm = new FFlTRI3(eid);
      elm->setNode(1,n1);
      elm->setNode(2,n2);
      elm->setNode(3,n3);
      this->addElement(elm);
    };

    // Nodal coordinates of the test grid
    const std::vector<double> XYZ = {
      0.0, 0.0, 0.0,
      1.0, 0.0, 0.0,
      0.0, 1.0, 0.0,
      1.0, 1.0, 0.0,
      0.0, 2.0, 0.1,
      1.9, 0.0, 0.0,
      1.2, 0.3, 0.1,
      2.0, 0.25,0.15,
      1.5, 0.6, 0.2
    };

    int nID = 0, eID = 0;
    for (size_t i = 0; i+2 < XYZ.size(); i += 3)
      this->addNode(new FFlNode(++nID,FaVec3(XYZ[i],XYZ[i+1],XYZ[i+2])));

    // Define the mesh topology.
    //        eID n1 n2 n3 n4
    addQuad(++eID, 1, 2, 4, 3);
    addTria(++eID, 3, 4, 5);
    addQuad(++eID, 2, 6, 8, 7);
    addTria(++eID, 7, 8, 9);

    this->resolve();
  }
};


/*!
  \brief Helper function to clean up element type singeltons.
*/

template<class T> void releaseElement()
{
  FFaSingelton<FFlFEElementTopSpec,T>::removeInstance();
  FFaSingelton<FFlFEAttributeSpec,T>::removeInstance();
  FFaSingelton<FFlTypeInfoSpec,T>::removeInstance();
}


/*!
  \brief Main program for the mapping invert unit test executable.
*/

int main (int argc, char** argv)
{
  ::testing::InitGoogleTest(&argc,argv);

  FFlNode::init();
  FFlTRI3::init();
  FFlQUAD4::init();

  FFlShellElementBase::offPlaneTol = 1.0;

  int status = RUN_ALL_TESTS();

  releaseElement<FFlTRI3>();
  releaseElement<FFlQUAD4>();
  ElementFactory::removeInstance();
  FFaSingelton<FFlTypeInfoSpec,FFlNode>::removeInstance();

  return status;
}


TEST(TestFFl,Mapping)
{
  RandomShell feModel;

  // Lambda function testing the mapping invertion at specified point
  auto&& checkPoint = [&feModel](double x, double y, double z)
  {
    double xi[3];
    FaVec3 X(x,y,z);
    std::cout <<"Searching for "<< X << std::endl;
    FFlElementBase* elm = feModel.findPoint(X,xi);
    ASSERT_TRUE(elm != NULL);

    FaVec3 X0 = elm->mapping(xi[0],xi[1]);
    std::cout <<"Found element "<< elm->getID()
              <<" xi = "<< xi[0] <<" "<< xi[1]
              <<" ---> "<< X0 << std::endl;
    if (X.equals(X0)) return; // The point searched for is in the shell plane

    // Calculate the normal vector of the shell element
    std::vector<FaVec3> v(1+elm->getNodeCount());
    for (int n = 1; n <= elm->getNodeCount(); n++)
      v[n] = elm->getNode(n)->getPos();
    if (v.size() == 5)
      v.front() = (v[3]-v[1]) ^ (v[4]-v[2]);
    else
      v.front() = (v[2]-v[1]) ^ (v[3]-v[1]);

    // Check that the found point is the projection of
    // the given point onto the shell surface
    std::cout <<"Normal = "<< v.front() <<"\nX - X0 = "<< X-X0 << std::endl;
    EXPECT_NE(v.front().isParallell(X-X0),0);
  };

  checkPoint(0.5, 1.5, -0.1);
  checkPoint(0.4, 1.31, 0.0);
  checkPoint(1.4, 0.35, 0.0);
  checkPoint(0.3, 0.40, 0.1);
  checkPoint(1.3, 0.15, 0.2);
}


//! \brief Data type to instantiate particular units tests over.
typedef std::array<double,15> Case;


//! \brief Class describing a parameterized unit test instance.
class TestFFl : public testing::Test, public testing::WithParamInterface<Case> {};


/*!
  Creates a parameterized test solving a bilinear equation system.
  GetParam() will be substituted with the actual equation parameters.
*/

TEST_P(TestFFl, Invert)
{
  std::array<FaVec3,5> X;
  for (size_t i = 0; i < X.size(); i++)
    for (size_t j = 0; j < 3; j++)
      X[i][j] = GetParam()[3*i+j];

  FFlNode n1(1,X[0]);
  FFlNode n2(2,X[1]);
  FFlNode n3(3,X[2]);
  FFlNode n4(4,X[3]);
  FaVec3& Xp = X[4];
  FFlQUAD4 elm(123);
  elm.setNode(1,&n1);
  elm.setNode(2,&n2);
  elm.setNode(3,&n3);
  elm.setNode(4,&n4);

  double xi[3];
  ASSERT_TRUE(elm.invertMapping(Xp,xi));
  std::cout <<"Found xi,eta = "<< xi[0] <<" "<< xi[1]
            <<" --> X = "<< elm.mapping(xi[0],xi[1]) << std::endl;
}


/*!
  Instantiate the test over some parameters.
*/

INSTANTIATE_TEST_CASE_P(TestInvert, TestFFl, // X        Y      Z
                        testing::Values(Case{ 6.0    , -16.4, -25.75,
                                              4.81463, -16.4, -25.75,
                                              4.79082, -17.8, -25.75,
                                              6.0    , -17.8, -25.75,
                                              6.0    , -16.4, -27.00 } ));
