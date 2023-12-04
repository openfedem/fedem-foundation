!! SPDX-FileCopyrightText: 2023 SAP SE
!!
!! SPDX-License-Identifier: Apache-2.0
!!
!! This file is part of FEDEM - https://openfedem.org
!!==============================================================================

!> @file fiDeviceFunctionInterface.f90
!> @brief Fortran interface for FiDeviceFunctionFactory methods.
!> @details This file contains a module with Fortran interface definitions
!> for the methods of the FiDeviceFunctionFactory class.
!> See the file FiDeviceFunctionFactory_F.C for the implementation
!> of the wrapper functions of this interface.

!!==============================================================================
!> @brief Fortran interface for FiDeviceFunctionFactory methods.

module fiDeviceFunctionInterface

  implicit none

  integer, parameter :: DACFile_p = 1 !< DAC file type enum value

  interface

     !> @brief Opens a device function file for read access.
     !> @param[in] fileName Name of device function file
     !> @param fileId On input; integration order. On output; file handle.
     !> @param[out] err Error flag
     subroutine fidf_open (fileName,fileId,err)
       character(len=*), intent(in)    :: fileName
       integer         , intent(inout) :: fileId
       integer         , intent(out)   :: err
     end subroutine fidf_open

     !> @brief Opens a device function file for write access.
     !> @param[in] fileName Name of device function file
     !> @param[in] fileType Type of device function file
     !> @param[out] fileId File handle
     !> @param[out] err Error flag
     subroutine fidf_openwrite (fileName,fileType,fileId,err)
       character(len=*), intent(in)  :: fileName
       integer         , intent(in)  :: fileType
       integer         , intent(out) :: fileId, err
     end subroutine fidf_openwrite

     !> @brief Closes a specified device function file.
     !> @param[in] fileId File handle
     subroutine fidf_close (fileId)
       integer, intent(in) :: fileId
     end subroutine fidf_close

     !> @brief Closes all currently opened device function files.
     subroutine fidf_closeall ()
     end subroutine fidf_closeall

     !> @brief Evaluates a device function at a given point.
     !> @param[in] fileId File handle
     !> @param[in] arg Function argument
     !> @param err Error flag. On input it specifies the integration order
     !> @param[in] channel Channel index for external functions or multi-columns
     !> @param[in] zeroAdj Flag for adjusting first function value to zero
     !> @param[in] vertShift Additional vertical shift of function values
     !> @param[in] scaleFac Scaling factor to apply on function values
     function fidf_getvalue (fileId,arg,err,channel,zeroAdj,vertShift,scaleFac)
       integer , parameter     :: dp = kind(1.0D0)
       real(dp)                :: fidf_getvalue
       integer , intent(in)    :: fileId, zeroAdj, channel
       real(dp), intent(in)    :: arg, vertShift, scaleFac
       integer , intent(inout) :: err
     end function fidf_getvalue

     !> @brief Assigns a value to a device function at a given point.
     !> @param[in] fileId File handle
     !> @param[in] arg Function argument
     !> @param[in] val Function value to assign
     subroutine fidf_setvalue (fileId,arg,val)
       integer , parameter  :: dp = kind(1.0D0)
       integer , intent(in) :: fileId
       real(dp), intent(in) :: arg, val
     end subroutine fidf_setvalue

     !> @brief Sets the sampling frequency for a device function.
     !> @param[in] fileId File handle
     !> @param[in] freq Sampling frequency
     subroutine fidf_setfrequency (fileId,freq)
       integer , parameter  :: dp = kind(1.0D0)
       integer , intent(in) :: fileId
       real(dp), intent(in) :: freq
     end subroutine fidf_setfrequency

     !> @brief Sets the sampling step size for a device function.
     !> @param[in] fileId File handle
     !> @param[in] step Sampling step size
     subroutine fidf_setstep (fileId,step)
       integer , parameter  :: dp = kind(1.0D0)
       integer , intent(in) :: fileId
       real(dp), intent(in) :: step
     end subroutine fidf_setstep

     !> @brief Returns the X-axis label for a device function.
     !> @param[in] fileId File handle
     !> @param[out] title X-axis description
     !> @param[out] unit X-axis unit
     subroutine fidf_getxaxis (fileId,title,unit)
       integer         , intent(in)  :: fileId
       character(len=*), intent(out) :: title, unit
     end subroutine fidf_getxaxis

     !> @brief Returns the Y-axis label for a device function.
     !> @param[in] fileId File handle
     !> @param[out] title Y-axis description
     !> @param[out] unit Y-axis unit
     subroutine fidf_getyaxis (fileId,title,unit)
       integer         , intent(in)  :: fileId
       character(len=*), intent(out) :: title, unit
     end subroutine fidf_getyaxis

     !> @brief Sets the X-axis label for a device function.
     !> @param[in] fileId File handle
     !> @param[in] title X-axis description
     !> @param[in] unit X-axis unit
     subroutine fidf_setxaxis (fileId,title,unit)
       integer         , intent(in) :: fileId
       character(len=*), intent(in) :: title, unit
     end subroutine fidf_setxaxis

     !> @brief Sets the Y-axis label for a device function.
     !> @param[in] fileId File handle
     !> @param[in] title Y-axis description
     !> @param[in] unit Y-axis unit
     subroutine fidf_setyaxis (fileId,title,unit)
       integer         , intent(in) :: fileId
       character(len=*), intent(in) :: title, unit
     end subroutine fidf_setyaxis

     !> @brief Dumps data about all defined device functions to console.
     subroutine fidf_dump ()
     end subroutine fidf_dump

     !> @brief Opens a file for reading external function values.
     !> @param[out] Error flag
     !> @param[in] fileName Name of file
     !> @param[in] labels Labels of file columns to use
     subroutine fidf_extfunc (err,fileName,labels)
       integer         , intent(out) :: err
       character(len=*), intent(in)  :: fileName, labels
     end subroutine fidf_extfunc

     !> @brief Updates the external function values from file.
     !> @param[in] nstep Number of steps to read
     subroutine fidf_extfunc_ff (nstep)
       integer, intent(in) :: nstep
     end subroutine fidf_extfunc_ff

     !> @brief Stores/extracts external function values from in-core array.
     !> @param data Solution state array
     !> @param[in] ndat Length of the solution state array
     !> @param[in] iop Option telling what to do (0=return required array size)
     !> @param istat Running array offset, negative value indicates an error
     subroutine fidf_storeextfunc (data, ndat, iop, istat)
       integer , parameter     :: dp = kind(1.0D0)
       integer , intent(in)    :: ndat, iop
       real(dp), intent(inout) :: data(ndat)
       integer , intent(inout) :: istat
     end subroutine fidf_storeextfunc

     !> @brief Initializes external function values from file or an array.
     !> @param data Array of external function values
     !> @param[in] ndat Length of the @a data array
     !> @param[out] istat Equals @a ndat or a negative value on error
     subroutine fidf_initextfunc (data, ndat, istat)
       integer , parameter   :: dp = kind(1.0D0)
       integer , intent(in)  :: ndat
       real(dp), intent(in)  :: data(ndat)
       integer , intent(out) :: istat
     end subroutine fidf_initextfunc

  end interface

contains

  !> @brief Returns the size of the external function values array.
  function fidf_extfuncsize () result(ndat)
    integer, parameter :: dp = kind(1.0D0)
    integer            :: ndat
    real(dp)           :: dummy(1)
    call fidf_storeExtFunc (dummy,1,0,ndat)
  end function fidf_extfuncsize

end module fiDeviceFunctionInterface
