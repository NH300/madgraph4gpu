//==========================================================================
// This file has been automatically generated for SYCL standalone by
// MadGraph5_aMC@NLO v. 2.9.5, 2021-08-22
// By the MadGraph5_aMC@NLO Development Team
// Visit launchpad.net/madgraph5 and amcatnlo.web.cern.ch
//==========================================================================

#include <algorithm>
#include <array>
#include <cstring>
#include <memory>

#include "mgOnGpuConfig.h"
#include "mgOnGpuTypes.h"
#include "mgOnGpuVectors.h"

#include "CPPProcess.h"
#include "HelAmps_sm.h"

//==========================================================================
// Class member functions for calculating the matrix elements for
// Process: g g > t t~ g WEIGHTED<=3 @1

namespace Proc
{
  using mgOnGpu::np4; // dimensions of 4-momenta (E,px,py,pz)
  using mgOnGpu::npar; // #particles in total (external = initial + final): e.g. 4 for e+ e- -> mu+ mu-
  using mgOnGpu::ncomb; // #helicity combinations: e.g. 16 for e+ e- -> mu+ mu- (2**4 = fermion spin up/down ** npar)

  using mgOnGpu::nwf; // #wavefunctions = #external (npar) + #internal: e.g. 5 for e+ e- -> mu+ mu- (1 internal is gamma or Z)
  using mgOnGpu::nw6; // dimensions of each wavefunction (HELAS KEK 91-11): e.g. 6 for e+ e- -> mu+ mu- (fermions and vectors)

  //--------------------------------------------------------------------------

