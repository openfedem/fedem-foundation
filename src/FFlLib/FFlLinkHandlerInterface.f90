!! SPDX-FileCopyrightText: 2023 SAP SE
!!
!! SPDX-License-Identifier: Apache-2.0
!!
!! This file is part of FEDEM - https://openfedem.org
!!==============================================================================

module FFlLinkHandlerInterface

  implicit none

  interface ffl_init

     subroutine ffl_reducer_init (maxNodes, maxElms, ierr)
       integer, intent(in)  :: maxNodes, maxElms
       integer, intent(out) :: ierr
     end subroutine ffl_reducer_init

     subroutine ffl_limited_init (maxNodes, maxElms, setcalcflag, ierr)
       integer, intent(in)  :: maxNodes, maxElms
       logical, intent(in)  :: setcalcflag
       integer, intent(out) :: ierr
     end subroutine ffl_limited_init

     subroutine ffl_full_init (linkName, elmGroups, ierr)
       character(len=*), intent(in)  :: linkName, elmGroups
       integer         , intent(out) :: ierr
     end subroutine ffl_full_init

     module procedure ffl_free_init

  end interface

  private :: ffl_reducer_init, ffl_limited_init, ffl_full_init, ffl_free_init


  interface

     subroutine ffl_set (linkIdx)
       integer, intent(in) :: linkIdx
     end subroutine ffl_set

     subroutine ffl_release (removeSingletons)
       logical, intent(in) :: removeSingletons
     end subroutine ffl_release

     subroutine ffl_export_vtf (vtffile, linkName, linkID, ierr)
       character(len=*), intent(in)  :: vtffile, linkName
       integer         , intent(in)  :: linkID
       integer         , intent(out) :: ierr
     end subroutine ffl_export_vtf

     subroutine ffl_elmorder_vtf (elmOrder, stat)
       integer, intent(out) :: elmOrder, stat
     end subroutine ffl_elmorder_vtf

     subroutine ffl_massprop (mass, cg, inertia)
       integer , parameter   :: dp = kind(1.0D0)
       real(dp), intent(out) :: mass, cg(*), inertia(*)
     end subroutine ffl_massprop

     subroutine ffl_getsize (nnod, nel, ndof, nmnpc, nmat, nxnod, npbeam, &
          &                  nrgd, nrbar, nwavgm, nprop, ncons, ierr)
       integer , intent(out) :: nnod, nel, ndof, nmnpc, nmat, nxnod, npbeam
       integer , intent(out) :: nrgd, nrbar, nwavgm, nprop, ncons, ierr
     end subroutine ffl_getsize

     subroutine ffl_getnodes (nnod, ndof, madof, minex, mnode, msc, &
          &                   x, y, z, ierr)
       integer , parameter   :: dp = kind(1.0D0)
       integer , intent(out) :: nnod, ndof, ierr
       integer , intent(out) :: madof, minex, mnode, msc
       real(dp), intent(out) :: x, y, z
     end subroutine ffl_getnodes

     subroutine ffl_gettopol (nel, nmnpc, mekn, mmnpc, mpmnpc, &
          &                   mpbeam, mprgd, mprbar, mpwavgm, ierr)
       integer , intent(out) :: nel, nmnpc, ierr
       integer , intent(out) :: mekn, mmnpc, mpmnpc
       integer , intent(out) :: mpbeam(*), mprgd(*), mprbar(*), mpwavgm(*)
     end subroutine ffl_gettopol

     function ffl_getelmid (iel)
       integer , intent(in)  :: iel
       integer               :: ffl_getelmid
     end function ffl_getelmid

     function ffl_ext2int (nodeID, Id)
       logical , intent(in)  :: nodeID
       integer , intent(in)  :: id
       integer               :: ffl_ext2int
     end function ffl_ext2int

     subroutine ffl_getcoor (x, y, z, iel, ierr)
       integer , parameter   :: dp = kind(1.0D0)
       real(dp), intent(out) :: x(*), y(*), z(*)
       integer , intent(in)  :: iel
       integer , intent(out) :: ierr
     end subroutine ffl_getcoor

     subroutine ffl_getmat (E, nu, rho, iel, ierr)
       integer , parameter   :: dp = kind(1.0D0)
       real(dp), intent(out) :: E, nu, rho
       integer , intent(in)  :: iel
       integer , intent(out) :: ierr
     end subroutine ffl_getmat

     subroutine ffl_getthick (Th, iel, ierr)
       integer , parameter   :: dp = kind(1.0D0)
       real(dp), intent(out) :: Th(*)
       integer , intent(in)  :: iel
       integer , intent(out) :: ierr
     end subroutine ffl_getthick

     subroutine ffl_getpinflags (pA, pB, iel, ierr)
       integer , intent(in)  :: iel
       integer , intent(out) :: pA, pB, ierr
     end subroutine ffl_getpinflags

     subroutine ffl_getnsm (Mass, iel, ierr)
       integer , parameter   :: dp = kind(1.0D0)
       real(dp), intent(out) :: Mass
       integer , intent(in)  :: iel
       integer , intent(out) :: ierr
     end subroutine ffl_getnsm

     subroutine ffl_getbeamsection (section, iel, ierr)
       integer , parameter   :: dp = kind(1.0D0)
       real(dp), intent(out) :: section(*)
       integer , intent(in)  :: iel
       integer , intent(out) :: ierr
     end subroutine ffl_getbeamsection

     subroutine ffl_getnodalcoor (x, y, z, inod, ierr)
       integer , parameter   :: dp = kind(1.0D0)
       real(dp), intent(out) :: x, y, z
       integer , intent(in)  :: inod
       integer , intent(out) :: ierr
     end subroutine ffl_getnodalcoor

     subroutine ffl_getmass (em, iel, ndof, ierr)
       integer , parameter   :: dp = kind(1.0D0)
       real(dp), intent(out) :: em(*)
       integer , intent(in)  :: iel
       integer , intent(out) :: ndof, ierr
     end subroutine ffl_getmass

     subroutine ffl_getspring (ek, nedof, iel, ierr)
       integer , parameter   :: dp = kind(1.0D0)
       real(dp), intent(out) :: ek(*)
       integer , intent(out) :: nedof
       integer , intent(in)  :: iel
       integer , intent(out) :: ierr
     end subroutine ffl_getspring

     subroutine ffl_getelcoorsys (T, iel, ierr)
       integer , parameter   :: dp = kind(1.0D0)
       real(dp), intent(out) :: T(3,3)
       integer , intent(in)  :: iel
       integer , intent(out) :: ierr
     end subroutine ffl_getelcoorsys

     subroutine ffl_getbush (k, iel, ierr)
       integer , parameter   :: dp = kind(1.0D0)
       real(dp), intent(out) :: k(6)
       integer , intent(in)  :: iel
       integer , intent(out) :: ierr
     end subroutine ffl_getbush

     subroutine ffl_getrgddofcomp (rgdComp, iel, ierr)
       integer , intent(in)  :: iel
       integer , intent(out) :: rgdComp(*), ierr
     end subroutine ffl_getrgddofcomp

     subroutine ffl_getwavgm (refC, indC, weights, iel, ierr)
       integer , parameter   :: dp = kind(1.0D0)
       integer , intent(in)  :: iel
       integer , intent(out) :: refC, indC(6), ierr
       real(dp), intent(out) :: weights(*)
     end subroutine ffl_getwavgm

     subroutine ffl_getloadcases (loadCases, nlc)
       integer , intent(out)   :: loadCases
       integer , intent(inout) :: nlc
     end subroutine ffl_getloadcases

     function ffl_getnoload (ilc)
       integer , intent(in) :: ilc
       integer              :: ffl_getnoload
     end function ffl_getnoload

     subroutine ffl_getload (ilc, eid, face, P)
       integer , parameter     :: dp = kind(1.0D0)
       integer , intent(in)    :: ilc
       integer , intent(out)   :: eid
       integer , intent(inout) :: face
       real(dp), intent(out)   :: P
     end subroutine ffl_getload

     subroutine ffl_getstraincoat (id, nnod, npoint, nodes, mnum, &
          &                        E, nu, Z, set, SCF, SNcurve, eid, ierr)
       integer , parameter   :: dp = kind(1.0D0)
       integer , intent(out) :: id, nnod, npoint, nodes(*), mnum(*)
       real(dp), intent(out) :: E(*), nu(*), Z(*), SCF(*)
       integer , intent(out) :: set(*), SNcurve(*), eid, ierr
     end subroutine ffl_getstraincoat

     function ffl_getnostrcmat ()
       integer :: ffl_getnostrcmat
     end function ffl_getnostrcmat

     function ffl_getnostrc ()
       integer :: ffl_getnostrc
     end function ffl_getnostrc

     subroutine ffl_calcs (ierr)
       integer , intent(out) :: ierr
     end subroutine ffl_calcs

     subroutine ffl_addcs_int (value)
       integer , intent(in)  :: value
     end subroutine ffl_addcs_int

     subroutine ffl_addcs_double (value)
       integer , parameter   :: dp = kind(1.0D0)
       real(dp), intent(in)  :: value
     end subroutine ffl_addcs_double

     subroutine ffl_getcs (cs, ierr)
       integer , intent(out) :: cs, ierr
     end subroutine ffl_getcs

     subroutine ffl_attribute (name, iel, ierr)
       character(len=*), intent(in) :: name
       integer , intent(in)         :: iel
       integer , intent(inout)      :: ierr
     end subroutine ffl_attribute

     subroutine ffl_getPMATSHELL (MID, E1, E2, NU12, G12, G1Z, G2Z, RHO, ierr)
       integer , parameter   :: dp = kind(1.0D0)
       integer , intent(in)  :: MID
       real(dp), intent(out) :: E1, E2, NU12, G12, G1Z, G2Z, RHO
       integer , intent(out) :: ierr
     end subroutine ffl_getPMATSHELL

     subroutine ffl_getPCOMP (compID, nPlys, Z0, T, theta, &
          &                   E1, E2, NU12, G12, G1Z, G2Z, RHO, ierr)
       integer , parameter     :: dp = kind(1.0D0)
       integer , intent(inout) :: compID
       integer , intent(out)   :: nPlys
       real(dp), intent(out)   :: Z0, T(*), theta(*), E1(*), E2(*), NU12(*)
       real(dp), intent(out)   :: G12(*), G1Z(*), G2Z(*), RHO(*)
       integer , intent(out)   :: ierr
     end subroutine ffl_getPCOMP

     function ffl_getMaxCompositePlys ()
       integer :: ffl_getMaxCompositePlys
     end function ffl_getMaxCompositePlys

     function ffl_getPCOMPnumPlys (compID)
       integer, intent(in) :: compID
       integer             :: ffl_getPCOMPnumPlys
     end function ffl_getPCOMPnumPlys

  end interface

  private :: ffl_attribute, ffl_release


