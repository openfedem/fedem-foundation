// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#include <cctype>

#include "FFaLib/FFaAlgebra/FFa3DLocation.H"
#include "FFaLib/FFaAlgebra/FFaMat34.H"
#include "FFaLib/FFaAlgebra/FFaMath.H"
#include "FFaLib/FFaDefinitions/FFaMsg.H"


FFa3DLocation::FFa3DLocation(bool saveNumData)
{
  myPosTyp = CART_X_Y_Z;
  myRotTyp = EUL_Z_Y_X;
  saveNumericalData = saveNumData;
}

FFa3DLocation::FFa3DLocation(PosType t, const FaVec3& v0,
			     RotType r, const FaVec3& v1, const FaVec3& v2)
{
  myPosTyp = t;
  myRotTyp = r;
  myL[0] = v0;
  myL[1] = v1;
  myL[2] = v2;
  saveNumericalData = true;
}

FFa3DLocation::FFa3DLocation(PosType t, const FaVec3& v0,
			     RotType r, const FaVec3& v1)
{
  myPosTyp = t;
  myRotTyp = r;
  myL[0] = v0;
  myL[1] = v1;
  saveNumericalData = true;

  if (this->getNumFields() == 9)
    std::cerr <<"FFa3DLocation constructor: Second rotation definition vector "
	      <<"is set to zero (possibly logic error?)"<< std::endl;
}

FFa3DLocation::FFa3DLocation(const FaMat34& m)
{
  myPosTyp = CART_X_Y_Z;
  myRotTyp = DIR_EX_EXY;
  myL[0] = m[3];
  myL[1] = m[0];
  myL[2] = m[1];
  saveNumericalData = true;
}


/*!
  Local operators.
*/

FFa3DLocation& FFa3DLocation::operator= (const FFa3DLocation& m)
{
  if (this == &m)
    return *this;

  myPosTyp = m.myPosTyp;
  myRotTyp = m.myRotTyp;
  myL[0] = m.myL[0];
  myL[1] = m.myL[1];
  myL[2] = m.myL[2];
  // JJS: Note that saveNumericalData is NOT copied.
  // That is meta data valid for one particular instance only.
  return *this;
}

FFa3DLocation& FFa3DLocation::operator= (const FaMat34& m)
{
  this->set(myPosTyp, myRotTyp, m);
  return *this;
}


bool FFa3DLocation::isCoincident(const FFa3DLocation& m) const
{
  return this->getMatrix().isCoincident(m.getMatrix(),1.0e-7);
}


/*!
  Converts the position representation in \a *this to the provided type.
*/

bool FFa3DLocation::changePosType(PosType newType)
{
  if (newType == myPosTyp) return false;

  this->setPos(newType, this->translation());

  return true;
}


/*!
  Converts the rotational representation in \a *this to the provided type.
*/

bool FFa3DLocation::changeRotType(RotType newType)
{
  if (newType == myRotTyp) return false;

  this->setRot(newType, this->direction());

  return true;
}


/*!
  Changes the imaginary reference CS for the translation numbers stored in this
  3DLocation to \a newRef from \a oldRef. It is only a convenience method.
*/

FFa3DLocation& FFa3DLocation::changePosRefCS(const FaMat34& newRef,
					     const FaMat34& oldRef)
{
#if FFA_DEBUG > 1
  static int count = 0;
  std::cout <<"\nFFa3DLocation::changePosRefCS("<< ++count <<")"
            <<"\nnewRef ="<< newRef <<"\noldRef ="<< oldRef << std::endl;
#endif

  static FaMat34 globMx;
  static FaMat34 newRelMx;

  globMx = oldRef * this->getMatrix();
  newRelMx = newRef.inverse() * globMx;
  this->setPos(myPosTyp, newRelMx.translation());

  return *this;
}


/*!
  Changes the imaginary reference CS for the rotation numbers stored in this
  3DLocation to \a newRef from \a oldRef. It is only a convenience method.
*/

FFa3DLocation& FFa3DLocation::changeRotRefCS(const FaMat34& newRef,
					     const FaMat34& oldRef)
{
#if FFA_DEBUG > 1
  static int count = 0;
  std::cout <<"\nFFa3DLocation::changeRotRefCS("<< ++count <<")"
            <<"\nnewRef ="<< newRef.direction()
            <<"\noldRef ="<< oldRef.direction() << std::endl;
#endif

  static FaMat33 globMx;
  static FaMat33 newRelMx;

  globMx = oldRef.direction() * this->direction();
  newRelMx = newRef.direction().transpose() * globMx;
  this->setRot(myRotTyp, newRelMx);

  return *this;
}


/*!
  Sets this to contain the position of \a globalPosition relative to \a posRelMx
  stored as type \a p, and the rotation of \a globalPosition relative to
  \a rotRelMx stored as type \a r.
*/

