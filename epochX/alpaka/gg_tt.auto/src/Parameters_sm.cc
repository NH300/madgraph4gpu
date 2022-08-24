//==========================================================================
// This file has been automatically generated for CUDA/C++ standalone by
// MadGraph5_aMC@NLO v. 3.4.0_lo_vect, 2022-05-06
// By the MadGraph5_aMC@NLO Development Team
// Visit launchpad.net/madgraph5 and amcatnlo.web.cern.ch
//==========================================================================

#include <iostream>
#include <iomanip>
#include "Parameters_sm.h"

// Initialize static instance
Parameters_sm* Parameters_sm::instance = 0;

// Function to get static instance - only one instance per program
Parameters_sm* Parameters_sm::getInstance()
{
  if (instance == 0)
    instance = new Parameters_sm();
  return instance;
}

void Parameters_sm::setIndependentParameters(SLHAReader& slha)
{
  // Define "zero"
  zero = 0;
  ZERO = 0;
  // Prepare a vector for indices
  std::vector<int> indices(2, 0);
  mdl_WH = slha.get_block_entry("decay", 25, 6.382339e-03);
  mdl_WW = slha.get_block_entry("decay", 24, 2.047600e+00);
  mdl_WZ = slha.get_block_entry("decay", 23, 2.441404e+00);
  mdl_WT = slha.get_block_entry("decay", 6, 1.491500e+00);
  mdl_ymtau = slha.get_block_entry("yukawa", 15, 1.777000e+00);
  mdl_ymt = slha.get_block_entry("yukawa", 6, 1.730000e+02);
  mdl_ymb = slha.get_block_entry("yukawa", 5, 4.700000e+00);
  aS = slha.get_block_entry("sminputs", 3, 1.180000e-01);
  mdl_Gf = slha.get_block_entry("sminputs", 2, 1.166390e-05);
  aEWM1 = slha.get_block_entry("sminputs", 1, 1.325070e+02);
  mdl_MH = slha.get_block_entry("mass", 25, 1.250000e+02);
  mdl_MZ = slha.get_block_entry("mass", 23, 9.118800e+01);
  mdl_MTA = slha.get_block_entry("mass", 15, 1.777000e+00);
  mdl_MT = slha.get_block_entry("mass", 6, 1.730000e+02);
  mdl_MB = slha.get_block_entry("mass", 5, 4.700000e+00);
  mdl_conjg__CKM3x3 = 1.;
  mdl_conjg__CKM1x1 = 1.;
  mdl_CKM3x3 = 1.;
  mdl_complexi = std::complex<double>(0.,1.);
  mdl_MZ__exp__2 = ((mdl_MZ)*(mdl_MZ));
  mdl_MZ__exp__4 = ((mdl_MZ)*(mdl_MZ)*(mdl_MZ)*(mdl_MZ));
  mdl_sqrt__2 = sqrt(2.);
  mdl_MH__exp__2 = ((mdl_MH)*(mdl_MH));
  mdl_aEW = 1./aEWM1;
  mdl_MW = sqrt(mdl_MZ__exp__2/2.+sqrt(mdl_MZ__exp__4/4.-(mdl_aEW*M_PI*mdl_MZ__exp__2)/(mdl_Gf*mdl_sqrt__2)));
  mdl_sqrt__aEW = sqrt(mdl_aEW);
  mdl_ee = 2.*mdl_sqrt__aEW*sqrt(M_PI);
  mdl_MW__exp__2 = ((mdl_MW)*(mdl_MW));
  mdl_sw2 = 1.-mdl_MW__exp__2/mdl_MZ__exp__2;
  mdl_cw = sqrt(1.-mdl_sw2);
  mdl_sqrt__sw2 = sqrt(mdl_sw2);
  mdl_sw = mdl_sqrt__sw2;
  mdl_g1 = mdl_ee/mdl_cw;
  mdl_gw = mdl_ee/mdl_sw;
  mdl_vev = (2.*mdl_MW*mdl_sw)/mdl_ee;
  mdl_vev__exp__2 = ((mdl_vev)*(mdl_vev));
  mdl_lam = mdl_MH__exp__2/(2.*mdl_vev__exp__2);
  mdl_yb = (mdl_ymb*mdl_sqrt__2)/mdl_vev;
  mdl_yt = (mdl_ymt*mdl_sqrt__2)/mdl_vev;
  mdl_ytau = (mdl_ymtau*mdl_sqrt__2)/mdl_vev;
  mdl_muH = sqrt(mdl_lam*mdl_vev__exp__2);
  mdl_I1x33 = mdl_yb*mdl_conjg__CKM3x3;
  mdl_I2x33 = mdl_yt*mdl_conjg__CKM3x3;
  mdl_I3x33 = mdl_CKM3x3*mdl_yt;
  mdl_I4x33 = mdl_CKM3x3*mdl_yb;
  mdl_ee__exp__2 = ((mdl_ee)*(mdl_ee));
  mdl_sw__exp__2 = ((mdl_sw)*(mdl_sw));
  mdl_cw__exp__2 = ((mdl_cw)*(mdl_cw));
}

