#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <numeric>
#include <string>
#include <unistd.h>

#ifdef _OPENMP
#include <omp.h>
#endif

#include "mgOnGpuConfig.h"
#include "mgOnGpuTypes.h"
#include "mgOnGpuVectors.h"

#include "Bridge.h"
#include "CPPProcess.h"
#include "MatrixElementKernels.h"
#include "MemoryAccessMatrixElements.h"
#include "MemoryAccessMomenta.h"
#include "MemoryAccessRandomNumbers.h"
#include "MemoryAccessWeights.h"
#include "MemoryBuffers.h"
#include "RamboSamplingKernels.h"
#include "RandomNumberKernels.h"
#include "timermap.h"

#include "epoch_process_id.h"
#define STRINGIFY(s) #s
#define XSTRINGIFY(s) STRINGIFY(s)

#define SEP79 79

bool is_number(const char *s) {
  const char *t = s;
  while (*t != '\0' && isdigit(*t))
    ++t;
  return (int)strlen(s) == t - s;
}

// Disabling fast math is essential here, otherwise results are undefined
// See https://stackoverflow.com/a/40702790 about __attribute__ on gcc
// See https://stackoverflow.com/a/32292725 about __attribute__ on clang
#ifdef __clang__
__attribute__((optnone))
#else
__attribute__((optimize("-fno-fast-math")))
#endif
bool fp_is_abnormal( const fptype& fp )
{
  if ( std::isnan( fp ) ) return true;
  if ( fp != fp ) return true;
  return false;
}

#ifdef __clang__
__attribute__((optnone))
#else
__attribute__((optimize("-fno-fast-math")))
#endif
bool fp_is_zero( const fptype& fp )
{
  if ( fp == 0 ) return true;
  return false;
}

// See https://en.cppreference.com/w/cpp/numeric/math/FP_categories
#ifdef __clang__
__attribute__((optnone))
#else
__attribute__((optimize("-fno-fast-math")))
#endif
const char* fp_show_class( const fptype& fp )
{
  switch( std::fpclassify( fp ) ) {
  case FP_INFINITE:  return "Inf";
  case FP_NAN:       return "NaN";
  case FP_NORMAL:    return "normal";
  case FP_SUBNORMAL: return "subnormal";
  case FP_ZERO:      return "zero";
  default:           return "unknown";
  }
}

#ifdef __clang__
__attribute__((optnone))
#else
__attribute__((optimize("-fno-fast-math")))
#endif
void debug_me_is_abnormal( const fptype& me, int ievtALL )
{
  std::cout << "DEBUG[" << ievtALL << "]"
            << " ME=" << me
            << " fpisabnormal=" << fp_is_abnormal( me )
            << " fpclass=" << fp_show_class( me )
            << " (me==me)=" << ( me == me )
            << " (me==me+1)=" << ( me == me+1 )
            << " isnan=" << std::isnan( me )
            << " isfinite=" << std::isfinite( me )
            << " isnormal=" << std::isnormal( me )
            << " is0=" << ( me == 0 )
            << " is1=" << ( me == 1 )
            << " abs(ME)=" << std::abs( me )
            << " isnan=" << std::isnan( std::abs( me ) )
            << std::endl;
}

int usage(char* argv0, int ret = 1) {
  std::cout << "Usage: " << argv0
            << " [--verbose|-v] [--debug|-d] [--performance|-p] [--json|-j] [--curhst|--curdev|--common] [--rmbhst|--rmbdev]"
            << " [#gpuBlocksPerGrid #gpuThreadsPerBlock] #iterations" << std::endl << std::endl;
  std::cout << "The number of events per iteration is #gpuBlocksPerGrid * #gpuThreadsPerBlock" << std::endl;
  std::cout << "(also in CPU/C++ code, where only the product of these two parameters counts)" << std::endl << std::endl;
  std::cout << "Summary stats are always computed: '-p' and '-j' only control their printout" << std::endl;
  std::cout << "The '-d' flag only enables NaN/abnormal warnings and OMP debugging" << std::endl;
#ifndef __CUDACC__
#ifdef _OPENMP
  std::cout << std::endl << "Use the OMP_NUM_THREADS environment variable to control OMP multi-threading" << std::endl;
  std::cout << "(OMP multithreading will be disabled if OMP_NUM_THREADS is not set)" << std::endl;
#endif
#endif
  return ret;
}

