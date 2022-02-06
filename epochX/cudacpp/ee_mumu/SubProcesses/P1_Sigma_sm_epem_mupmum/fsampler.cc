#include "mgOnGpuConfig.h"

#include "Bridge.h"
#include "CPPProcess.h"
#include "MemoryBuffers.h"
#include "RamboSamplingKernels.h"
#include "RandomNumberKernels.h"

//--------------------------------------------------------------------------

#ifndef __CUDACC__
namespace mg5amcCpu
{

  template<typename FORTRANFPTYPE>
  class Sampler
  {
  public:

    // Constructor
    // @param nevtF (NB_PAGE, vector.inc) number of events in Fortran arrays
    // @param nparF (NEXTERNAL, nexternal.inc) number of external particles in Fortran arrays (KEPT FOR SANITY CHECKS ONLY: remove it?)
    // @param np4F number of momenta components, usually 4, in Fortran arrays (KEPT FOR SANITY CHECKS ONLY: remove it?)
    Sampler( int nevtF, int nparF, int np4F );

    // Destructor
    ~Sampler(){}

    // Delete copy/move constructors and assignment operators
    Sampler( const Sampler&  ) = delete;
    Sampler( Sampler&&  ) = delete;
    Sampler& operator=( const Sampler& ) = delete;
    Sampler& operator=( Sampler&& ) = delete;

    // Draw random numbers and convert them to momenta in C++, then transpose them to Fortran momenta
    void samplerHostSequence( FORTRANFPTYPE* fortranMomenta )
    {
      // HARDCODED DEFAULTS
      constexpr bool verbose = false;
      constexpr int gpublocks = 4;
      constexpr int gputhreads = 32;
      constexpr int niter = 2;
      constexpr int ndim = gpublocks * gputhreads; // number of threads in one GPU grid
      constexpr int nevt = ndim; // number of events in one iteration == number of GPU threads

      // --- 0a. Initialise physics process

      // Create a process object
      CPPProcess process( niter, gpublocks, gputhreads, verbose );

      // Read param_card and set parameters
      process.initProc( "../../Cards/param_card.dat" );
      constexpr fptype energy = 1500; // historical default, Ecms = 1500 GeV = 1.5 TeV (above the Z peak)

      // --- 0b. Allocate memory structures

      // Memory buffers for random numbers
      HostBufferRandomNumbers hstRnarray( nevt );

      // Memory buffers for momenta
      HostBufferMomenta hstMomenta( nevt );

      // Memory buffers for sampling weights
      HostBufferWeights hstWeights( nevt );

      // --- 0c. Create curand or common generator

      // Allocate the appropriate RandomNumberKernel
      std::unique_ptr<RandomNumberKernelBase> prnk;
      prnk.reset( new CommonRandomNumberKernel( hstRnarray ) );

      // --- 0c. Create rambo sampling kernel
      std::unique_ptr<SamplingKernelBase> prsk;
      prsk.reset( new RamboSamplingKernelHost( energy, hstRnarray, hstMomenta, hstWeights, nevt ) );

      // Memory buffers for matrix elements
      //HostBufferMatrixElements hstMatrixElements( nevt );

      // --- 0c. Create cross section kernel [keep this in 0c for the moment]
      //EventStatistics hstStats;
      //CrossSectionKernelHost xsk( hstWeights, hstMatrixElements, hstStats, nevt );

      // **************************************
      // *** START MAIN LOOP ON #ITERATIONS ***
      // **************************************
      int iiter = 0; // FIXME
      //std::cout << "Iteration #" << iiter+1 << std::endl;

      // === STEP 1 OF 3

      // --- 1a. Seed curand generator (to get same results on host and device)
      // [NB This should not be necessary using the host API: "Generation functions
      // can be called multiple times on the same generator to generate successive
      // blocks of results. For pseudorandom generators, multiple calls to generation
      // functions will yield the same result as a single call with a large size."]
      const unsigned long long seed = 20200805;
      prnk->seedGenerator( seed+iiter );

      // --- 1b. Generate all relevant numbers to build nevt events (i.e. nevt phase space points) on the host
      prnk->generateRnarray();
      //std::cout << "Got random numbers" << std::endl;

      // === STEP 2 OF 3

      // --- 2a. Fill in momenta of initial state particles on the device
      prsk->getMomentaInitial();
      //std::cout << "Got initial momenta" << std::endl;

      // --- 2b. Fill in momenta of final state particles using the RAMBO algorithm on the device
      // (i.e. map random numbers to final-state particle momenta for each of nevt events)
      prsk->getMomentaFinal();
      //std::cout << "Got final momenta" << std::endl;

      // --- 2c. TransposeC2F
      hst_transposeMomentaC2F( hstMomenta.data(), fortranMomenta, nevt );
    }

  };

}
#endif

//--------------------------------------------------------------------------

#ifndef __CUDACC__
extern "C"
{
  using namespace mg5amcCpu;

  /**
   * The floating point precision used in Fortran arrays.
   * This is presently hardcoded to double precision (REAL*8).
   */
  using FORTRANFPTYPE = double; // for Fortran double precision (REAL*8) arrays
  //using FORTRANFPTYPE = float; // for Fortran single precision (REAL*4) arrays

  /**
   * Create a Sampler and return its pointer.
   * This is a C symbol that should be called from the Fortran code (in auto_dsig1.f).
   *
   * @param ppsampler the pointer to the Sampler pointer (the Sampler pointer is handled in Fortran as an INTEGER*8 variable)
   * @param nevtF the pointer to the number of events in the Fortran arrays
   * @param nparF the pointer to the number of external particles in the Fortran arrays (KEPT FOR SANITY CHECKS ONLY)
   * @param np4F the pointer to the number of momenta components, usually 4, in the Fortran arrays (KEPT FOR SANITY CHECKS ONLY)
   */
  void fsamplercreate_( Sampler<FORTRANFPTYPE>** ppsampler, const int* pnevtF, const int* pnparF, const int* pnp4F )
  {
    *ppsampler = new Sampler<FORTRANFPTYPE>( *pnevtF, *pnparF, *pnp4F );
  }

  /**
   * Delete a Sampler.
   * This is a C symbol that should be called from the Fortran code (in auto_dsig1.f).
   *
   * @param ppsampler the pointer to the Sampler pointer (the Sampler pointer is handled in Fortran as an INTEGER*8 variable)
   */
  void fsamplerdelete_( Sampler<FORTRANFPTYPE>** ppsampler )
  {
    delete *ppsampler;
  }

  /**
   * Execute the matrix-element calculation "sequence" via a Sampler on GPU/CUDA or CUDA/C++.
   * This is a C symbol that should be called from the Fortran code (in auto_dsig1.f).
   *
   * @param ppsampler the pointer to the Sampler pointer (the Sampler pointer is handled in Fortran as an INTEGER*8 variable)
   * @param momenta the pointer to the input 4-momenta
   * @param mes the pointer to the output matrix elements
   */
  void fsamplersequence_( Sampler<FORTRANFPTYPE>** ppsampler, FORTRANFPTYPE* momenta )
  {
    // Use the host/CPU implementation
    // (there is no device implementation)
    (*ppsampler)->samplerHostSequence( momenta );
  }

}
#endif

//--------------------------------------------------------------------------
