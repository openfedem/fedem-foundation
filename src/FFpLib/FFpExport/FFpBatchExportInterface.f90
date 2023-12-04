!! SPDX-FileCopyrightText: 2023 SAP SE
!!
!! SPDX-License-Identifier: Apache-2.0
!!
!! This file is part of FEDEM - https://openfedem.org
!!==============================================================================

module FFpBatchExportInterface

  implicit none

  interface

     subroutine ffp_crvexp (frsnames, expPath, modName, ierr)
       character*(*), intent(in)  :: frsnames, expPath, modName
       integer      , intent(out) :: ierr
     end subroutine ffp_crvexp

  end interface

end module FFpBatchExportInterface