int main(int argc, char **argv)
{
  // Namespaces for CUDA and C++ (FIXME - eventually use the same namespace everywhere...)
#ifdef __CUDACC__
  using namespace mg5amcGpu;
#else
  using namespace mg5amcCpu;
#endif
  
  // DEFAULTS FOR COMMAND LINE ARGUMENTS
  bool verbose = false;
  bool debug = false;
  bool perf = false;
  bool json = false;
  int niter = 0;
  int gpublocks = 1;
  int gputhreads = 32;
  int jsondate = 0;
  int jsonrun = 0;
  int numvec[5] = {0,0,0,0,0};
  int nnum = 0;

  enum class RandomNumberMode{ CommonRandom=0, CurandHost=1, CurandDevice=2 };
#ifdef __CUDACC__
  RandomNumberMode rndgen = RandomNumberMode::CurandDevice; // default on GPU
#elif not defined MGONGPU_HAS_NO_CURAND
  RandomNumberMode rndgen = RandomNumberMode::CurandHost; // default on CPU if build has curand
#else
  RandomNumberMode rndgen = RandomNumberMode::CommonRandom; // default on CPU if build has no curand
#endif

  enum class RamboSamplingMode{ RamboHost=1, RamboDevice=2 };
#ifdef __CUDACC__
  RamboSamplingMode rmbsmp = RamboSamplingMode::RamboDevice; // default on GPU
#else
  RamboSamplingMode rmbsmp = RamboSamplingMode::RamboHost; // default on CPU
#endif

  // READ COMMAND LINE ARGUMENTS
  for ( int argn = 1; argn < argc; ++argn )
  {
    std::string arg = argv[argn];
    if ( ( arg == "--verbose" ) || ( arg == "-v" ) )
    {
      verbose = true;
    }
    else if ( ( arg == "--debug" ) || ( arg == "-d" ) )
    {
      debug = true;
    }
    else if ( ( arg == "--performance" ) || ( arg == "-p" ) )
    {
      perf = true;
    }
    else if ( ( arg == "--json" ) || ( arg == "-j" ) )
    {
      json = true;
    }
    else if ( arg == "--curdev" )
    {
#ifdef __CUDACC__
      rndgen = RandomNumberMode::CurandDevice;
#else
      throw std::runtime_error( "CurandDevice is not supported on CPUs" );
#endif
    }
    else if ( arg == "--curhst" )
    {
#ifndef MGONGPU_HAS_NO_CURAND
      rndgen = RandomNumberMode::CurandHost;
#else
      throw std::runtime_error( "CurandHost is not supported because this application was built without Curand support" );
#endif
    }
    else if ( arg == "--common" )
    {
      rndgen = RandomNumberMode::CommonRandom;
    }
    else if ( arg == "--rmbdev" )
    {
#ifdef __CUDACC__
      rmbsmp = RamboSamplingMode::RamboDevice;
#else
      throw std::runtime_error( "RamboDevice is not supported on CPUs" );
#endif
    }
    else if ( arg == "--rmbhst" )
    {
      rmbsmp = RamboSamplingMode::RamboHost;
    }
    else if ( is_number(argv[argn]) && nnum<5 )
    {
      numvec[nnum++] = atoi( argv[argn] );
    }
    else
    {
      return usage( argv[0] );
    }
  }

  if (nnum == 3 || nnum == 5) {
    gpublocks = numvec[0];
    gputhreads = numvec[1];
    niter = numvec[2];
    if (nnum == 5){
      jsondate = numvec[3];
      jsonrun = numvec[4];
    }
  } else if (nnum == 1) {
    niter = numvec[0];
  } else {
    return usage(argv[0]);
  }

  if (niter == 0)
    return usage(argv[0]);

  if ( rmbsmp == RamboSamplingMode::RamboHost && rndgen == RandomNumberMode::CurandDevice )
  {
#if not defined MGONGPU_HAS_NO_CURAND
    std::cout << "WARNING! RamboHost selected: cannot use CurandDevice, will use CurandHost" << std::endl;
    rndgen = RandomNumberMode::CurandHost;
#else
    std::cout << "WARNING! RamboHost selected: cannot use CurandDevice, will use CommonRandom" << std::endl;
    rndgen = RandomNumberMode::CommonRandom;
#endif
  }
  
  constexpr int neppM = MemoryAccessMomenta::neppM; // AOSOA layout
  constexpr int neppR = MemoryAccessRandomNumbers::neppR; // AOSOA layout

  using mgOnGpu::ntpbMAX;
  if ( gputhreads > ntpbMAX )
  {
    std::cout << "ERROR! #threads/block should be <= " << ntpbMAX << std::endl;
    return usage(argv[0]);
  }

#ifndef __CUDACC__
#ifdef _OPENMP
  // Set OMP_NUM_THREADS equal to 1 if it is not yet set
  char* ompnthr = getenv( "OMP_NUM_THREADS" );
  if ( debug )
  {
    std::cout << "DEBUG: omp_get_num_threads() = " << omp_get_num_threads() << std::endl; // always == 1 here!
    std::cout << "DEBUG: omp_get_max_threads() = " << omp_get_max_threads() << std::endl;
    std::cout << "DEBUG: ${OMP_NUM_THREADS}    = '" << ( ompnthr == 0 ? "[not set]" : ompnthr ) << "'" << std::endl;
  }
  if ( ompnthr == NULL || std::string(ompnthr).find_first_not_of("0123456789") != std::string::npos || atol( ompnthr ) == 0 )
  {
    if ( ompnthr != NULL ) std::cout << "WARNING! OMP_NUM_THREADS is invalid: will use only 1 thread" << std::endl;
    else if ( debug ) std::cout << "DEBUG: OMP_NUM_THREADS is not set: will use only 1 thread" << std::endl;
    omp_set_num_threads( 1 ); // https://stackoverflow.com/a/22816325
    if ( debug ) std::cout << "DEBUG: omp_get_num_threads() = " << omp_get_num_threads() << std::endl; // always == 1 here!
    if ( debug ) std::cout << "DEBUG: omp_get_max_threads() = " << omp_get_max_threads() << std::endl;
  }

  // Fail gently and avoid "Illegal instruction (core dumped)" if the host does not support the requested AVX
  // [NB: this prevents a crash on pmpe04 but not on some github CI nodes]
  auto supportsAvx = [](){
#if defined __AVX512VL__
    bool ok = __builtin_cpu_supports( "avx512vl" );
    const std::string tag = "skylake-avx512 (AVX512VL)";
#elif defined __AVX2__
    bool ok = __builtin_cpu_supports( "avx2" );
    const std::string tag = "haswell (AVX2)";
#elif defined __SSE4_2__
#ifdef __PPC__
    // See https://gcc.gnu.org/onlinedocs/gcc/Basic-PowerPC-Built-in-Functions-Available-on-all-Configurations.html
    bool ok = __builtin_cpu_supports( "vsx" );
    const std::string tag = "powerpc vsx (128bit as in SSE4.2)";
#else
    bool ok = __builtin_cpu_supports( "sse4.2" );
    const std::string tag = "nehalem (SSE4.2)";
#endif
#else
    bool ok = true;
    const std::string tag = "none";
#endif
    if ( tag == "none" )
      std::cout << "INFO: The application does not require the host to support any AVX feature" << std::endl;
    else if ( ok )
      std::cout << "INFO: The application is built for " << tag << " and the host supports it" << std::endl;
    else
      std::cout << "ERROR! The application is built for " << tag << " but the host does not support it" << std::endl;
    return ok;
  };
  if ( ! supportsAvx() ) return 1;
#endif
#endif

  const int ndim = gpublocks * gputhreads; // number of threads in one GPU grid
  const int nevt = ndim; // number of events in one iteration == number of GPU threads
  const int nevtALL = niter*nevt; // total number of ALL events in all iterations

  if (verbose)
    std::cout << "# iterations: " << niter << std::endl;

  // *** START THE NEW TIMERS ***
  mgOnGpu::TimerMap timermap;

  // === STEP 0 - INITIALISE

#ifdef __CUDACC__
  // --- 00. Initialise cuda (call cudaFree to ease cuda profile analysis)
  const std::string cdfrKey = "00 CudaFree";
  timermap.start( cdfrKey );
  //std::cout << "Calling cudaFree... " << std::endl;
  checkCuda( cudaFree( 0 ) ); // SLOW!
  //std::cout << "Calling cudaFree... done" << std::endl;

  // --- Book the tear down at the end of main:
  struct CudaTearDown {
    CudaTearDown(bool print) : _print(print) { }
    ~CudaTearDown() {
      //if ( _print ) std::cout << "Calling cudaDeviceReset()." << std::endl;
      checkCuda( cudaDeviceReset() ); // this is needed by cuda-memcheck --leak-check full
    }
    bool _print{false};
  } cudaTearDown(debug);
#endif

  // --- 0a. Initialise physics process
  const std::string procKey = "0a ProcInit";
  timermap.start( procKey );

  // Create a process object
  CPPProcess process( niter, gpublocks, gputhreads, verbose );

  // Read param_card and set parameters
  process.initProc("../../Cards/param_card.dat");
  const fptype energy = 1500; // historical default, Ecms = 1500 GeV = 1.5 TeV (above the Z peak)
  //const fptype energy = 91.2; // Ecms = 91.2 GeV (Z peak)
  //const fptype energy = 0.100; // Ecms = 100 MeV (well below the Z peak, pure em scattering)
  const int meGeVexponent = -(2 * mgOnGpu::npar - 8);

  // --- 0b. Allocate memory structures
  const std::string alloKey = "0b MemAlloc";
  timermap.start( alloKey );

  // Memory buffers for random numbers
#ifndef __CUDACC__
  HostBufferRandomNumbers hstRnarray( nevt );
#else
  PinnedHostBufferRandomNumbers hstRnarray( nevt );
  DeviceBufferRandomNumbers devRnarray( nevt );
#endif

  // Memory buffers for momenta
#ifndef __CUDACC__
  HostBufferMomenta hstMomenta( nevt );
#else
  PinnedHostBufferMomenta hstMomenta( nevt );
  DeviceBufferMomenta devMomenta( nevt );
#endif

  // Memory buffers for sampling weights
#ifndef __CUDACC__
  HostBufferWeights hstWeights( nevt );
#else
  PinnedHostBufferWeights hstWeights( nevt );
  DeviceBufferWeights devWeights( nevt );
#endif

  // Memory buffers for matrix elements
#ifndef __CUDACC__
  HostBufferMatrixElements hstMatrixElements( nevt );
#else
  PinnedHostBufferMatrixElements hstMatrixElements( nevt );
  DeviceBufferMatrixElements devMatrixElements( nevt );
#endif

  // Memory buffers for the helicity mask
  using mgOnGpu::ncomb; // the number of helicity combinations
#ifndef __CUDACC__
  HostBufferHelicityMask hstIsGoodHel( ncomb );
#else
  PinnedHostBufferHelicityMask hstIsGoodHel( ncomb );
  DeviceBufferHelicityMask devIsGoodHel( ncomb );
#endif

  std::unique_ptr<double[]> genrtimes( new double[niter] );
  std::unique_ptr<double[]> rambtimes( new double[niter] );
  std::unique_ptr<double[]> wavetimes( new double[niter] );
  std::unique_ptr<double[]> wv3atimes( new double[niter] );
  std::unique_ptr<fptype[]> matrixelementALL( new fptype[nevtALL] ); // FIXME: assume process.nprocesses == 1
  std::unique_ptr<fptype[]> weightALL( new fptype[nevtALL] );

  // --- 0c. Create curand or common generator
  const std::string cgenKey = "0c GenCreat";
  timermap.start( cgenKey );
  // Allocate the appropriate RandomNumberKernel
  std::unique_ptr<RandomNumberKernelBase> prnk;
  if ( rndgen == RandomNumberMode::CommonRandom )
  {
    prnk.reset( new CommonRandomNumberKernel( hstRnarray ) );
  }
#ifndef MGONGPU_HAS_NO_CURAND    
  else if ( rndgen == RandomNumberMode::CurandHost )
  {
    const bool onDevice = false;
    prnk.reset( new CurandRandomNumberKernel( hstRnarray, onDevice ) );
  }
#ifdef __CUDACC__
  else
  {
    const bool onDevice = true;
    prnk.reset( new CurandRandomNumberKernel( devRnarray, onDevice ) );
  }
#else
  else
  {
    throw std::logic_error( "CurandDevice is not supported on CPUs" ); // INTERNAL ERROR (no path to this statement)
  }
#endif
#else
  else
  {
    throw std::logic_error( "This application was built without Curand support" ); // INTERNAL ERROR (no path to this statement)
  }
#endif

  // --- 0c. Create rambo sampling kernel [keep this in 0c for the moment]
  std::unique_ptr<SamplingKernelBase> prsk;
  if ( rmbsmp == RamboSamplingMode::RamboHost )
  {
    prsk.reset( new RamboSamplingKernelHost( energy, hstRnarray, hstMomenta, hstWeights, nevt ) );
  }
  else
  {
#ifdef __CUDACC__
    prsk.reset( new RamboSamplingKernelDevice( energy, devRnarray, devMomenta, devWeights, gpublocks, gputhreads ) );
#else
    throw std::logic_error( "RamboDevice is not supported on CPUs" ); // INTERNAL ERROR (no path to this statement)
#endif
  }

  // --- 0c. Create matrix element kernel [keep this in 0c for the moment]
#ifdef __CUDACC__
  MatrixElementKernelDevice mek( devMomenta, devMatrixElements, gpublocks, gputhreads );
#else
  MatrixElementKernelHost mek( hstMomenta, hstMatrixElements, nevt );
#endif

  // **************************************
  // *** START MAIN LOOP ON #ITERATIONS ***
  // **************************************

  for (int iiter = 0; iiter < niter; ++iiter)
  {
    //std::cout << "Iteration #" << iiter+1 << " of " << niter << std::endl;

    // === STEP 1 OF 3

    // *** START THE OLD-STYLE TIMER FOR RANDOM GEN ***
    double genrtime = 0;

    // --- 1a. Seed curand generator (to get same results on host and device)
    // [NB This should not be necessary using the host API: "Generation functions
    // can be called multiple times on the same generator to generate successive
    // blocks of results. For pseudorandom generators, multiple calls to generation
    // functions will yield the same result as a single call with a large size."]
    const unsigned long long seed = 20200805;
    const std::string sgenKey = "1a GenSeed ";
    timermap.start( sgenKey );
    prnk->seedGenerator( seed+iiter );
    genrtime += timermap.stop();

    // --- 1b. Generate all relevant numbers to build nevt events (i.e. nevt phase space points) on the host
    const std::string rngnKey = "1b GenRnGen";
    timermap.start( rngnKey );
    prnk->generateRnarray();
    //std::cout << "Got random numbers" << std::endl;

#ifdef __CUDACC__
    if ( rndgen != RandomNumberMode::CurandDevice )
    {
      // --- 1c. Copy rnarray from host to device
      const std::string htodKey = "1c CpHTDrnd";
      genrtime += timermap.start( htodKey );
      copyDeviceFromHost( devRnarray, hstRnarray );
    }
#endif

    // *** STOP THE OLD-STYLE TIMER FOR RANDOM GEN ***
    genrtime += timermap.stop();

    // === STEP 2 OF 3
    // Fill in particle momenta for each of nevt events on the device

    // *** START THE OLD-STYLE TIMER FOR RAMBO ***
    double rambtime = 0;

    // --- 2a. Fill in momenta of initial state particles on the device
    const std::string riniKey = "2a RamboIni";
    timermap.start( riniKey );
    prsk->getMomentaInitial();
    //std::cout << "Got initial momenta" << std::endl;

    // --- 2b. Fill in momenta of final state particles using the RAMBO algorithm on the device
    // (i.e. map random numbers to final-state particle momenta for each of nevt events)
    const std::string rfinKey = "2b RamboFin";
    rambtime += timermap.start( rfinKey );
    prsk->getMomentaFinal();
    //std::cout << "Got final momenta" << std::endl;

#ifdef __CUDACC__
    if ( rmbsmp == RamboSamplingMode::RamboDevice )
    {
      // --- 2c. CopyDToH Weights
      const std::string cwgtKey = "2c CpDTHwgt";
      rambtime += timermap.start( cwgtKey );
      copyHostFromDevice( hstWeights, devWeights );
      
      // --- 2d. CopyDToH Momenta
      const std::string cmomKey = "2d CpDTHmom";
      rambtime += timermap.start( cmomKey );
      copyHostFromDevice( hstMomenta, devMomenta );
    }
    else
    {
      // --- 2c. CopyHToD Weights
      const std::string cwgtKey = "2c CpHTDwgt";
      rambtime += timermap.start( cwgtKey );
      copyDeviceFromHost( devWeights, hstWeights );
      
      // --- 2d. CopyHToD Momenta
      const std::string cmomKey = "2d CpHTDmom";
      rambtime += timermap.start( cmomKey );
      copyDeviceFromHost( devMomenta, hstMomenta );
    }
#endif

    // *** STOP THE OLD-STYLE TIMER FOR RAMBO ***
    rambtime += timermap.stop();

    // === STEP 3 OF 3
    // Evaluate matrix elements for all nevt events
    // 0d. (Only on the first iteration) Get good helicities [renamed as 0d: this is initialisation!]
    // 3a. Evaluate MEs on the device
    // 3b. Copy MEs back from device to host

    // --- 0d. SGoodHel
    if ( iiter == 0 )
    {
      const std::string ghelKey = "0d SGoodHel";
      timermap.start( ghelKey );
      mek.computeGoodHelicities();
    }

    // *** START THE OLD-STYLE TIMERS FOR MATRIX ELEMENTS (WAVEFUNCTIONS) ***
    double wavetime = 0; // calc plus copy
    double wv3atime = 0; // calc only

    // --- 3a. SigmaKin
    const std::string skinKey = "3a SigmaKin";
    timermap.start( skinKey );
    mek.computeMatrixElements();

    // *** STOP THE NEW OLD-STYLE TIMER FOR MATRIX ELEMENTS (WAVEFUNCTIONS) ***
    wv3atime += timermap.stop(); // calc only
    wavetime += wv3atime; // calc plus copy

#ifdef __CUDACC__
    // --- 3b. CopyDToH MEs
    const std::string cmesKey = "3b CpDTHmes";
    timermap.start( cmesKey );
    copyHostFromDevice( hstMatrixElements, devMatrixElements );
    // *** STOP THE OLD OLD-STYLE TIMER FOR MATRIX ELEMENTS (WAVEFUNCTIONS) ***
    wavetime += timermap.stop(); // calc plus copy
#endif

    // === STEP 4 FINALISE LOOP
    // --- 4a Dump within the loop
    const std::string loopKey = "4a DumpLoop";
    timermap.start(loopKey);
    genrtimes[iiter] = genrtime;
    rambtimes[iiter] = rambtime;
    wavetimes[iiter] = wavetime;
    wv3atimes[iiter] = wv3atime;

    if (verbose)
    {
      std::cout << std::string(SEP79, '*') << std::endl
                << "Iteration #" << iiter+1 << " of " << niter << std::endl;
      if (perf) std::cout << "Wave function time: " << wavetime << std::endl;
    }

    for (int ievt = 0; ievt < nevt; ++ievt) // Loop over all events in this iteration
    {
      if (verbose)
      {
        // Display momenta
        std::cout << "Momenta:" << std::endl;
        for (int ipar = 0; ipar < mgOnGpu::npar; ipar++)
        {
          // NB: 'setw' affects only the next field (of any type)
          std::cout << std::scientific // fixed format: affects all floats (default precision: 6)
                    << std::setw(4) << ipar + 1
                    << std::setw(14) << MemoryAccessMomenta::ieventAccessIp4IparConst( hstMomenta.data(), ievt, 0, ipar )
                    << std::setw(14) << MemoryAccessMomenta::ieventAccessIp4IparConst( hstMomenta.data(), ievt, 1, ipar )
                    << std::setw(14) << MemoryAccessMomenta::ieventAccessIp4IparConst( hstMomenta.data(), ievt, 2, ipar )
                    << std::setw(14) << MemoryAccessMomenta::ieventAccessIp4IparConst( hstMomenta.data(), ievt, 3, ipar )
                    << std::endl
                    << std::defaultfloat; // default format: affects all floats
        }
        std::cout << std::string(SEP79, '-') << std::endl;
        // Display matrix elements
        std::cout << " Matrix element = " << MemoryAccessMatrixElements::ieventAccessConst( hstMatrixElements.data(), ievt )
                  << " GeV^" << meGeVexponent << std::endl; // FIXME: assume process.nprocesses == 1
        std::cout << std::string(SEP79, '-') << std::endl;
      }
      // Fill the arrays with ALL MEs and weights
      // FIXME: assume process.nprocesses == 1
      matrixelementALL[iiter*nevt + ievt] = MemoryAccessMatrixElements::ieventAccessConst( hstMatrixElements.data(), ievt );
      weightALL[iiter*nevt + ievt] = MemoryAccessWeights::ieventAccessConst( hstWeights.data(), ievt );
    }

    if (!(verbose || debug || perf))
    {
      std::cout << ".";
    }

  }

  // **************************************
  // *** END MAIN LOOP ON #ITERATIONS ***
  // **************************************

  // === STEP 8 ANALYSIS
  // --- 8a Analysis: compute stats after the loop
  const std::string statKey = "8a CompStat";
  timermap.start(statKey);

  double sumgtim = 0;
  double sqsgtim = 0;
  double mingtim = genrtimes[0];
  double maxgtim = genrtimes[0];
  for ( int iiter = 0; iiter < niter; ++iiter )
  {
    sumgtim += genrtimes[iiter];
    sqsgtim += genrtimes[iiter]*genrtimes[iiter];
    mingtim = std::min( mingtim, genrtimes[iiter] );
    maxgtim = std::max( maxgtim, genrtimes[iiter] );
  }

  double sumrtim = 0;
  double sqsrtim = 0;
  double minrtim = rambtimes[0];
  double maxrtim = rambtimes[0];
  for ( int iiter = 0; iiter < niter; ++iiter )
  {
    sumrtim += rambtimes[iiter];
    sqsrtim += rambtimes[iiter]*rambtimes[iiter];
    minrtim = std::min( minrtim, rambtimes[iiter] );
    maxrtim = std::max( maxrtim, rambtimes[iiter] );
  }

  double sumwtim = 0;
  double sqswtim = 0;
  double minwtim = wavetimes[0];
  double maxwtim = wavetimes[0];
  for ( int iiter = 0; iiter < niter; ++iiter )
  {
    sumwtim += wavetimes[iiter];
    sqswtim += wavetimes[iiter]*wavetimes[iiter];
    minwtim = std::min( minwtim, wavetimes[iiter] );
    maxwtim = std::max( maxwtim, wavetimes[iiter] );
  }
  double meanwtim = sumwtim / niter;
  //double stdwtim = std::sqrt( sqswtim / niter - meanwtim * meanwtim );

  double sumw3atim = 0;
  double sqsw3atim = 0;
  double minw3atim = wv3atimes[0];
  double maxw3atim = wv3atimes[0];
  for ( int iiter = 0; iiter < niter; ++iiter )
  {
    sumw3atim += wv3atimes[iiter];
    sqsw3atim += wv3atimes[iiter]*wv3atimes[iiter];
    minw3atim = std::min( minw3atim, wv3atimes[iiter] );
    maxw3atim = std::max( maxw3atim, wv3atimes[iiter] );
  }
  double meanw3atim = sumw3atim / niter;
  //double stdw3atim = std::sqrt( sqsw3atim / niter - meanw3atim * meanw3atim );

  int nabn = 0;
  int nzero = 0;
  double minelem = matrixelementALL[0];
  double maxelem = matrixelementALL[0];
  double minweig = weightALL[0];
  double maxweig = weightALL[0];
  for ( int ievtALL = 0; ievtALL < nevtALL; ++ievtALL )
  {
    // The following events are abnormal in a run with "-p 2048 256 12 -d"
    // - check.exe/commonrand: ME[310744,451171,3007871,3163868,4471038,5473927] with fast math
    // - check.exe/curand: ME[578162,1725762,2163579,5407629,5435532,6014690] with fast math
    // - gcheck.exe/curand: ME[596016,1446938] with fast math
    // Debug NaN/abnormal issues
    //if ( ievtALL == 310744 ) // this ME is abnormal both with and without fast math
    //  debug_me_is_abnormal( matrixelementALL[ievtALL], ievtALL );
    //if ( ievtALL == 5473927 ) // this ME is abnormal only with fast math
    //  debug_me_is_abnormal( matrixelementALL[ievtALL], ievtALL );
    // Compute min/max
    if ( fp_is_zero( matrixelementALL[ievtALL] ) ) nzero++;
    if ( fp_is_abnormal( matrixelementALL[ievtALL] ) )
    {
      if ( debug ) // only printed out with "-p -d" (matrixelementALL is not filled without -p)
        std::cout << "WARNING! ME[" << ievtALL << "] is NaN/abnormal" << std::endl;
      nabn++;
      continue;
    }
    minelem = std::min( minelem, (double)matrixelementALL[ievtALL] );
    maxelem = std::max( maxelem, (double)matrixelementALL[ievtALL] );
    minweig = std::min( minweig, (double)weightALL[ievtALL] );
    maxweig = std::max( maxweig, (double)weightALL[ievtALL] );
  }
  double sumelemdiff = 0;
  double sumweigdiff = 0;
  for ( int ievtALL = 0; ievtALL < nevtALL; ++ievtALL )
  {
    // Compute mean from the sum of diff to min
    if ( fp_is_abnormal( matrixelementALL[ievtALL] ) ) continue;
    sumelemdiff += ( matrixelementALL[ievtALL] - minelem );
    sumweigdiff += ( weightALL[ievtALL] - minweig );
  }
  double meanelem = minelem + sumelemdiff / ( nevtALL - nabn );
  double meanweig = minweig + sumweigdiff / ( nevtALL - nabn );
  double sqselemdiff = 0;
  double sqsweigdiff = 0;
  for ( int ievtALL = 0; ievtALL < nevtALL; ++ievtALL )
  {
    // Compute stddev from the squared sum of diff to mean
    if ( fp_is_abnormal( matrixelementALL[ievtALL] ) ) continue;
    sqselemdiff += std::pow( matrixelementALL[ievtALL] - meanelem, 2 );
    sqsweigdiff += std::pow( weightALL[ievtALL] - meanweig, 2 );
  }
  double stdelem = std::sqrt( sqselemdiff / ( nevtALL - nabn ) );
  double stdweig = std::sqrt( sqsweigdiff / ( nevtALL - nabn ) );

  // === STEP 9 FINALISE
  
  std::string rndgentxt;
  if ( rndgen == RandomNumberMode::CommonRandom ) rndgentxt = "COMMON RANDOM HOST";
  else if ( rndgen == RandomNumberMode::CurandHost ) rndgentxt = "CURAND HOST";
  else if ( rndgen == RandomNumberMode::CurandDevice ) rndgentxt = "CURAND DEVICE";
#ifdef __CUDACC__
  rndgentxt += " (CUDA code)";
#else
  rndgentxt += " (C++ code)";
#endif

  // Workflow description summary
  std::string wrkflwtxt;
  // -- CUDA or C++?
#ifdef __CUDACC__
  wrkflwtxt += "CUD:";
#else
  wrkflwtxt += "CPP:";
#endif
  // -- DOUBLE or FLOAT?
#if defined MGONGPU_FPTYPE_DOUBLE
  wrkflwtxt += "DBL+";
#elif defined MGONGPU_FPTYPE_FLOAT
  wrkflwtxt += "FLT+";
#else
  wrkflwtxt += "???+"; // no path to this statement
#endif
  // -- CUCOMPLEX or THRUST or STD complex numbers?
#ifdef __CUDACC__
#if defined MGONGPU_CXTYPE_CUCOMPLEX
  wrkflwtxt += "CUX:";
#elif defined MGONGPU_CXTYPE_THRUST
  wrkflwtxt += "THX:";
#else
  wrkflwtxt += "???:"; // no path to this statement
#endif
#else
  wrkflwtxt += "STX:";
#endif
  // -- COMMON or CURAND HOST or CURAND DEVICE random numbers?
  if ( rndgen == RandomNumberMode::CommonRandom ) wrkflwtxt += "COMMON+";
  else if ( rndgen == RandomNumberMode::CurandHost ) wrkflwtxt += "CURHST+";
  else if ( rndgen == RandomNumberMode::CurandDevice ) wrkflwtxt += "CURDEV+";
  else wrkflwtxt += "??????+"; // no path to this statement
  // -- HOST or DEVICE rambo sampling?
  if ( rmbsmp == RamboSamplingMode::RamboHost ) wrkflwtxt += "RMBHST+";
  else if ( rmbsmp == RamboSamplingMode::RamboDevice ) wrkflwtxt += "RMBDEV+";
  else wrkflwtxt += "??????+"; // no path to this statement
  // -- HOST or DEVICE matrix elements?
#ifdef __CUDACC__
  wrkflwtxt += "MESDEV";
#else
  wrkflwtxt += "MESHST"; // FIXME! allow this also in CUDA (eventually with various simd levels)
#endif
  // -- SIMD matrix elements?
#if !defined MGONGPU_CPPSIMD
  wrkflwtxt += "/none";
#elif defined __AVX512VL__
#ifdef MGONGPU_PVW512
  wrkflwtxt += "/512z";
#else
  wrkflwtxt += "/512y";
#endif
#elif defined __AVX2__
  wrkflwtxt += "/avx2";
#elif defined __SSE4_2__
#ifdef __PPC__
  wrkflwtxt += "/ppcv";
#else
  wrkflwtxt += "/sse4";
#endif
#else
  wrkflwtxt += "/????"; // no path to this statement
#endif

  // --- 9a Dump to screen
  const std::string dumpKey = "9a DumpScrn";
  timermap.start(dumpKey);

  if (!(verbose || debug || perf))
  {
    std::cout << std::endl;
  }

  if (perf)
  {
#ifndef __CUDACC__
    // Get the output of "nproc --all" (https://stackoverflow.com/a/478960)
    std::string nprocall;
    std::unique_ptr<FILE, decltype(&pclose)> nprocpipe( popen( "nproc --all", "r" ), pclose );
    if ( !nprocpipe ) throw std::runtime_error( "`nproc --all` failed?" );
    std::array<char, 128> nprocbuf;
    while ( fgets( nprocbuf.data(), nprocbuf.size(), nprocpipe.get() ) != nullptr ) nprocall += nprocbuf.data();
#endif
#ifdef MGONGPU_CPPSIMD
#ifdef MGONGPU_HAS_CXTYPE_REF
    const std::string cxtref = " [cxtype_ref=YES]";
#else
    const std::string cxtref = " [cxtype_ref=NO]";
#endif
#endif
  // Dump all configuration parameters and all results
    std::cout << std::string(SEP79, '*') << std::endl
#ifdef __CUDACC__
              << "Process                     = " << XSTRINGIFY(MG_EPOCH_PROCESS_ID) << "_CUDA"
#else
              << "Process                     = " << XSTRINGIFY(MG_EPOCH_PROCESS_ID) << "_CPP"
#endif
              << " [" << process.getCompiler() << "]"
#ifdef MGONGPU_INLINE_HELAMPS
              << " [inlineHel=1]"
#else
              << " [inlineHel=0]"
#endif
#ifdef MGONGPU_HARDCODE_CIPC
              << " [hardcodeCIPC=1]" << std::endl
#else
              << " [hardcodeCIPC=0]" << std::endl
#endif
              << "NumBlocksPerGrid            = " << gpublocks << std::endl
              << "NumThreadsPerBlock          = " << gputhreads << std::endl
              << "NumIterations               = " << niter << std::endl
              << std::string(SEP79, '-') << std::endl;
    std::cout << "Workflow summary            = " << wrkflwtxt << std::endl
#if defined MGONGPU_FPTYPE_DOUBLE
              << "FP precision                = DOUBLE (NaN/abnormal=" << nabn << ", zero=" << nzero << ")" << std::endl
#elif defined MGONGPU_FPTYPE_FLOAT
              << "FP precision                = FLOAT (NaN/abnormal=" << nabn << ", zero=" << nzero << ")" << std::endl
#endif
#ifdef __CUDACC__
#if defined MGONGPU_CXTYPE_CUCOMPLEX
              << "Complex type                = CUCOMPLEX" << std::endl
#elif defined MGONGPU_CXTYPE_THRUST
              << "Complex type                = THRUST::COMPLEX" << std::endl
#endif
#else
              << "Complex type                = STD::COMPLEX" << std::endl
#endif
              << "RanNumb memory layout       = AOSOA[" << neppR << "]"
              << ( neppR == 1 ? " == AOS" : "" )
              << " [HARDCODED FOR REPRODUCIBILITY]" << std::endl
              << "Momenta memory layout       = AOSOA[" << neppM << "]"
              << ( neppM == 1 ? " == AOS" : "" ) << std::endl
#ifdef __CUDACC__
      //<< "Wavefunction GPU memory     = LOCAL" << std::endl
#else
#if !defined MGONGPU_CPPSIMD
              << "Internal loops fptype_sv    = SCALAR ('none': ~vector[" << neppV
              << "], no SIMD)" << std::endl
#elif defined __AVX512VL__
#ifdef MGONGPU_PVW512
              << "Internal loops fptype_sv    = VECTOR[" << neppV
              << "] ('512z': AVX512, 512bit)" << cxtref << std::endl
#else
              << "Internal loops fptype_sv    = VECTOR[" << neppV
              << "] ('512y': AVX512, 256bit)" << cxtref << std::endl
#endif
#elif defined __AVX2__
              << "Internal loops fptype_sv    = VECTOR[" << neppV
              << "] ('avx2': AVX2, 256bit)" << cxtref << std::endl
#elif defined __SSE4_2__
              << "Internal loops fptype_sv    = VECTOR[" << neppV
#ifdef __PPC__
              << "] ('sse4': PPC VSX, 128bit)" << cxtref << std::endl
#else
              << "] ('sse4': SSE4.2, 128bit)" << cxtref << std::endl
#endif
#else
#error Internal error: unknown SIMD build configuration
#endif
#endif
              << "Random number generation    = " << rndgentxt << std::endl
#ifndef __CUDACC__
#ifdef _OPENMP
              << "OMP threads / `nproc --all` = " << omp_get_max_threads() << " / " << nprocall // includes a newline
#endif
#endif
      //<< "MatrixElements compiler     = " << process.getCompiler() << std::endl
              << std::string(SEP79, '-') << std::endl
              << "NumberOfEntries             = " << niter << std::endl
              << std::scientific // fixed format: affects all floats (default precision: 6)
              << "TotalTime[Rnd+Rmb+ME] (123) = ( " << sumgtim+sumrtim+sumwtim << std::string(16, ' ') << " )  sec" << std::endl
              << "TotalTime[Rambo+ME]    (23) = ( " << sumrtim+sumwtim << std::string(16, ' ') << " )  sec" << std::endl
              << "TotalTime[RndNumGen]    (1) = ( " << sumgtim << std::string(16, ' ') << " )  sec" << std::endl
              << "TotalTime[Rambo]        (2) = ( " << sumrtim << std::string(16, ' ') << " )  sec" << std::endl
              << "TotalTime[MatrixElems]  (3) = ( " << sumwtim << std::string(16, ' ') << " )  sec" << std::endl
              << "MeanTimeInMatrixElems       = ( " << meanwtim << std::string(16, ' ') << " )  sec" << std::endl
              << "[Min,Max]TimeInMatrixElems  = [ " << minwtim
              << " ,  " << maxwtim << " ]  sec" << std::endl
      //<< "StdDevTimeInMatrixElems     = ( " << stdwtim << std::string(16, ' ') << " )  sec" << std::endl
              << "TotalTime[MECalcOnly]  (3a) = ( " << sumw3atim << std::string(16, ' ') << " )  sec" << std::endl
              << "MeanTimeInMECalcOnly        = ( " << meanw3atim << std::string(16, ' ') << " )  sec" << std::endl
              << "[Min,Max]TimeInMECalcOnly   = [ " << minw3atim
              << " ,  " << maxw3atim << " ]  sec" << std::endl
      //<< "StdDevTimeInMECalcOnly      = ( " << stdw3atim << std::string(16, ' ') << " )  sec" << std::endl
              << std::string(SEP79, '-') << std::endl
      //<< "ProcessID:                  = " << getpid() << std::endl
      //<< "NProcesses                  = " << process.nprocesses << std::endl
              << "TotalEventsComputed         = " << nevtALL << std::endl
              << "EvtsPerSec[Rnd+Rmb+ME](123) = ( " << nevtALL/(sumgtim+sumrtim+sumwtim)
              << std::string(16, ' ') << " )  sec^-1" << std::endl
              << "EvtsPerSec[Rmb+ME]     (23) = ( " << nevtALL/(sumrtim+sumwtim)
              << std::string(16, ' ') << " )  sec^-1" << std::endl
      //<< "EvtsPerSec[RndNumbGen]   (1) = ( " << nevtALL/sumgtim
      //<< std::string(16, ' ') << " )  sec^-1" << std::endl
      //<< "EvtsPerSec[Rambo]        (2) = ( " << nevtALL/sumrtim
      //<< std::string(16, ' ') << " )  sec^-1" << std::endl
              << "EvtsPerSec[MatrixElems] (3) = ( " << nevtALL/sumwtim
              << std::string(16, ' ') << " )  sec^-1" << std::endl
              << "EvtsPerSec[MECalcOnly] (3a) = ( " << nevtALL/sumw3atim
              << std::string(16, ' ') << " )  sec^-1" << std::endl
              << std::defaultfloat; // default format: affects all floats
    std::cout << std::string(SEP79, '*') << std::endl
              << "NumMatrixElems(notAbnormal) = " << nevtALL - nabn << std::endl
              << std::scientific // fixed format: affects all floats (default precision: 6)
              << "MeanMatrixElemValue         = ( " << meanelem
              << " +- " << stdelem/sqrt(nevtALL - nabn) << " )  GeV^" << meGeVexponent << std::endl // standard error
              << "[Min,Max]MatrixElemValue    = [ " << minelem
              << " ,  " << maxelem << " ]  GeV^" << meGeVexponent << std::endl
              << "StdDevMatrixElemValue       = ( " << stdelem << std::string(16, ' ') << " )  GeV^" << meGeVexponent << std::endl
              << "MeanWeight                  = ( " << meanweig
              << " +- " << stdweig/sqrt(nevtALL - nabn) << " )" << std::endl // standard error
              << "[Min,Max]Weight             = [ " << minweig
              << " ,  " << maxweig << " ]" << std::endl
              << "StdDevWeight                = ( " << stdweig << std::string(16, ' ') << " )" << std::endl
              << std::defaultfloat; // default format: affects all floats
  }

  // --- 9b Dump to json
  const std::string jsonKey = "9b DumpJson";
  timermap.start(jsonKey);

  if(json)
  {
    std::string jsonFileName = std::to_string(jsondate) + "-perf-test-run" + std::to_string(jsonrun) + ".json";
    jsonFileName = "./perf/data/" + jsonFileName;

    //Checks if file exists
    std::ifstream fileCheck;
    bool fileExists = false;
    fileCheck.open(jsonFileName);
    if(fileCheck){
      fileExists = true;
      fileCheck.close();
    }

    std::ofstream jsonFile;
    jsonFile.open(jsonFileName, std::ios_base::app);
    if(!fileExists){
      jsonFile << "[" << std::endl;
    }
    else{
      //deleting the last bracket and outputting a ", "
      std::string temp = "truncate -s-1 " + jsonFileName;
      const char *command = temp.c_str();
      if ( system(command) != 0 )
        std::cout << "WARNING! Command '" << temp << "' failed" << std::endl;
      jsonFile << ", " << std::endl;
    }

    jsonFile << "{" << std::endl
             << "\"NumIterations\": " << niter << ", " << std::endl
             << "\"NumThreadsPerBlock\": " << gputhreads << ", " << std::endl
             << "\"NumBlocksPerGrid\": " << gpublocks << ", " << std::endl
#if defined MGONGPU_FPTYPE_DOUBLE
             << "\"FP precision\": " << "\"DOUBLE (NaN/abnormal=" << nabn << ")\"," << std::endl
#elif defined MGONGPU_FPTYPE_FLOAT
             << "\"FP precision\": " << "\"FLOAT (NaN/abnormal=" << nabn << ")\"," << std::endl
#endif
             << "\"Complex type\": "
#ifdef __CUDACC__
#if defined MGONGPU_CXTYPE_CUCOMPLEX
             << "\"CUCOMPLEX\"," << std::endl
#elif defined MGONGPU_CXTYPE_THRUST
             << "\"THRUST::COMPLEX\"," << std::endl
#endif
#else
             << "\"STD::COMPLEX\"," << std::endl
#endif
             << "\"RanNumb memory layout\": " << "\"AOSOA[" << neppR << "]\""
             << ( neppR == 1 ? " == AOS" : "" ) << ", " << std::endl
             << "\"Momenta memory layout\": " << "\"AOSOA[" << neppM << "]\""
             << ( neppM == 1 ? " == AOS" : "" ) << ", " << std::endl
#ifdef __CUDACC__
      //<< "\"Wavefunction GPU memory\": " << "\"LOCAL\"," << std::endl
#endif
             << "\"Curand generation\": "
             << "\"" << rndgentxt << "\"," << std::endl;
    jsonFile << "\"NumberOfEntries\": " << niter << "," << std::endl
      //<< std::scientific // Not sure about this
             << "\"TotalTime[Rnd+Rmb+ME] (123)\": \""
             << std::to_string(sumgtim+sumrtim+sumwtim) << " sec\","
             << std::endl
             << "\"TotalTime[Rambo+ME] (23)\": \""
             << std::to_string(sumrtim+sumwtim) << " sec\"," << std::endl
             << "\"TotalTime[RndNumGen] (1)\": \""
             << std::to_string(sumgtim) << " sec\"," << std::endl
             << "\"TotalTime[Rambo] (2)\": \""
             << std::to_string(sumrtim) << " sec\"," << std::endl
             << "\"TotalTime[MatrixElems] (3)\": \""
             << std::to_string(sumwtim) << " sec\"," << std::endl
             << "\"MeanTimeInMatrixElems\": \""
             << std::to_string(meanwtim) << " sec\"," << std::endl
             << "\"MinTimeInMatrixElems\": \""
             << std::to_string(minwtim) << " sec\"," << std::endl
             << "\"MaxTimeInMatrixElems\": \""
             << std::to_string(maxwtim) << " sec\"," << std::endl
      //<< "ProcessID:                = " << getpid() << std::endl
      //<< "NProcesses                = " << process.nprocesses << std::endl
             << "\"TotalEventsComputed\": " << nevtALL << "," << std::endl
             << "\"EvtsPerSec[Rnd+Rmb+ME](123)\": \""
             << std::to_string(nevtALL/(sumgtim+sumrtim+sumwtim)) << " sec^-1\"," << std::endl
             << "\"EvtsPerSec[Rmb+ME] (23)\": \""
             << std::to_string(nevtALL/(sumrtim+sumwtim)) << " sec^-1\"," << std::endl
             << "\"EvtsPerSec[MatrixElems] (3)\": \""
             << std::to_string(nevtALL/sumwtim) << " sec^-1\"," << std::endl
             << "\"EvtsPerSec[MECalcOnly] (3)\": \""
             << std::to_string(nevtALL/sumw3atim) << " sec^-1\"," << std::endl
             << "\"NumMatrixElems(notAbnormal)\": " << nevtALL - nabn << "," << std::endl
             << std::scientific
             << "\"MeanMatrixElemValue\": "
             << "\"" << std::to_string(meanelem) << " GeV^"
             << std::to_string(meGeVexponent) << "\"," << std::endl
             << "\"StdErrMatrixElemValue\": "
             << "\"" << std::to_string(stdelem/sqrt(nevtALL)) << " GeV^"
             << std::to_string(meGeVexponent) << "\"," << std::endl
             << "\"StdDevMatrixElemValue\": "
             << "\"" << std::to_string(stdelem)
             << " GeV^" << std::to_string(meGeVexponent) << "\"," << std::endl
             << "\"MinMatrixElemValue\": "
             << "\"" << std::to_string(minelem) << " GeV^"
             << std::to_string(meGeVexponent) << "\"," << std::endl
             << "\"MaxMatrixElemValue\": "
             << "\"" << std::to_string(maxelem) << " GeV^"
             << std::to_string(meGeVexponent) <<  "\"," << std::endl;

    timermap.dump(jsonFile, true); // NB For the active json timer this dumps a partial total

    jsonFile << "}" << std::endl << "]";
    jsonFile.close();
  }

  // *** STOP THE NEW TIMERS ***
  timermap.stop();
  if (perf)
  {
    std::cout << std::string(SEP79, '*') << std::endl;
    timermap.dump();
    std::cout << std::string(SEP79, '*') << std::endl;
  }

  // [NB some resources like curand generators will be deleted here when stack-allocated classes go out of scope]
  //std::cout << "ALL OK" << std::endl;
  return 0;
}
