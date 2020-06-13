/* -*- mode: C; tab-width: 2; indent-tabs-mode: nil; fill-column: 79; coding: iso-latin-1-unix -*- */
/* tstdgemm.c
 */

#ifdef __cplusplus
extern "C" {
#endif
#include "HPL_auxil.h"
#include "hpcc.h"
#ifdef __cplusplus
}
#endif

#include "lime.h"

#ifndef CLOCKS_PER_SEC // from time.h
/* Used to generate a seed */
int
time(void *t)
{
  static int val = 0;
  return ++val;
}
#endif

/* Generates random matrix with entries between 0.0 and 1.0 */
static void
dmatgen(int m, int n, double *a, int lda, int seed) {
  int i, j;
  double *a0 = a, rcp = 1.0 / RAND_MAX;

  srand( seed );

  for (j = 0; j < n; j++) {
    for (i = 0; i < m; i++)
      a0[i] = rcp * rand();

    a0 += lda;
  }
}

static double
dnrm_inf(int m, int n, double *a, int lda) {
  int i, j, k, lnx;
  double mx, *a0;

  int nx = 10;
  double x[10] = {0};

  mx = 0.0;

  for (i = 0; i < m; i += nx) {
    lnx = Mmin( nx, m-i );
    for (k = 0; k < lnx; ++k) x[k] = 0.0;

    a0 = a + i;

    for (j = 0; j < n; ++j) {
      for (k = 0; k < lnx; ++k)
        x[k] += fabs( a0[k] );

      a0 += lda;
    }

    for (k = 0; k < lnx; ++k)
      if (mx < x[k]) mx = x[k];
  }

  return mx;
}

int
HPCC_TestDGEMM(HPCC_Params *params, int doIO, double *UGflops, int *Un, int *Ufailure) {
  int n, lda, ldb, ldc, failure = 1;
  double *a, *b, *c, *x, *y, *z, alpha, beta, sres, cnrm, xnrm;
  double Gflops = 0.0, dn;
  double /*t0,*/ t1;
  tick_t t2, t3, t4;
  long l_n;
  FILE *outFile = NULL;
  int seed_a, seed_b, seed_c, seed_x;

  if (doIO) {
    if (params->outFname[0] == '\0') outFile = stdout;
    else outFile = fopen(params->outFname, "a");
    if (! outFile) {
      outFile = stderr;
      fprintf( outFile, "Cannot open output file.\n" );
      return 1;
    }
  }

  n = (int)(sqrt( params->HPLMaxProcMem / sizeof(double) / 3 + 0.25 ) - 0.5);
  if (n < 0) n = -n; /* if 'n' has overflown an integer */
  l_n = n;
  lda = ldb = ldc = n;

  a = HPCC_XMALLOC( double, l_n * l_n );
  b = HPCC_XMALLOC( double, l_n * l_n );
  c = HPCC_XMALLOC( double, l_n * l_n );

  x = HPCC_XMALLOC( double, l_n );
  y = HPCC_XMALLOC( double, l_n );
  z = HPCC_XMALLOC( double, l_n );

  if (! a || ! b || ! c || ! x || ! y || ! z) {
    goto comp_end;
  }

  seed_a = (int)time( NULL );
  dmatgen( n, n, a, n, seed_a );

  seed_b = (int)time( NULL );
  dmatgen( n, n, b, n, seed_b );

  seed_c = (int)time( NULL );
  dmatgen( n, n, c, n, seed_c );

  seed_x = (int)time( NULL );
  dmatgen( n, 1, x, n, seed_x );

  alpha = a[n / 2];
  beta  = b[n / 2];

  if (doIO) fprintf( outFile, "Matrix dimension = %d x %d\n", n, n );

  CLOCKS_EMULATE
  CACHE_BARRIER(NULL)
  TRACE_START
  STATS_START
  /* assume input data is in memory and not cached (flushed and invalidated) */

  tget(t2);
//  t0 = MPI_Wtime();
  HPL_dgemm( HplColumnMajor, HplNoTrans, HplNoTrans, n, n, n, alpha, a, n, b, n, beta, c, n );
//  t1 = MPI_Wtime();
  tget(t3);
  host::cache_flush(); /* flush all */
  tget(t4);

  /* assume output data is in memory (flushed) */
  CACHE_BARRIER(NULL)
  STATS_STOP
  TRACE_STOP
  CLOCKS_NORMAL
  t1 = tesec(t4,t2);

//  t1 -= t0;
  dn = (double)n;
  if (t1 != 0.0 && t1 != -0.0)
    Gflops = 2.0e-9 * dn * dn * dn / t1;
  else
    Gflops = 0.0;

  cnrm = dnrm_inf( n, n, c, n );
  xnrm = dnrm_inf( n, 1, x, n );

  /* y <- c*x */
  HPL_dgemv( HplColumnMajor, HplNoTrans, n, n, 1.0, c, ldc, x, 1, 0.0, y, 1 );

  /* z <- b*x */
  HPL_dgemv( HplColumnMajor, HplNoTrans, n, n, 1.0, b, ldb, x, 1, 0.0, z, 1 );

  /* y <- alpha * a * z - y */
  HPL_dgemv( HplColumnMajor, HplNoTrans, n, n, alpha, a, lda, z, 1, -1.0, y, 1 );

  dmatgen( n, n, c, n, seed_c );

  /* y <- beta * c_orig * x + y */
  HPL_dgemv( HplColumnMajor, HplNoTrans, n, n, beta, c, ldc, x, 1, 1.0, y, 1 );

  sres = dnrm_inf( n, 1, y, n ) / cnrm / xnrm / n / HPL_dlamch( HPL_MACH_EPS );

  if (doIO) {
    fprintf( outFile, "Scaled residual: %g\n", sres );
    fprintf( outFile, "Real time used = %.6f seconds\n", t1 );
    fprintf( outFile, "Single DGEMM Gflop/s %.6f\n", Gflops );
    fprintf( outFile, "Oper. time: %f sec\n", tesec(t3,t2));
    fprintf( outFile, "Cache time: %f sec\n", tesec(t4,t3));
    STATS_PRINT
  }

  if (sres < 16.0 /*params->test.thrsh*/)
    failure = 0;

  comp_end:

  if (z) HPCC_free( z );
  if (y) HPCC_free( y );
  if (x) HPCC_free( x );
  if (c) HPCC_free( c );
  if (b) HPCC_free( b );
  if (a) HPCC_free( a );

  if (doIO) {
    fflush( outFile );
    if (params->outFname[0] != '\0') fclose( outFile );
  }

  if (UGflops) *UGflops = Gflops;
  if (Un) *Un = n;
  if (Ufailure) *Ufailure = failure;

  return 0;
}