  // Evaluate |M|^2 for each subprocess
  // NB: calculate_wavefunctions ADDS |M|^2 for a given ihel to the running sum of |M|^2 over helicities for the given event(s)
  SYCL_EXTERNAL
  INLINE
  void calculate_wavefunctions( int ihel,
                                const fptype_sv* allmomenta, // input: momenta as AOSOA[npagM][npar][4][neppM] with nevt=npagM*neppM
                                fptype_sv* allMEs,           // output: allMEs[npagM][neppM], final |M|^2 averaged over helicities
                                size_t ievt,
                                short *cHel,
                                const fptype *cIPC,
                                const fptype *cIPD
                                )
  //ALWAYS_INLINE // attributes are not permitted in a function definition
  {
    using namespace MG5_sm;
    mgDebug( 0, __FUNCTION__ );


    // The number of colors
    constexpr int ncolor = 6;

    // Local TEMPORARY variables for a subset of Feynman diagrams in the given SYCL event (ievt)
    // [NB these variables are reused several times (and re-initialised each time) within the same event or event page]
    cxtype_sv w_sv[nwf][nw6]; // particle wavefunctions within Feynman diagrams (nw6 is often 6, the dimension of spin 1/2 or spin 1 particles)
    cxtype_sv amp_sv[1]; // invariant amplitude for one given Feynman diagram

    // Local variables for the given CUDA event (ievt) or C++ event page (ipagV)
    cxtype_sv jamp_sv[ncolor] = {}; // sum of the invariant amplitudes for all Feynman diagrams in the event or event page

    // === Calculate wavefunctions and amplitudes for all diagrams in all processes - Loop over nevt events ===

    {
      // Reset color flows (reset jamp_sv) at the beginning of a new event or event page
      for( int i=0; i<ncolor; i++ ){ jamp_sv[i] = cxzero_sv(); }

      // *** DIAGRAM 1 OF 16 ***

      // Wavefunction(s) for diagram number 1
      vxxxxx( allmomenta, ievt, 0., cHel[ihel*npar + 0], -1, w_sv[0], 0 );

      vxxxxx( allmomenta, ievt, 0., cHel[ihel*npar + 1], -1, w_sv[1], 1 );

      oxxxxx( allmomenta, ievt, cIPD[0], cHel[ihel*npar + 2], +1, w_sv[2], 2 );

      ixxxxx( allmomenta, ievt, cIPD[0], cHel[ihel*npar + 3], -1, w_sv[3], 3 );

      vxxxxx( allmomenta, ievt, 0., cHel[ihel*npar + 4], +1, w_sv[4], 4 );

      VVV1P0_1( w_sv[0], w_sv[1], cxmake( cIPC[0], cIPC[1] ), 0., 0., w_sv[5] );
      FFV1P0_3( w_sv[3], w_sv[2], cxmake( cIPC[2], cIPC[3] ), 0., 0., w_sv[6] );

      // Amplitude(s) for diagram number 1
      VVV1_0( w_sv[5], w_sv[6], w_sv[4], cxmake( cIPC[0], cIPC[1] ), &amp_sv[0] );
      jamp_sv[0] -= amp_sv[0];
      jamp_sv[2] += +amp_sv[0];
      jamp_sv[4] += +amp_sv[0];
      jamp_sv[5] -= amp_sv[0];

      // *** DIAGRAM 2 OF 16 ***

      // Wavefunction(s) for diagram number 2
      FFV1_1( w_sv[2], w_sv[4], cxmake( cIPC[2], cIPC[3] ), cIPD[0], cIPD[1], w_sv[7] );

      // Amplitude(s) for diagram number 2
      FFV1_0( w_sv[3], w_sv[7], w_sv[5], cxmake( cIPC[2], cIPC[3] ), &amp_sv[0] );
      jamp_sv[4] += +cxtype(0,1)*amp_sv[0];
      jamp_sv[5] -= cxtype(0,1)*amp_sv[0];

      // *** DIAGRAM 3 OF 16 ***

      // Wavefunction(s) for diagram number 3
      FFV1_2( w_sv[3], w_sv[4], cxmake( cIPC[2], cIPC[3] ), cIPD[0], cIPD[1], w_sv[8] );

      // Amplitude(s) for diagram number 3
      FFV1_0( w_sv[8], w_sv[2], w_sv[5], cxmake( cIPC[2], cIPC[3] ), &amp_sv[0] );
      jamp_sv[0] += +cxtype(0,1)*amp_sv[0];
      jamp_sv[2] -= cxtype(0,1)*amp_sv[0];

      // *** DIAGRAM 4 OF 16 ***

      // Wavefunction(s) for diagram number 4
      FFV1_1( w_sv[2], w_sv[0], cxmake( cIPC[2], cIPC[3] ), cIPD[0], cIPD[1], w_sv[5] );
      FFV1_2( w_sv[3], w_sv[1], cxmake( cIPC[2], cIPC[3] ), cIPD[0], cIPD[1], w_sv[9] );

      // Amplitude(s) for diagram number 4
      FFV1_0( w_sv[9], w_sv[5], w_sv[4], cxmake( cIPC[2], cIPC[3] ), &amp_sv[0] );
      jamp_sv[1] -= amp_sv[0];

      // *** DIAGRAM 5 OF 16 ***

      // Wavefunction(s) for diagram number 5
      VVV1P0_1( w_sv[1], w_sv[4], cxmake( cIPC[0], cIPC[1] ), 0., 0., w_sv[10] );

      // Amplitude(s) for diagram number 5
      FFV1_0( w_sv[3], w_sv[5], w_sv[10], cxmake( cIPC[2], cIPC[3] ), &amp_sv[0] );
      jamp_sv[0] += +cxtype(0,1)*amp_sv[0];
      jamp_sv[1] -= cxtype(0,1)*amp_sv[0];

      // *** DIAGRAM 6 OF 16 ***

      // Wavefunction(s) for diagram number 6
      // (none)

      // Amplitude(s) for diagram number 6
      FFV1_0( w_sv[8], w_sv[5], w_sv[1], cxmake( cIPC[2], cIPC[3] ), &amp_sv[0] );
      jamp_sv[0] -= amp_sv[0];

      // *** DIAGRAM 7 OF 16 ***

      // Wavefunction(s) for diagram number 7
      FFV1_2( w_sv[3], w_sv[0], cxmake( cIPC[2], cIPC[3] ), cIPD[0], cIPD[1], w_sv[5] );
      FFV1_1( w_sv[2], w_sv[1], cxmake( cIPC[2], cIPC[3] ), cIPD[0], cIPD[1], w_sv[11] );

      // Amplitude(s) for diagram number 7
      FFV1_0( w_sv[5], w_sv[11], w_sv[4], cxmake( cIPC[2], cIPC[3] ), &amp_sv[0] );
      jamp_sv[3] -= amp_sv[0];

      // *** DIAGRAM 8 OF 16 ***

      // Wavefunction(s) for diagram number 8
      // (none)

      // Amplitude(s) for diagram number 8
      FFV1_0( w_sv[5], w_sv[2], w_sv[10], cxmake( cIPC[2], cIPC[3] ), &amp_sv[0] );
      jamp_sv[3] += +cxtype(0,1)*amp_sv[0];
      jamp_sv[5] -= cxtype(0,1)*amp_sv[0];

      // *** DIAGRAM 9 OF 16 ***

      // Wavefunction(s) for diagram number 9
      // (none)

      // Amplitude(s) for diagram number 9
      FFV1_0( w_sv[5], w_sv[7], w_sv[1], cxmake( cIPC[2], cIPC[3] ), &amp_sv[0] );
      jamp_sv[5] -= amp_sv[0];

      // *** DIAGRAM 10 OF 16 ***

      // Wavefunction(s) for diagram number 10
      VVV1P0_1( w_sv[0], w_sv[4], cxmake( cIPC[0], cIPC[1] ), 0., 0., w_sv[5] );

      // Amplitude(s) for diagram number 10
      FFV1_0( w_sv[3], w_sv[11], w_sv[5], cxmake( cIPC[2], cIPC[3] ), &amp_sv[0] );
      jamp_sv[2] += +cxtype(0,1)*amp_sv[0];
      jamp_sv[3] -= cxtype(0,1)*amp_sv[0];

      // *** DIAGRAM 11 OF 16 ***

      // Wavefunction(s) for diagram number 11
      // (none)

      // Amplitude(s) for diagram number 11
      FFV1_0( w_sv[9], w_sv[2], w_sv[5], cxmake( cIPC[2], cIPC[3] ), &amp_sv[0] );
      jamp_sv[1] += +cxtype(0,1)*amp_sv[0];
      jamp_sv[4] -= cxtype(0,1)*amp_sv[0];

      // *** DIAGRAM 12 OF 16 ***

      // Wavefunction(s) for diagram number 12
      // (none)

      // Amplitude(s) for diagram number 12
      VVV1_0( w_sv[5], w_sv[1], w_sv[6], cxmake( cIPC[0], cIPC[1] ), &amp_sv[0] );
      jamp_sv[1] += +amp_sv[0];
      jamp_sv[2] -= amp_sv[0];
      jamp_sv[3] += +amp_sv[0];
      jamp_sv[4] -= amp_sv[0];

      // *** DIAGRAM 13 OF 16 ***

      // Wavefunction(s) for diagram number 13
      // (none)

      // Amplitude(s) for diagram number 13
      FFV1_0( w_sv[8], w_sv[11], w_sv[0], cxmake( cIPC[2], cIPC[3] ), &amp_sv[0] );
      jamp_sv[2] -= amp_sv[0];

      // *** DIAGRAM 14 OF 16 ***

      // Wavefunction(s) for diagram number 14
      // (none)

      // Amplitude(s) for diagram number 14
      FFV1_0( w_sv[9], w_sv[7], w_sv[0], cxmake( cIPC[2], cIPC[3] ), &amp_sv[0] );
      jamp_sv[4] -= amp_sv[0];

      // *** DIAGRAM 15 OF 16 ***

      // Wavefunction(s) for diagram number 15
      // (none)

      // Amplitude(s) for diagram number 15
      VVV1_0( w_sv[0], w_sv[10], w_sv[6], cxmake( cIPC[0], cIPC[1] ), &amp_sv[0] );
      jamp_sv[0] += +amp_sv[0];
      jamp_sv[1] -= amp_sv[0];
      jamp_sv[3] -= amp_sv[0];
      jamp_sv[5] += +amp_sv[0];

      // *** DIAGRAM 16 OF 16 ***

      // Wavefunction(s) for diagram number 16
      VVVV1P0_1( w_sv[0], w_sv[1], w_sv[4], cxmake( cIPC[4], cIPC[5] ), 0., 0., w_sv[10] );
      VVVV3P0_1( w_sv[0], w_sv[1], w_sv[4], cxmake( cIPC[4], cIPC[5] ), 0., 0., w_sv[6] );
      VVVV4P0_1( w_sv[0], w_sv[1], w_sv[4], cxmake( cIPC[4], cIPC[5] ), 0., 0., w_sv[9] );

      // Amplitude(s) for diagram number 16
      FFV1_0( w_sv[3], w_sv[2], w_sv[10], cxmake( cIPC[2], cIPC[3] ), &amp_sv[0] );
      jamp_sv[0] += +amp_sv[0];
      jamp_sv[1] -= amp_sv[0];
      jamp_sv[3] -= amp_sv[0];
      jamp_sv[5] += +amp_sv[0];
      FFV1_0( w_sv[3], w_sv[2], w_sv[6], cxmake( cIPC[2], cIPC[3] ), &amp_sv[0] );
      jamp_sv[1] -= amp_sv[0];
      jamp_sv[2] += +amp_sv[0];
      jamp_sv[3] -= amp_sv[0];
      jamp_sv[4] += +amp_sv[0];
      FFV1_0( w_sv[3], w_sv[2], w_sv[9], cxmake( cIPC[2], cIPC[3] ), &amp_sv[0] );
      jamp_sv[0] -= amp_sv[0];
      jamp_sv[2] += +amp_sv[0];
      jamp_sv[4] += +amp_sv[0];
      jamp_sv[5] -= amp_sv[0];

      // *** COLOR ALGEBRA BELOW ***
      // (This method used to be called CPPProcess::matrix_1_gg_ttxg()?)

      // The color matrix (initialize all array elements, with ncolor=1)
      // [NB do keep 'static' for these constexpr arrays, see issue #283]
      constexpr fptype denom[ncolor] = {9, 9, 9, 9, 9, 9};
      constexpr fptype cf[ncolor][ncolor] = {
      {64, -8, -8, 1, 1, 10},
      {-8, 64, 1, 10, -8, 1},
      {-8, 1, 64, -8, 10, 1},
      {1, 10, -8, 64, 1, -8},
      {1, -8, 10, 1, 64, -8},
      {10, 1, 1, -8, -8, 64}};

      // Sum and square the color flows to get the matrix element
      // (compute |M|^2 by squaring |M|, taking into account colours)
      fptype_sv deltaMEs = {0}; // all zeros
      for( int icol = 0; icol < ncolor; icol++ )
      {
        cxtype_sv ztemp_sv = cxzero_sv();
        for( int jcol = 0; jcol < ncolor; jcol++ )
          ztemp_sv += cf[icol][jcol] * jamp_sv[jcol];
        deltaMEs += cxreal( ztemp_sv * cxconj( jamp_sv[icol] ) ) / denom[icol];
      }

      // *** STORE THE RESULTS ***

      // Store the leading color flows for choice of color
      // (NB: jamp2_sv must be an array of fptype_sv)
      // for( int icol = 0; icol < ncolor; icol++ )
      // jamp2_sv[0][icol] += cxreal( jamp_sv[icol]*cxconj( jamp_sv[icol] ) );

      // NB: calculate_wavefunctions ADDS |M|^2 for a given ihel to the running sum of |M|^2 over helicities for the given event(s)
      // FIXME: assume process.nprocesses == 1 for the moment (eventually: need a loop over processes here?)
      allMEs[ievt] += deltaMEs;
    }
    mgDebug( 1, __FUNCTION__ );
    return;
  }

