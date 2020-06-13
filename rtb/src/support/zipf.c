
#include <stdlib.h> /* rand, qsort */
#include <math.h> /* pow, ceil */
#include <stdio.h> /* printf */

/* * * * * * * * * * Gray * * * * * * * * * */

#if defined(GRAY)
static inline double randf(void)
{
	return (double)rand() / RAND_MAX;
}

double zeta(double theta, long n)
{
	int i;
	double ans = 0.0;
	for (i = 1; i <= n; i++) ans += pow(1.0/i, theta);
	return(ans);
}

long zipf(double theta, long n)
{
	static int first = 1;
	double zetan, alpha, eta, u, uz;
	if (first) {zetan = zeta(theta, n); first = 0;}
	u = randf();
	uz = u * zetan;
	if (uz < 1.0) return 1;
	if (uz < (1.0 + pow(0.5, theta))) return 2;
	alpha = 1.0 / (1.0 - theta);
	eta = (1.0 - pow(2.0 / n, 1.0 - theta)) / (1.0 - zeta(theta, 2) / zetan);
	return 1 + (long)(n * pow(eta*u - eta + 1.0, alpha));
}

/* * * * * * * * * * USF * * * * * * * * * */

#elif defined(USF)

// #include <assert.h> /* assert */

#define  FALSE          0       // Boolean false
#define  TRUE           1       // Boolean true

double rand_val(int seed)
{
  const long  a =      16807;  // Multiplier
  const long  m = 2147483647;  // Modulus
  const long  q =     127773;  // m div a
  const long  r =       2836;  // m mod a
  static long x =   1L << 30;  // Random int value
  long        x_div_q;         // x divided by q
  long        x_mod_q;         // x modulo q
  long        x_new;           // New x value

  // Set the seed if argument is non-zero and then return zero
  if (seed > 0)
  {
    x = seed;
    return(0.0);
  }

  // RNG using integer arithmetic
  x_div_q = x / q;
  x_mod_q = x % q;
  x_new = (a * x_mod_q) - (r * x_div_q);
  if (x_new > 0)
    x = x_new;
  else
    x = x_new + m;

  // Return a random value between 0.0 and 1.0
  return((double) x / m);
}

int zipf(double alpha, int n)
{
  static int first = TRUE;      // Static first time flag
  static double c = 0;          // Normalization constant
  double z;                     // Uniform random number (0 < z < 1)
  double sum_prob;              // Sum of probabilities
  int    zipf_value;            // Computed exponential value to be returned
  int    i;                     // Loop counter

  // Compute normalization constant on first call only
  if (first == TRUE)
  {
    for (i=1; i<=n; i++)
      c = c + (1.0 / pow((double) i, alpha));
    c = 1.0 / c;
    first = FALSE;
  }

  // Pull a uniform random number (0 < z < 1)
  do
  {
    z = rand_val(0);
  }
  while ((z == 0) || (z == 1));

  // Map z to the value
  sum_prob = 0;
  zipf_value = n;
  for (i=1; i<=n; i++)
  {
    sum_prob = sum_prob + c / pow((double) i, alpha);
    if (sum_prob >= z)
    {
      zipf_value = i;
      break;
    }
  }

  // Assert that zipf_value is between 1 and N
  // assert((zipf_value >=1) && (zipf_value <= n));

  // printf(" %.4f,%d", z, zipf_value);
  return(zipf_value);
}

#endif

// #define N 1000
// #define SAMPLES 100000
// #define N 10000
// #define SAMPLES 10000
// #define N 100000
// #define SAMPLES 1000
#define N (1U << 15)
#define SAMPLES (1U << 11)
// #define N (1U << 24)
// #define SAMPLES (1U << 19)
#define ALPHA 0.99

int cmpi(const void *a, const void *b)
{
  return -(*(int*)a - *(int*)b);
}

int histo[N];

int main(int argc, char *argv[])
{
	unsigned i;
	unsigned samples = 0;

#if defined(GRAY) || defined(USF)
	for (i = 1; i <= SAMPLES; i++) {
		histo[zipf(ALPHA, N)-1]++;
	}
#else

	{
		double zetan = 0.0;
		unsigned n;

		// FIO limits N to 10M
		n = N;
		if (n > (1U << 23)) n = (1U << 23);
		for (i = 1; i <= n; i++) zetan += 1.0/pow((double)i, ALPHA);
		printf("zeta:%f\n", zetan);

#if 1
		{
			unsigned sum = 0;
			// unsigned lcount = SAMPLES;
			for (i = 1; i <= N; i++) {
				double f = 1.0/(pow((double)i,ALPHA)*zetan);
				unsigned count = ceil(f * SAMPLES);
				// if (count < lcount) lcount = count; else count = lcount;
				if (sum+count > SAMPLES) {histo[i-1] = SAMPLES-sum; break;}
				histo[i-1] = count;
				sum += count;
			}
		}
#endif

#if 0
		{
			double k = 0.0;
			unsigned current = 0;
			for (i = 1; i <= N; i++) {
				unsigned target;
				k += 1.0/pow((double)i,ALPHA);
				target = ceil(k/zetan * SAMPLES);
				histo[i-1] = target-current;
				current += histo[i-1];
			}
		}
#endif

#if 0
		{
			int flag = 1;
			for (i = 1; i <= N; i++) {
				double f = 1.0/(pow((double)i,ALPHA)*zetan);
				histo[i-1] = floor(f * SAMPLES + 0.5);
				if (flag && f < 1.0/SAMPLES) {printf("1@rank:%d\n", i); flag = 0;}
			}
		}
#endif

	}
#endif

	qsort(histo, N, sizeof(int), cmpi);
	for (i = 0; i < N; i++) {
		if (i > 0 && histo[i] == 1 && histo[i-1] > 1) {
			printf("one  at rank:%d\n", i+1);
		}
		if (i > 0 && histo[i] == 0 && histo[i-1] > 0) {
			printf("zero at rank:%d\n", i+1);
		}
		samples += histo[i];
	}
	printf("max at rank 1:%d\n", histo[0]);
	printf("min at rank N:%d\n", histo[N-1]);
	printf("ranks:%d\n", N);
	printf("samples expected:%u counted:%u\n", SAMPLES, samples);

#if defined(GRAPH)
	{
		FILE *data;
		FILE *pipe;
		char *dfile = "zipf.dat";

		data = fopen(dfile, "w");
		for (i = 0; i < N; i++) {
			fprintf(data, "%d\t%d\n", i+1, histo[i]);
		}
		fclose(data);

		pipe = popen("gnuplot -persist", "w"); // -persist
		fprintf(pipe, "set title 'Zipf Distribution with %d Ranks and %d Samples'\n", N, SAMPLES);
		fprintf(pipe, "set xlabel 'Rank'\n");
		fprintf(pipe, "set ylabel 'Frequency'\n");
		fprintf(pipe, "set yrange [1:]\n");
		fprintf(pipe, "set logscale xy\n");
		fprintf(pipe, "plot '%s' with points lt 7 title 'skew %.2f'\n", dfile, ALPHA);
		// fprintf(pipe, "set terminal png size 800,600\n");
		// fprintf(pipe, "set output 'zipf.png'\n");
		// fprintf(pipe, "replot\n");
		fflush(pipe);
		// getchar();
		pclose(pipe);
		remove(dfile);
	}
#endif
}
