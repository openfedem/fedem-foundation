// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

/*!
  \file FFaAlgebra.C
  \brief Auxiliary matrix-vector functions.
*/

#include "FFaLib/FFaAlgebra/FFaMat33.H"
#include "FFaLib/FFaAlgebra/FFaAlgebra.H"


/*!
  The implementation of this function is based on the Fortran routine trix30()
  in the file src/Femlib/beamaux.f
  \note The vector \a X is here assumed to point FROM the nodal point location
  TO the actual element location (this is opposite to the case in trix30()).
*/

void FFaAlgebra::eccTransform6 (double mat[6][6], const FaVec3& X)
{

  int i;

  for (i = 0; i < 6; i++)
  {
    mat[3][i] -= X[2]*mat[1][i] - X[1]*mat[2][i];
    mat[4][i] -= X[0]*mat[2][i] - X[2]*mat[0][i];
    mat[5][i] -= X[1]*mat[0][i] - X[0]*mat[1][i];
  }

  for (int i = 0; i < 6; i++)
  {
    mat[i][3] -= X[2]*mat[i][1] - X[1]*mat[i][2];
    mat[i][4] -= X[0]*mat[i][2] - X[2]*mat[i][0];
    mat[i][5] -= X[1]*mat[i][0] - X[0]*mat[i][1];
  }
}


/*!
  The transformation matrix consists of a 3&times;3 submatrix \b T, which is
  repeated along the diagonal (when \a node == 0).
  If \a node &gt; 0, the transformation matrix equals the identity matrix,
  but with the submatrix \b T inserted on the diagonal at position
  3*(\a node-1)+1 to 3*\a node.

  The implementation of this function is based on the Fortran routine mpro30()
  in the file src/Femlib/beamaux.f
*/

bool FFaAlgebra::congruenceTransform (double** mat, const FaMat33& T,
				      int N, int node)
{
  if (N < 1 || node > N) return false;

  int i, j, iA, jA, kA;
  double tmp, B[3][3];

  for (i = 0; i < N; i++)
    for (j = i; j < N; j++)
    {
      for (iA = 0; iA < 3; iA++)
        for (jA = 0; jA < 3; jA++)
          B[iA][jA] = 0.0;

      if (node < 1 || i == node-1)
	for (iA = 0; iA < 3; iA++)
	  for (kA = 0; kA < 3; kA++)
	  {
	    tmp = mat[3*j+kA][3*i+iA];
	    for (jA = 0; jA < 3; jA++)
	      B[jA][iA] += T[kA][jA] * tmp;
	  }

      else if (j == node-1)
	for (iA = 0; iA < 3; iA++)
	  for (jA = 0; jA < 3; jA++)
	    B[jA][iA] = mat[3*j+jA][3*i+iA];

      if (node < 1 || j == node-1)
	for (iA = 0; iA < 3; iA++)
	  for (kA = 0; kA < 3; kA++)
	  {
	    tmp = 0.0;
	    for (jA = 0; jA < 3; jA++)
	      tmp += B[kA][jA] * T[jA][iA];
	    mat[3*j+kA][3*i+iA] = tmp;
	  }

      else if (i == node-1)
	for (iA = 0; iA < 3; iA++)
	  for (jA = 0; jA < 3; jA++)
	    mat[3*j+jA][3*i+iA] = B[jA][iA];
    }

  for (i = 0; i < 3*N; i++)
    for (j = i+1; j < 3*N; j++)
      mat[i][j] = mat[j][i];

  return true;
}
