!! SPDX-FileCopyrightText: 2023 SAP SE
!!
!! SPDX-License-Identifier: Apache-2.0
!!
!! This file is part of FEDEM - https://openfedem.org
!!==============================================================================

!> @file fiRAOTableInterface.f90
!> @brief Fortran interface for FiRAOTable methods.
!> @details This file contains a module with Fortran interface definitions
!> for the methods of the FiRAOTable class.
!> See the file FiRAOTable_F.C for the implementation
!> of the wrapper functions of this interface.

!!==============================================================================
!> @brief Fortran interface for FiRAOTable methods.

module fiRAOTableInterface

  implicit none

  interface

     !!=========================================================================
     !> @brief Applies a Response Amplitude Operator (RAO) to a wave function.
     !> @param[in] fileName Name of file contaning the ROA data
     !> @param[in] dir Wave direction (angle in degrees)
     !> @param[in] nRows Number of data values per wave component (3 or more)
     !> @param[in] nComp Number of wave component
     !> @param[in] waveData Wave component data (amplitude, frequency, phase)
     !> @param err Error indicator
     !>
     !> @details The resulting vessel motion data is stored in an internal
     !> container which is released by firaotableinterface::fireleasemotion.
     subroutine ficonvertwavedata (fileName,dir,nRows,nComp,waveData,err)
       integer     , parameter     :: dp = kind(1.0D0)
       character(*), intent(in)    :: fileName
       integer     , intent(in)    :: dir, nComp
       real(dp)    , intent(in)    :: waveData
       integer     , intent(inout) :: err
     end subroutine ficonvertwavedata

     !!=========================================================================
     !> @brief Extracts vessel motion data for a given DOF.
     !> @param[in] dof The DOF to extract motion data for
     !> @param[in] motionData Vessel motion data for the specified DOF.
     !> @param err Error indicator
     subroutine fiextractmotion (dof,motionData,err)
       integer , parameter     :: dp = kind(1.0D0)
       integer , intent(in)    :: dof
       real(dp), intent(out)   :: motionData
       integer , intent(inout) :: err
     end subroutine fiextractmotion

     !!=========================================================================
     !> @brief Releases the internal vessel motion container.
     subroutine fireleasemotion ()
     end subroutine fireleasemotion

  end interface

end module fiRAOTableInterface