void Parameters_sm::setIndependentCouplings()
{
  // (none)
}

void Parameters_sm::setDependentParameters()
{
  mdl_sqrt__aS = sqrt(aS);
  G = 2.*mdl_sqrt__aS*sqrt(M_PI);
  mdl_G__exp__2 = ((G)*(G));
}

void Parameters_sm::setDependentCouplings()
{
  GC_10 = -G;
  GC_11 = mdl_complexi*G;
}

// Routines for printing out parameters
void Parameters_sm::printIndependentParameters()
{
  std::cout << "sm model parameters independent of event kinematics:" << std::endl;
  std::cout << std::setw(20) << "mdl_WH " << "= " << std::setiosflags(std::ios::scientific) << std::setw(10) << mdl_WH << std::endl;
  std::cout << std::setw(20) << "mdl_WW " << "= " << std::setiosflags(std::ios::scientific) << std::setw(10) << mdl_WW << std::endl;
  std::cout << std::setw(20) << "mdl_WZ " << "= " << std::setiosflags(std::ios::scientific) << std::setw(10) << mdl_WZ << std::endl;
  std::cout << std::setw(20) << "mdl_WT " << "= " << std::setiosflags(std::ios::scientific) << std::setw(10) << mdl_WT << std::endl;
  std::cout << std::setw(20) << "mdl_ymtau " << "= " << std::setiosflags(std::ios::scientific) << std::setw(10) << mdl_ymtau << std::endl;
  std::cout << std::setw(20) << "mdl_ymt " << "= " << std::setiosflags(std::ios::scientific) << std::setw(10) << mdl_ymt << std::endl;
  std::cout << std::setw(20) << "mdl_ymb " << "= " << std::setiosflags(std::ios::scientific) << std::setw(10) << mdl_ymb << std::endl;
  std::cout << std::setw(20) << "aS " << "= " << std::setiosflags(std::ios::scientific) << std::setw(10) << aS << std::endl;
  std::cout << std::setw(20) << "mdl_Gf " << "= " << std::setiosflags(std::ios::scientific) << std::setw(10) << mdl_Gf << std::endl;
  std::cout << std::setw(20) << "aEWM1 " << "= " << std::setiosflags(std::ios::scientific) << std::setw(10) << aEWM1 << std::endl;
  std::cout << std::setw(20) << "mdl_MH " << "= " << std::setiosflags(std::ios::scientific) << std::setw(10) << mdl_MH << std::endl;
  std::cout << std::setw(20) << "mdl_MZ " << "= " << std::setiosflags(std::ios::scientific) << std::setw(10) << mdl_MZ << std::endl;
  std::cout << std::setw(20) << "mdl_MTA " << "= " << std::setiosflags(std::ios::scientific) << std::setw(10) << mdl_MTA << std::endl;
  std::cout << std::setw(20) << "mdl_MT " << "= " << std::setiosflags(std::ios::scientific) << std::setw(10) << mdl_MT << std::endl;
  std::cout << std::setw(20) << "mdl_MB " << "= " << std::setiosflags(std::ios::scientific) << std::setw(10) << mdl_MB << std::endl;
  std::cout << std::setw(20) << "mdl_conjg__CKM3x3 " << "= " << std::setiosflags(std::ios::scientific) << std::setw(10) << mdl_conjg__CKM3x3 << std::endl;
  std::cout << std::setw(20) << "mdl_conjg__CKM1x1 " << "= " << std::setiosflags(std::ios::scientific) << std::setw(10) << mdl_conjg__CKM1x1 << std::endl;
  std::cout << std::setw(20) << "mdl_CKM3x3 " << "= " << std::setiosflags(std::ios::scientific) << std::setw(10) << mdl_CKM3x3 << std::endl;
  std::cout << std::setw(20) << "mdl_complexi " << "= " << std::setiosflags(std::ios::scientific) << std::setw(10) << mdl_complexi << std::endl;
  std::cout << std::setw(20) << "mdl_MZ__exp__2 " << "= " << std::setiosflags(std::ios::scientific) << std::setw(10) << mdl_MZ__exp__2 << std::endl;
  std::cout << std::setw(20) << "mdl_MZ__exp__4 " << "= " << std::setiosflags(std::ios::scientific) << std::setw(10) << mdl_MZ__exp__4 << std::endl;
  std::cout << std::setw(20) << "mdl_sqrt__2 " << "= " << std::setiosflags(std::ios::scientific) << std::setw(10) << mdl_sqrt__2 << std::endl;
  std::cout << std::setw(20) << "mdl_MH__exp__2 " << "= " << std::setiosflags(std::ios::scientific) << std::setw(10) << mdl_MH__exp__2 << std::endl;
  std::cout << std::setw(20) << "mdl_aEW " << "= " << std::setiosflags(std::ios::scientific) << std::setw(10) << mdl_aEW << std::endl;
  std::cout << std::setw(20) << "mdl_MW " << "= " << std::setiosflags(std::ios::scientific) << std::setw(10) << mdl_MW << std::endl;
  std::cout << std::setw(20) << "mdl_sqrt__aEW " << "= " << std::setiosflags(std::ios::scientific) << std::setw(10) << mdl_sqrt__aEW << std::endl;
  std::cout << std::setw(20) << "mdl_ee " << "= " << std::setiosflags(std::ios::scientific) << std::setw(10) << mdl_ee << std::endl;
  std::cout << std::setw(20) << "mdl_MW__exp__2 " << "= " << std::setiosflags(std::ios::scientific) << std::setw(10) << mdl_MW__exp__2 << std::endl;
  std::cout << std::setw(20) << "mdl_sw2 " << "= " << std::setiosflags(std::ios::scientific) << std::setw(10) << mdl_sw2 << std::endl;
  std::cout << std::setw(20) << "mdl_cw " << "= " << std::setiosflags(std::ios::scientific) << std::setw(10) << mdl_cw << std::endl;
  std::cout << std::setw(20) << "mdl_sqrt__sw2 " << "= " << std::setiosflags(std::ios::scientific) << std::setw(10) << mdl_sqrt__sw2 << std::endl;
  std::cout << std::setw(20) << "mdl_sw " << "= " << std::setiosflags(std::ios::scientific) << std::setw(10) << mdl_sw << std::endl;
  std::cout << std::setw(20) << "mdl_g1 " << "= " << std::setiosflags(std::ios::scientific) << std::setw(10) << mdl_g1 << std::endl;
  std::cout << std::setw(20) << "mdl_gw " << "= " << std::setiosflags(std::ios::scientific) << std::setw(10) << mdl_gw << std::endl;
  std::cout << std::setw(20) << "mdl_vev " << "= " << std::setiosflags(std::ios::scientific) << std::setw(10) << mdl_vev << std::endl;
  std::cout << std::setw(20) << "mdl_vev__exp__2 " << "= " << std::setiosflags(std::ios::scientific) << std::setw(10) << mdl_vev__exp__2 << std::endl;
  std::cout << std::setw(20) << "mdl_lam " << "= " << std::setiosflags(std::ios::scientific) << std::setw(10) << mdl_lam << std::endl;
  std::cout << std::setw(20) << "mdl_yb " << "= " << std::setiosflags(std::ios::scientific) << std::setw(10) << mdl_yb << std::endl;
  std::cout << std::setw(20) << "mdl_yt " << "= " << std::setiosflags(std::ios::scientific) << std::setw(10) << mdl_yt << std::endl;
  std::cout << std::setw(20) << "mdl_ytau " << "= " << std::setiosflags(std::ios::scientific) << std::setw(10) << mdl_ytau << std::endl;
  std::cout << std::setw(20) << "mdl_muH " << "= " << std::setiosflags(std::ios::scientific) << std::setw(10) << mdl_muH << std::endl;
  std::cout << std::setw(20) << "mdl_I1x33 " << "= " << std::setiosflags(std::ios::scientific) << std::setw(10) << mdl_I1x33 << std::endl;
  std::cout << std::setw(20) << "mdl_I2x33 " << "= " << std::setiosflags(std::ios::scientific) << std::setw(10) << mdl_I2x33 << std::endl;
  std::cout << std::setw(20) << "mdl_I3x33 " << "= " << std::setiosflags(std::ios::scientific) << std::setw(10) << mdl_I3x33 << std::endl;
  std::cout << std::setw(20) << "mdl_I4x33 " << "= " << std::setiosflags(std::ios::scientific) << std::setw(10) << mdl_I4x33 << std::endl;
  std::cout << std::setw(20) << "mdl_ee__exp__2 " << "= " << std::setiosflags(std::ios::scientific) << std::setw(10) << mdl_ee__exp__2 << std::endl;
  std::cout << std::setw(20) << "mdl_sw__exp__2 " << "= " << std::setiosflags(std::ios::scientific) << std::setw(10) << mdl_sw__exp__2 << std::endl;
  std::cout << std::setw(20) << "mdl_cw__exp__2 " << "= " << std::setiosflags(std::ios::scientific) << std::setw(10) << mdl_cw__exp__2 << std::endl;
}

void Parameters_sm::printIndependentCouplings()
{
  std::cout << "sm model couplings independent of event kinematics:" << std::endl;
  // (none)
}

void Parameters_sm::printDependentParameters()
{
  std::cout << "sm model parameters dependent on event kinematics:" << std::endl;
  std::cout << std::setw(20) << "mdl_sqrt__aS " << "= " << std::setiosflags(std::ios::scientific) << std::setw(10) << mdl_sqrt__aS << std::endl;
  std::cout << std::setw(20) << "G " << "= " << std::setiosflags(std::ios::scientific) << std::setw(10) << G << std::endl;
  std::cout << std::setw(20) << "mdl_G__exp__2 " << "= " << std::setiosflags(std::ios::scientific) << std::setw(10) << mdl_G__exp__2 << std::endl;
}

void Parameters_sm::printDependentCouplings()
{
  std::cout << "sm model couplings dependent on event kinematics:" << std::endl;
  std::cout << std::setw(20) << "GC_10 " << "= " << std::setiosflags(std::ios::scientific) << std::setw(10) << GC_10 << std::endl;
  std::cout << std::setw(20) << "GC_11 " << "= " << std::setiosflags(std::ios::scientific) << std::setw(10) << GC_11 << std::endl;
}
