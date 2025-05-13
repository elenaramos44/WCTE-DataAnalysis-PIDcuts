#!/bin/bash

dir=/opt
echo "Top directory is: " $dir

#set ROOTSYS and source thisroot.sh
#unset ROOTSYS
#export ROOTSYS=${dir}/root
echo "ROOTSYS = " $ROOTSYS

#if [[ -e "${ROOTSYS}/bin/thisroot.sh" ]]; then
#    source ${ROOTSYS}/bin/thisroot.sh
#else
#    echo "Cannot source bin/thisroot.sh"
#fi

#unset and set GEANT4 base directory, then source necessary GEANT4 scripts
source /opt/geant4/bin/geant4.sh
#unset GEANT4_BASE_DIR
#export GEANT4_BASE_DIR=${dir}/geant4.10.01.p03/install
echo "GEANT4 directory = /opt/geant4" $GEANT4_BASE_DIR

#if [[ -e "${GEANT4_BASE_DIR}/bin/geant4.sh" ]]; then
#    source ${GEANT4_BASE_DIR}/bin/geant4.sh
#else 
#    echo "cannot source geant4.sh"
#fi

#if [[ -e "${GEANT4_BASE_DIR}/share/Geant4-10.1.3/geant4make/geant4make.sh" ]]; then
#    source ${GEANT4_BASE_DIR}/share/Geant4-10.1.3/geant4make/geant4make.sh
#else 
#    echo "cannot source geant4make.sh"
#fi

#set WCSIMDIR and working directory for WCSim
source /opt/WCSim/build/this_wcsim.sh
#source /opt/WCSim_1.12.12/build/bin/this_wcsim.sh
#unset WCSIMDIR
export WCSIMDIR=/opt/WCSim
echo "WCSim directory WCSIMDIR = " $WCSIMDIR

#unset G4WORKDIR
#export G4WORKDIR=${WCSIMDIR}/exe

#set FITQUN_ROOT
#unset FITQUN_ROOT
#export FITQUN_ROOT=/opt/fiTQun
export FITQUN_ROOT=/opt/MVfiTQun
#export FITQUN_ROOT=/opt/fiTQun_2024_06
echo "FITQUN_ROOT = " $FITQUN_ROOT