  //--------------------------------------------------------------------------

  CPPProcess::CPPProcess( int numiterations,
                          int ngpublocks,
                          int ngputhreads,
                          bool verbose,
                          bool debug )
    : m_numiterations( numiterations )
    , m_ngpublocks( ngpublocks )
    , m_ngputhreads( ngputhreads )
    , m_verbose( verbose )
    , m_debug( debug )
    , m_pars( 0 )
    , m_masses()
    , m_tHel {
      {-1, -1, -1, -1, -1},
      {-1, -1, -1, -1, 1},
      {-1, -1, -1, 1, -1},
      {-1, -1, -1, 1, 1},
      {-1, -1, 1, -1, -1},
      {-1, -1, 1, -1, 1},
      {-1, -1, 1, 1, -1},
      {-1, -1, 1, 1, 1},
      {-1, 1, -1, -1, -1},
      {-1, 1, -1, -1, 1},
      {-1, 1, -1, 1, -1},
      {-1, 1, -1, 1, 1},
      {-1, 1, 1, -1, -1},
      {-1, 1, 1, -1, 1},
      {-1, 1, 1, 1, -1},
      {-1, 1, 1, 1, 1},
      {1, -1, -1, -1, -1},
      {1, -1, -1, -1, 1},
      {1, -1, -1, 1, -1},
      {1, -1, -1, 1, 1},
      {1, -1, 1, -1, -1},
      {1, -1, 1, -1, 1},
      {1, -1, 1, 1, -1},
      {1, -1, 1, 1, 1},
      {1, 1, -1, -1, -1},
      {1, 1, -1, -1, 1},
      {1, 1, -1, 1, -1},
      {1, 1, -1, 1, 1},
      {1, 1, 1, -1, -1},
      {1, 1, 1, -1, 1},
      {1, 1, 1, 1, -1},
      {1, 1, 1, 1, 1}}
  {
  }