contains

  subroutine ffl_free_init (setcalcflag, ierr)
    logical, intent(in)  :: setcalcflag
    integer, intent(out) :: ierr
    call ffl_limited_init (0,0,setcalcflag,ierr)
  end subroutine ffl_free_init

  !!============================================================================
  !> @brief Initializes an FE part by providing the file content as a string.
  !> @details Used for unit testing only.
  subroutine ffl_test_init (ftldata, ierr)
    character(len=*), intent(in)  :: ftldata
    integer         , intent(out) :: ierr
    integer         , parameter   :: lpu = 99
    character(len=8), parameter   :: ftlfile = 'temp.ftl'
    open(lpu,FILE=ftlfile,STATUS='UNKNOWN')
    write(lpu,'(A)') ftldata
    close(lpu)
    call ffl_full_init (ftlfile,'',ierr)
  end subroutine ffl_test_init

  function ffl_hasAttribute (name, iel, status)
    character(len=*), intent(in) :: name
    integer , intent(in)         :: iel
    integer , intent(out)        :: status
    logical                      :: ffl_hasAttribute
    status = 0 ! Return whether the attribute exists or not
    call ffl_attribute (name,iel,status)
    ffl_hasAttribute = status /= 0
  end function ffl_hasAttribute

  function ffl_getAttributeID (name, iel) result(status)
    character(len=*), intent(in) :: name
    integer         , intent(in) :: iel
    integer                      :: status
    status = 1 ! Return the attribute ID
    call ffl_attribute (name,iel,status)
  end function ffl_getAttributeID

  subroutine ffl_done (removeSingletons)
    logical, optional, intent(in) :: removeSingletons
    if (present(removeSingletons)) then
       call ffl_release (removeSingletons)
    else
       call ffl_release (.false.)
    end if
  end subroutine ffl_done

end module FFlLinkHandlerInterface
