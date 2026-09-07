!! SPDX-FileCopyrightText: 2023 SAP SE
!!
!! SPDX-License-Identifier: Apache-2.0
!!
!! This file is part of FEDEM - https://openfedem.org
!!==============================================================================

!> @file FFpBatchExportInterface.f90
!> @brief Fortran interface for the batch curve exporter.
!>
!> @details This file contains a module with Fortran interface definitions
!> for the curve export feature, to be invoked after the simulation is finished.
!> See the file FFpBatchExport_F.C for the actual implementation in C++.

!!==============================================================================
!> @brief Fortran interface for FFpBatchExport methods.

module FFpBatchExportInterface

  implicit none

  interface

     !> @brief Launches automatic curve export after the solver has finished.
     !> @param[in] frsnames List of frs-file names to read solver results from
     !> @param[in] expPath Name of exported curve data file
     !> @param[in] modName Model name to use as meta-data in the @a expPath file
     !> @param[out] ierr Error flag
     subroutine ffp_crvexp (frsnames, expPath, modName, ierr)
       character*(*), intent(in)  :: frsnames, expPath, modName
       integer      , intent(out) :: ierr
     end subroutine ffp_crvexp

  end interface

end module FFpBatchExportInterface