  //--------------------------------------------------------------------------

  CPPProcess::~CPPProcess() {}

  //--------------------------------------------------------------------------

  // Initialize process (with parameters read from user cards)
  void CPPProcess::initProc( const std::string& param_card_name )
  {
    // Instantiate the model class and set parameters that stay fixed during run
    m_pars = Parameters_sm::getInstance();
    SLHAReader slha( param_card_name, m_verbose );
    m_pars->setIndependentParameters( slha );
    m_pars->setIndependentCouplings();
    m_pars->setDependentParameters();
    m_pars->setDependentCouplings();
    if ( m_verbose )
    {
      m_pars->printIndependentParameters();
      m_pars->printIndependentCouplings();
      m_pars->printDependentParameters();
      m_pars->printDependentCouplings();
    }
    // Set external particle masses for this matrix element
    m_masses.push_back( m_pars->ZERO );
    m_masses.push_back( m_pars->ZERO );
    m_masses.push_back( m_pars->mdl_MT );
    m_masses.push_back( m_pars->mdl_MT );
    m_masses.push_back( m_pars->ZERO );
    // Read physics parameters like masses and couplings from user configuration files (static: initialize once)
    m_tIPC[0] = cxmake( m_pars->GC_10 );
m_tIPC[1] = cxmake( m_pars->GC_11 );
m_tIPC[2] = cxmake( m_pars->GC_12 );
m_tIPD[0] = (fptype)m_pars->mdl_MT;
m_tIPD[1] = (fptype)m_pars->mdl_WT;

  }

