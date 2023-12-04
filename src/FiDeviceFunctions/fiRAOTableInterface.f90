!! SPDX-FileCopyrightText: 2023 SAP SE
!!
!! SPDX-License-Identifier: Apache-2.0
!!
!! This file is part of FEDEM - https://openfedem.org
!!==============================================================================

module fiRAOTableInterface

  implicit none

  interface

     subroutine ficonvertwavedata (fileName,dir,nRows,nComp,waveData,err)
       integer     , parameter     :: dp = kind(1.0D0)
       character(*), intent(in)    :: fileName
       integer     , intent(in)    :: dir, nComp
       real(dp)    , intent(in)    :: waveData
       integer     , intent(inout) :: err
     end subroutine ficonvertwavedata

     subroutine fiextractmotion (dof,motionData,err)
       integer , parameter     :: dp = kind(1.0D0)
       integer , intent(in)    :: dof
       real(dp), intent(out)   :: motionData
       integer , intent(inout) :: err
     end subroutine fiextractmotion

     subroutine fireleasemotion ()
     end subroutine fireleasemotion

  end interface

end module fiRAOTableInterface