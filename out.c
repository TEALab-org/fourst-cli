#include <hello>
#include <this>
#include <is>
#include <a>

here is some code

#include <iostream>
#include <vector>
#include <algorithm>
#include <cstring>
#include <complex.h>
#include <string>
#include <cstdlib>
#include <cmath>
#include <ctime>
#include <sys/time.h>
#include <cstdio>
#include "mkl_service.h"
#include "mkl_dfti.h"

#include <omp.h>

Here is a super cool prefix woohoo!


#define MAXN  110
int T, N, N_THREADS;
const int BASE = 1024;
double a1[MAXN][MAXN][MAXN]; // TODO: Use dynamic memory allocation
DFTI_DESCRIPTOR_HANDLE my_desc1_handle = NULL;
DFTI_DESCRIPTOR_HANDLE my_desc2_handle = NULL;
double complex *a_complex;
double complex *odd_mults;
double complex *input_complex;

void mkl_fft_forward(double *input_buffer, double complex *output_buffer, int N){
	DftiComputeForward(my_desc1_handle, input_buffer, output_buffer);
}

void mkl_fft_backward(double complex *input_buffer, double *output_buffer, int N){
	DftiComputeBackward(my_desc2_handle, input_buffer, output_buffer);

	#pragma omp parallel for
	for (int i = 0; i < N*N*N; i++)
		output_buffer[i] /= (N*N*N);
}

void convolution_fft(double *stencil_real, double *input, double *result){
	if (T == 0) return ;

	mkl_fft_forward(stencil_real, a_complex, N);

	bool is_initialized = false;
	int t = T;
	while (t > 1){
		if (t & 1){
			if (is_initialized == false){
				#pragma omp parallel for
				for (int i = 0; i < N*N*N; i++)
					odd_mults[i] = a_complex[i];
				is_initialized = true;
			} else {
				#pragma omp parallel for
				for (int i = 0; i < N*N*N; i++)
					odd_mults[i] = odd_mults[i] * a_complex[i];
			}
		}
		#pragma omp parallel for
		for (int i = 0; i < N*N*N; i++)
			a_complex[i] = a_complex[i] * a_complex[i];
		t /= 2;
	}
	if (is_initialized){
		#pragma omp parallel for
		for (int i = 0; i < N*N*N; i++)
			a_complex[i] = a_complex[i] * odd_mults[i];
	}

	mkl_fft_forward(input, input_complex, N);

	#pragma omp parallel for
	for (int i = 0; i < N*N*N; i++)
		a_complex[i] = a_complex[i] * input_complex[i];
	mkl_fft_backward(a_complex, result, N);
}

void create_stencil(double *stencil_real){
	#pragma omp parallel for
	for (int i = 0; i < N*N*N; i++)
		stencil_real[i] = 0.0;

	stencil_real[0*N*N+10*N + 0] = -1.500000;
	stencil_real[2*N*N+17*N + 0] = 0.000000;
	stencil_real[2*N*N+18*N + 0] = 0.000000;
	stencil_real[2*N*N+19*N + 0] = 0.000000;
	stencil_real[2*N*N+20*N + 0] = 0.000000;
	// TODO: Shift Stencil Matrix to avoid rotation
}

void mkl_init(int n){
	MKL_LONG status;
	MKL_LONG len[3] = {n, n, n};

	status = DftiCreateDescriptor(&my_desc1_handle, DFTI_DOUBLE, DFTI_REAL, 3, len);
	status = DftiSetValue(my_desc1_handle, DFTI_PLACEMENT, DFTI_NOT_INPLACE);
	status = DftiSetValue(my_desc1_handle, DFTI_CONJUGATE_EVEN_STORAGE, DFTI_COMPLEX_COMPLEX);
	status = DftiSetValue(my_desc1_handle, DFTI_PACKED_FORMAT, DFTI_CCE_FORMAT);
	status = DftiCommitDescriptor(my_desc1_handle);

	status = DftiCreateDescriptor(&my_desc2_handle, DFTI_DOUBLE, DFTI_REAL, 3, len);
	status = DftiSetValue(my_desc2_handle, DFTI_CONJUGATE_EVEN_STORAGE, DFTI_COMPLEX_COMPLEX);
	status = DftiSetValue(my_desc2_handle, DFTI_PLACEMENT, DFTI_NOT_INPLACE);
	status = DftiSetValue(my_desc2_handle, DFTI_PACKED_FORMAT, DFTI_CCE_FORMAT);
	status = DftiCommitDescriptor(my_desc2_handle);
}

void initialize(){
	mkl_init(N);
	a_complex = (double complex *)malloc(sizeof(double complex) * N * N * N);
	odd_mults = (double complex *)malloc(sizeof(double complex) * N * N * N);
	input_complex = (double complex *)malloc(sizeof(double complex) * N * N * N);

	#pragma omp parallel for
	for (int i0 = 0; i0 < N; i0++)
		for (int i1 = 0; i1 < N; i1++)
			for (int i2 = 0; i2 < N; i2++)
				a1[i0][i1][i2] = 1.0 * (rand() % BASE);
}

void mkl_destroy(){
	MKL_LONG status;
	status = DftiFreeDescriptor(&my_desc1_handle);
	status = DftiFreeDescriptor(&my_desc2_handle);

	free(a_complex);
	free(odd_mults);
	free(input_complex);
}

int main(int argc, char *argv[]){
	int t, n, numThreads;
	if (argc < 4){
		std::cout << "Enter: N T numThreads" << std::endl;
		return 1;
	}

	n = atoi(argv[1]);
	t = atoi(argv[2]);
	numThreads = atoi(argv[3]);
	omp_set_num_threads(numThreads);

	N = n; T = t; N_THREADS = numThreads;
	double *stencil_real, *input, *result;
	stencil_real = (double *)malloc(sizeof(double) * N * N * N);
	input = (double *)malloc(sizeof(double) * N * N * N);
	result = (double *)malloc(sizeof(double) * N * N * N);

	initialize();
	create_stencil(stencil_real);

	#pragma omp parallel for
	for (int i0 = 0; i0 < N; i0++)
		for (int i1 = 0; i1 < N; i1++)
			for (int i2 = 0; i2 < N; i2++)
				input[i0*N*N + i1*N + i2] = a1[i0][i1][i2];

	convolution_fft(stencil_real, input, result);
	mkl_destroy();
}



here is some code beyond

Here is a super cool suffix woot woot!