  //--------------------------------------------------------------------------
  // Define pointer accessors
  const short* CPPProcess::get_tHel_ptr() const {return &(**m_tHel);}

  cxtype* CPPProcess::get_tIPC_ptr() {return m_tIPC;}
  const cxtype* CPPProcess::get_tIPC_ptr() const {return m_tIPC;}

  fptype* CPPProcess::get_tIPD_ptr() {return m_tIPD;}
  const fptype* CPPProcess::get_tIPD_ptr() const {return m_tIPD;}

  //--------------------------------------------------------------------------

  SYCL_EXTERNAL
  void sigmaKin_getGoodHel( const fptype* allmomenta, // input: momenta[nevt*npar*4]
                            fptype* allMEs,           // output: allMEs[nevt], |M|^2 final_avg_over_helicities
                            bool* isGoodHel,          // output: isGoodHel[ncomb] - device array
                            const size_t ievt,
                            short* cHel,
                            const fptype* cIPC,
                            const fptype* cIPD
                            )
  {
    // FIXME: assume process.nprocesses == 1 for the moment (eventually: need a loop over processes here?)
    fptype allMEsLast = 0;
    for ( int ihel = 0; ihel < ncomb; ihel++ )
    {
      // NB: calculate_wavefunctions ADDS |M|^2 for a given ihel to the running sum of |M|^2 over helicities for the given event(s)
      calculate_wavefunctions( ihel, allmomenta, allMEs, ievt, cHel, cIPC, cIPD );
      if ( allMEs[ievt] != allMEsLast )
      {
        //if ( !isGoodHel[ihel] ) std::cout << "sigmaKin_getGoodHel ihel=" << ihel << " TRUE" << std::endl;
        isGoodHel[ihel] = true;
      }
      allMEsLast = allMEs[ievt]; // running sum up to helicity ihel for event ievt
    }
  }

