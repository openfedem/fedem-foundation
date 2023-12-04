!! SPDX-FileCopyrightText: 2023 SAP SE
!!
!! SPDX-License-Identifier: Apache-2.0
!!
!! This file is part of FEDEM - https://openfedem.org
!!==============================================================================

!! Dummy subroutines

subroutine ffamsg_list (text,ival)
  character(len=*), intent(in) :: text
  integer,optional, intent(in) :: ival
  if (present(ival)) then
     print 1,text,ival
  else
     print 1,text
  end if
1 format(a,i4)
end subroutine ffamsg_list

function fidf_getvalue (id,arg,err,channel,zeroAdjust,vertShift,scaleFac)
  integer :: id, err, channel, zeroAdjust
  double precision :: fidf_getvalue, arg, vertShift, scaleFac
  fidf_getvalue = 0.0d0; err = -1
  print*,' ** fidf_getvalue dummy:',id,arg,channel,zeroAdjust,vertShift,scaleFac
end function fidf_getvalue

function ffame_getvalue (id,arg,err) result(val)
  integer :: id, err
  double precision :: arg, val
  val = 0.0d0; err = -1
  print*,' ** ffame_getvalue dummy:',id,arg
end function ffame_getvalue

function ffame_getvalue2 (id,arg,err) result(val)
  integer :: id, err
  double precision :: arg(*), val
  val = 0.0d0; err = -1
  print*,' ** ffame_getvalue2 dummy:',id,ia,arg(1)
end function ffame_getvalue2

function ffame_getdiff (id,arg,err) result(val)
  integer :: id, err
  double precision :: arg, val
  val = 0.0d0; err = -1
  print*,' ** ffame_getdiff dummy:',id,arg
end function ffame_getdiff

function ffame_getdiff2 (id,ia,arg,err) result(val)
  integer :: id, ia, err
  double precision :: arg(*), val
  val = 0.0d0; err = -1
  print*,' ** ffame_getdiff2 dummy:',id,ia,arg(1)
end function ffame_getdiff2
