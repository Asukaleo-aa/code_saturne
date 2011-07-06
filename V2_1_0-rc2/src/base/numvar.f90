!-------------------------------------------------------------------------------

!     This file is part of the Code_Saturne Kernel, element of the
!     Code_Saturne CFD tool.

!     Copyright (C) 1998-2010 EDF S.A., France

!     contact: saturne-support@edf.fr

!     The Code_Saturne Kernel is free software; you can redistribute it
!     and/or modify it under the terms of the GNU General Public License
!     as published by the Free Software Foundation; either version 2 of
!     the License, or (at your option) any later version.

!     The Code_Saturne Kernel is distributed in the hope that it will be
!     useful, but WITHOUT ANY WARRANTY; without even the implied warranty
!     of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
!     GNU General Public License for more details.

!     You should have received a copy of the GNU General Public License
!     along with the Code_Saturne Kernel; if not, write to the
!     Free Software Foundation, Inc.,
!     51 Franklin St, Fifth Floor,
!     Boston, MA  02110-1301  USA

!-------------------------------------------------------------------------------

! Module for variable numbering

module numvar

  !=============================================================================

  use paramx

  !=============================================================================

  ! Position des variables
  !  ( dans rtp, rtpa )

  ! ipr                        pression
  ! iu   iv   iw               vitesse(x,y,z)
  ! ik                         energie turbulente en k-epsilon
  ! ir11, ir22, ir33, ...
  ! ... ir12, ir13, ir23       tensions de Reynolds en Rij
  ! iep                        Dissipation turbulente
  ! iphi, ifb, ial             variables phi et f_barre du v2f phi-model
  ! iomg                       variable omega du k-omega SST
  ! inusa                      variable nu du Spalart-Allmaras
  ! isca(i)                    scalaire numero i
  ! iscapp(i)                  no du scalaire physique particuliere i
  ! nscaus                     nbre de scalaires utilisateur
  ! nscapp                     nbre de scalaires physique particuliere
  ! iuma, ivma, iwma           Vitesse de maillage en ALE

  integer, save :: ipr ,                                        &
                   iu  , iv    , iw  ,                          &
                   ik  , iep   ,                                &
                   ir11, ir22  , ir33,                          &
                   ir12, ir13  , ir23,                          &
                   iphi, ifb   , ial , iomg,                    &
                   inusa,                                       &
                   isca(nscamx), iscapp(nscamx),                &
                   nscaus      , nscapp        ,                &
                   iuma        , ivma          , iwma


  ! Position des proprietes (physiques ou numeriques)
  !  (dans propce, propfa et propfb)
  !    le numero des proprietes est unique, quelle aue soit la
  !      localisation de ces dernieres (cellule, face, face de bord)
  !    Voir usclim pour quelques exemples

  ! ipproc : pointeurs dans propce
  ! ipprof : pointeurs dans propfa
  ! ipprob : pointeurs dans propfb

  ! irom   : Masse volumique des phases
  ! iroma  : Masse volumique des phases au pas de temps precedent
  ! iviscl : Viscosite moleculaire dynamique en kg/(ms) des phases
  ! ivisct : Viscosite turbulente des phases
  ! ivisla : Viscosite moleculaire dynamique en kg/(ms) des phases au pas
  !          de temps precedent
  ! ivista : Viscosite turbulente des phases au pas de temps precedent
  ! icp    : Chaleur specifique des phases
  ! icpa   : Chaleur specifique des phases au pas de temps precedent
  ! itsnsa : Terme source Navier Stokes des phases au pas de temps precedent
  ! itstua : Terme source des grandeurs turbulentes au pas de temps precedent
  ! itssca : Terme source des scalaires au pas de temps precedent
  ! iestim : Estimateur d'erreur pour Navier-Stokes
  ! ifluma : Flux de masse associe aux variables
  ! ifluaa : Flux de masse explicite (plus vu comme un tableau de travail)
  !          associe aux variables
  ! ismago : constante de Smagorinsky dynamique
  ! icour  : Nombre de Courant des phases
  ! ifour  : Nombre de Fourier des phases
  ! iprtot : Pression totale au centre des cellules Ptot=P*+rho*g.(x-x0)
  !                                                             -  - -
  ! ivisma : Viscosite de maillage en ALE (eventuellement orthotrope)

  integer, save :: ipproc(npromx), ipprof(npromx), ipprob(npromx), &
                   irom  , iroma , iviscl,                         &
                   ivisct, ivisla, ivista,                         &
                   icp   , icpa  , itsnsa,                         &
                   itstua, itssca(nscamx),                         &
                   iestim(nestmx)         , ifluma(nvarmx),        &
                   ifluaa(nvarmx), ismago, icour ,                 &
                   ifour , iprtot, ivisma(3)

  ! Position des conditions aux limites
  !  (position dans coefa et coefb des coef (coef. coef.f) relatifs a
  !   une variable donnee)

  ! icoef   : coef numeros 1 (anciens coefa coefb)
  ! icoeff  : coef numeros 2 (anciens coefaf coefbf)
  ! iclrtp  : pointeur dans COEFA et COEFB

  integer, save :: icoef , icoeff , iclrtp(nvarmx,2)

  !=============================================================================

end module numvar