  //--------------------------------------------------------------------------

  int sigmaKin_setGoodHel( const bool* isGoodHel, int* goodHel ) // input: isGoodHel[ncomb] - host array
  {
    int nGoodHel = 0; // FIXME: assume process.nprocesses == 1 for the moment (eventually nGoodHel[nprocesses]?)
    for ( int ihel = 0; ihel < ncomb; ihel++ )
    {
      //std::cout << "sigmaKin_setGoodHel ihel=" << ihel << ( isGoodHel[ihel] ? " true" : " false" ) << std::endl;
      if ( isGoodHel[ihel] )
      {
        //goodHel[nGoodHel[0]] = ihel; // FIXME: assume process.nprocesses == 1 for the moment
        //nGoodHel[0]++; // FIXME: assume process.nprocesses == 1 for the moment
        goodHel[nGoodHel] = ihel;
        nGoodHel++;
      }
    }
    return nGoodHel;
  }

  //--------------------------------------------------------------------------
  // Evaluate |M|^2, part independent of incoming flavour
  // FIXME: assume process.nprocesses == 1 (eventually: allMEs[nevt] -> allMEs[nevt*nprocesses]?)

  SYCL_EXTERNAL
  void sigmaKin( const fptype* allmomenta, // input: momenta[nevt*npar*4]
                 fptype* allMEs,           // output: allMEs[nevt], |M|^2 final_avg_over_helicities
                 size_t ievt,
                 short* cHel,
                 const fptype* cIPC,
                 const fptype* cIPD,
                 int* cNGoodHel,
                 int* cGoodHel
                 )
  {
    mgDebugInitialise();

    // Denominators: spins, colors and identical particles
    const int denominators = 256; // FIXME: assume process.nprocesses == 1 for the moment (eventually denominators[nprocesses]?)

    // Set the parameters which change event by event
    // Need to discuss this with Stefan
    //m_pars->setDependentParameters();
    //m_pars->setDependentCouplings();

    // Start sigmaKin_lines
    // PART 0 - INITIALISATION (before calculate_wavefunctions)
    // Reset the "matrix elements" - running sums of |M|^2 over helicities for the given event
    // FIXME: assume process.nprocesses == 1 for the moment (eventually: need a loop over processes here?)
    allMEs[ievt] = 0;

    // PART 1 - HELICITY LOOP: CALCULATE WAVEFUNCTIONS
    // (in both CUDA and C++, using precomputed good helicities)
    // FIXME: assume process.nprocesses == 1 for the moment (eventually: need a loop over processes here?)
    for ( int ighel = 0; ighel < cNGoodHel[0]; ighel++ )
    {
      const int ihel = cGoodHel[ighel];
      calculate_wavefunctions( ihel, allmomenta, allMEs, ievt, cHel, cIPC, cIPD );
    }

    // PART 2 - FINALISATION (after calculate_wavefunctions)
    // Get the final |M|^2 as an average over helicities/colors of the running sum of |M|^2 over helicities for the given event
    // [NB 'sum over final spins, average over initial spins', eg see
    // https://www.uzh.ch/cmsssl/physik/dam/jcr:2e24b7b1-f4d7-4160-817e-47b13dbf1d7c/Handout_4_2016-UZH.pdf]
    // FIXME: assume process.nprocesses == 1 for the moment (eventually: need a loop over processes here?)
    allMEs[ievt] /= (fptype)denominators;
    mgDebugFinalise();
  }

  //--------------------------------------------------------------------------

} // end namespace

//==========================================================================

