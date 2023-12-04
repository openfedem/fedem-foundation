!! SPDX-FileCopyrightText: 2023 SAP SE
!!
!! SPDX-License-Identifier: Apache-2.0
!!
!! This file is part of FEDEM - https://openfedem.org
!!==============================================================================

program memchk

  use FFlLinkHandlerInterface, only : ffl_init, ffl_done

  character(len=256) :: fileName
  character(len=8)   :: answer
  integer            :: ntrial, i, ierr

  if (iargc() < 2) then
     call getarg (0,fileName)
     print*,'usage: ',trim(fileName),' <ntrial> <ftl-file>'
     stop
  end if

  call getarg (1,fileName)
  read(fileName,*) ntrial
  print*,'Ntrial=',ntrial
  call getarg (2,fileName)
  print*,'fName =',trim(fileName)
  write(6,6,advance='NO') 'Start ? '
  read(5,*) answer

  ierr = 0
  do i = 1, ntrial
     if (answer == 'n') exit

     call ffl_init (trim(fileName),'',ierr)
     write(6,*) 'Initialized',i,':',ierr
     write(6,6,advance='NO') 'Continue ? '
     read(5,*) answer
     if (answer == 'n' .or. ierr < 0) exit

     call ffl_done ()
     write(6,*) 'Done',i
     write(6,6,advance='NO') 'Continue ? '
     read(5,*) answer

  end do

6 format(1X,A)

end program memchk
