#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <omp.h>

#define N (2048)

float sum;
float a[N][N];
float b[N][N];
float c[N*N];

void matmul()
{
	int	i, j, k;

	#pragma omp parallel private(i,j,k)
	#pragma omp for schedule(static, N/omp_get_num_procs())
	for (i = 0; i < N; i += 1) {
		for (j = 0; j < N; j += 1) {
			a[i][j] = 0;
			for (k = 0; k < N; k += 1) {
				a[i][j] += b[i][k] * c[N*k+j];
			}
		}
	}
}

void init()
{
	int	i, j;

	#pragma omp parallel private(i,j)
	#pragma omp for schedule(static, N/omp_get_num_procs())
	for (i = 0; i < N; i += 1) {
		for (j = 0; j < N; j += 1) {
			b[i][j] = 12 + i * j * 13;
			c[N*i+j] = -13 + i + j * 21;
		}
	}
}

void check()
{
	int	i, j;


	#pragma parallel private(i,j)
	#pragma omp for reduction(+:sum)
	for (i = 0; i < N; i += 1)
		for (j = 0; j < N; j += 1)
			sum += a[i][j];
	printf("sum = %lf\n", sum);
	assert(sum == 2672835994823558168576.0);
}

int main()
{
	omp_set_num_threads(8);

	init();
	matmul();
	check();

	return 0;
}