FFa3DLocation& FFa3DLocation::set(PosType p, const FaMat34& posRelMx,
                                  RotType r, const FaMat34& rotRelMx,
				  const FaMat34& globalPosition)
{
#if FFA_DEBUG > 1
  static int count = 0;
  std::cout <<"\nFFa3DLocation::set("<< ++count
            <<") PosType="<< p <<", RotType="<< r
            <<"\nposRelMx ="<< posRelMx <<"\nrotRelMx ="<< rotRelMx
            <<"\nglobalPosition ="<< globalPosition << std::endl;
#endif

  static FaMat34 tmpMx;

  tmpMx = posRelMx.inverse() * globalPosition;
  this->setPos(p, tmpMx.translation());

  tmpMx = rotRelMx.inverse() * globalPosition;
  this->setRot(r, tmpMx.direction());

  return *this;
}


/*!
  Sets this to be positioned and rotated as the provided matrix \a mx,
  converting it to the provided storage types as needed.
*/

FFa3DLocation& FFa3DLocation::set(PosType p, RotType r, const FaMat34& mx)
{
  this->setPos(p, mx.translation());
  this->setRot(r, mx.direction());
  return *this;
}


/*!
  Sets translaton of this to the provided cartesian position \a cartPos,
  converting it to PosType \a p way of storing it.
*/

FFa3DLocation& FFa3DLocation::setPos(PosType p, const FaVec3& cartPos)
{
#if FFA_DEBUG > 1
  static int count = 0;
  std::cout <<"\nFFa3DLocation::setPos("<< ++count <<") PosType="<< p
            <<"\ncartPos = "<< cartPos << std::endl;
#endif

  switch (p)
    {
    case CART_X_Y_Z:
      myL[0] = cartPos;
      break;
    case CYL_R_YR_X:
      myL[0] = cartPos.getAsCylCoords(VX);
      myL[0][1] *= 180.0/M_PI;
      break;
    case CYL_R_ZR_Y:
      myL[0] = cartPos.getAsCylCoords(VY);
      myL[0][1] *= 180.0/M_PI;
      break;
    case CYL_R_XR_Z:
      myL[0] = cartPos.getAsCylCoords(VZ);
      myL[0][1] *= 180.0/M_PI;
      break;
    default:
      return *this;
    }

#if FFA_DEBUG > 1
  std::cout <<"myLoc[0] = "<< myL[0] << std::endl;
#endif
  myPosTyp = p;
  return *this;
}


/*!
  Sets rotation of this to the provided matrix rotation \a rotMat,
  converting it to RotType \a r way of storing it.
  The translation of the matrix is ignored. When needed,
  the current position is fetched by calling this->translation().
*/

FFa3DLocation& FFa3DLocation::setRot(RotType r, const FaMat33& rotMat)
{
#if FFA_DEBUG > 1
  static int count = 0;
  std::cout <<"\nFFa3DLocation::setRot("<< ++count <<") RotType="<< r
            <<"\nrotMat ="<< rotMat << std::endl;
#endif
  static FaVec3 cartPos;

  switch (r)
    {
    case EUL_Z_Y_X:
      myL[1] = rotMat.getEulerZYX() * 180.0/M_PI; // Euler angles in degrees
      break;
    case PNT_PX_PXY:
      cartPos = this->translation();
      myL[1] = cartPos + rotMat[VX];
      myL[2] = cartPos + rotMat[VY];
      break;
    case PNT_PZ_PXZ:
      cartPos = this->translation();
      myL[1] = cartPos + rotMat[VZ];
      myL[2] = cartPos + rotMat[VX];
      break;
    case DIR_EX_EXY:
      myL[1] = rotMat[VX];
      myL[2] = rotMat[VY];
      break;
    default:
      return *this;
    }

#if FFA_DEBUG > 1
  std::cout <<"myLoc[1] = "<< myL[1] <<"\nmyLoc[2] = "<< myL[2] << std::endl;
#endif
  myRotTyp = r;
  return *this;
}


/*!
  Returns the cartesian position of this 3DLocation.
*/

FaVec3 FFa3DLocation::translation() const
{
  static FaVec3 cartPos, cylPos;

  switch (myPosTyp)
    {
    case CART_X_Y_Z:
      cartPos = myL[0];
      break;
    case CYL_R_YR_X:
      cylPos = myL[0];
      cylPos[1] *= M_PI/180.0;
      cartPos.setByCylCoords(cylPos,VX);
      break;
    case CYL_R_ZR_Y:
      cylPos = myL[0];
      cylPos[1] *= M_PI/180.0;
      cartPos.setByCylCoords(cylPos,VY);
      break;
    case CYL_R_XR_Z:
      cylPos = myL[0];
      cylPos[1] *= M_PI/180.0;
      cartPos.setByCylCoords(cylPos,VZ);
      break;
    default:
      break;
    }

#if FFA_DEBUG > 1
  static int count = 0;
  std::cout <<"FFa3DLocation::translation("<< ++count <<") "<< cartPos
            << std::endl;
#endif
  return cartPos;
}


