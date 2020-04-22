/* From: https://doi.org/10.1145/191843.191886 */
/* Jim Gray, et al. */
/* Quickly generating billion-record synthetic databases */
/* 1994 */

#include <stdlib.h> /* rand, qsort */
#include <math.h> /* pow */
#include <stdio.h> /* printf */

double randf(void)
{
	return (double)rand() / RAND_MAX;
}

double zeta(long n, double theta)
{
	int i;
	double ans = 0.0;
	for (i = 1; i <= n; i++) ans += pow(1.0/i, theta);
	return(ans);
}

long zipf(long n, double theta)
{
	double alpha = 1.0 / (1.0 - theta);
	double zetan = zeta(n, theta);
	double eta = (1.0 - pow(2.0 / n, 1.0 - theta)) / (1.0 - zeta(2, theta) / zetan);
	double u = randf();
	double uz = u * zetan;
	if (uz < 1.0) return 1;
	if (uz < (1.0 + pow(0.5, theta))) return 2;
	return 1 + (long)(n * pow(eta*u - eta + 1.0, alpha));
}
