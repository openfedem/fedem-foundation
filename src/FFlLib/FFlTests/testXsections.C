// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

/*!
  \file testXsections.C
  \brief Unit test for the FFlCrossSection class.
*/

#include "gtest/gtest.h"

#include "FFlLib/FFlIOAdaptors/FFlCrossSection.H"
#include <cmath>


/*!
  \brief Creates a unit test for the FFlCrossSection class.
*/

TEST(TestFFl,CrossSection)
{
  const double pi = acos(-1.0);
  const double tol = 1.0e-15;
  const double tol2 = 1.0e-6;

  FFlCrossSection Rod("ROD",{1.0});
  FFlCrossSection Tube("TUBE",{1.0,0.9});
  FFlCrossSection Bar("BAR",{1.0,2.0});
  FFlCrossSection Box("BOX",{1.0,2.0,0.2,0.1});
  FFlCrossSection I("I",{2.0,1.0,1.2,0.1,0.2,0.3});
  FFlCrossSection T("T",{1.0,2.0,0.2,0.1});
  FFlCrossSection L("L",{1.0,2.0,0.2,0.15});
  EXPECT_NEAR(Rod.A,pi,tol);
  EXPECT_NEAR(Tube.A,pi*0.19,tol);
  EXPECT_NEAR(Bar.A,2.0,tol);
  EXPECT_NEAR(Box.A,0.72,tol);
  EXPECT_NEAR(I.A,0.71,tol);
  EXPECT_NEAR(I.Izz,0.434189,tol2);
  EXPECT_NEAR(I.Iyy,0.0599917,tol2);
  EXPECT_NEAR(T.A,0.38,tol);
  EXPECT_NEAR(T.Izz,0.144004,tol2);
  EXPECT_NEAR(T.Iyy,0.0168167,tol2);
  EXPECT_NEAR(L.A,0.47,tol);
  EXPECT_GT(L.findMainAxes(),0.0);

  std::cout <<"Rod :"<< Rod << std::endl;
  std::cout <<"Tube:"<< Tube << std::endl;
  std::cout <<"Bar :"<< Bar << std::endl;
  std::cout <<"Box :"<< Box << std::endl;
  std::cout <<"I   :"<< I << std::endl;
  std::cout <<"T   :"<< T << std::endl;
  std::cout <<"L   :"<< L << std::endl;
}