/*!
  Returns the rotation of this 3DLocation as a rotation matrix.
*/

FaMat33 FFa3DLocation::direction() const
{
  static FaMat33 result;
  static FaMat34 mx;

  switch (myRotTyp)
    {
    case EUL_Z_Y_X:
      result.eulerRotateZYX(myL[1]*M_PI/180.0);
      break;
    case PNT_PX_PXY:
      mx.makeCS_X_YX(this->translation(), myL[1], myL[2]);
      result = mx.direction();
      break;
    case PNT_PZ_PXZ:
      mx.makeCS_Z_XZ(this->translation(), myL[1], myL[2]);
      result = mx.direction();
      break;
    case DIR_EX_EXY:
      mx.makeCS_X_YX(FaVec3(), myL[1], myL[2]);
      result = mx.direction();
      break;
    }

#if FFA_DEBUG > 1
  static int count = 0;
  std::cout <<"FFa3DLocation::direction("<< ++count <<")"<< result << std::endl;
#endif
  return result;
}


/*!
  Returns the number of active fields in this 3DLocation.
  Depends on the rotation type set.
*/

int FFa3DLocation::getNumFields() const
{
  switch (myRotTyp)
    {
    case EUL_Z_Y_X:  return 6;
    case PNT_PX_PXY: return 9;
    case PNT_PZ_PXZ: return 9;
    case DIR_EX_EXY: return 9;
    }

  return 6;
}


/*!
  Get a FaMat34 representation of the 3DLocation.
*/

FaMat34 FFa3DLocation::getMatrix() const
{
  return FaMat34(this->direction(),this->translation());
}


/*!
  Utility method that returns a FaMat34 that represents the global position of
  this 3DLocation data if this is placed relative to the two matrices provided.

  return FaMat34([rotRelMx * 3DLoc.rotation], [posRelMx * 3DLoc.pos])
*/

FaMat34 FFa3DLocation::getMatrix (const FaMat34& posRelMx,
				  const FaMat34& rotRelMx) const
{
  return FaMat34(rotRelMx.direction() * this->direction(),
		 posRelMx * this->translation());
}


/*!
  Check that the current data represent a valid location.
*/

bool FFa3DLocation::isValid() const
{
  const double tol = 1.0e-8;
  static FaVec3 origin;
  int error = 0;

  switch (myRotTyp)
    {
    case PNT_PX_PXY:
    case PNT_PZ_PXZ:
      origin = this->translation();
      if (myL[1].equals(origin,tol))
        error = 1;
      else if (myL[2].equals(origin,tol))
        error = 2;
      break;

    case DIR_EX_EXY:
      if (myL[1].isZero(tol))
        error = 1;
      else if (myL[2].isZero(tol))
        error = 2;
      break;

    default:
      break;
    }

  if (error == 0) return true;

  char cvec[64];
  origin = myL[error];
  snprintf(cvec,64,"[%g,%g,%g] ",origin.x(),origin.y(),origin.z());
  FFaMsg::dialog("The given vector " + std::string(cvec) +
                 "can not be used to define the orientation of this object");
  return false;
}


std::ostream& operator << (std::ostream& s, const FFa3DLocation& m)
{
  if (m.saveNumericalData)
  {
    // Output everything, using 8 significant digits
    std::ios_base::fmtflags tmpFlag = s.flags(std::ios::fixed);
    int tmpPrec = s.precision(8);
    s <<"\n"
      << m.myPosTyp << "\t" << m[0] << "\n"
      << m.myRotTyp << "\t" << m[1];
    if (m.getNumFields() == 9)
      s << "\n\t\t" << m[2];

    s.flags(tmpFlag);
    s.precision(tmpPrec);
  }
  else // Output only the type enums
    s << m.myPosTyp <<"  "<< m.myRotTyp;

  return s;
}


std::istream& operator >> (std::istream& s, FFa3DLocation& m)
{
  // Retain the save flag
  FFa3DLocation m_tmp(m.saveNumericalData);
  s >> m_tmp.myPosTyp;

  // Check if the numerical data was stored
  char testChar = 0;
  s.get(testChar);
  s.putback(testChar);
  if (isdigit(testChar) || testChar == '-')
  {
    s >> m_tmp[0] >> m_tmp.myRotTyp >> m_tmp[1];
    if (m_tmp.getNumFields() == 9)
      s >> m_tmp[2];
    else
      m_tmp[2].clear(); // Ignore the last 3 numerical fields, if present
  }
  else // No numerical data stored, just read the position type enum
    s >> m_tmp.myRotTyp;

  if (s) m = m_tmp;
  return s;
}
