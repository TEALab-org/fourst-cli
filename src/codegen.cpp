/*
g++ -std=c++11 -o exec codegen.cpp datatypes.cpp -I.
./exec
vi test.cpp

*/

#include <iostream>
#include <vector>
#include <algorithm>
#include <cstring>
#include <complex.h>
#include <string>
#include <cstdlib>
#include <cmath>
#include <cstdio>

// #include <omp.h>
#include "datatypes.hpp"


using namespace std;

int global_dim = 2;
options global_options;

void generate_fft_forward(FILE *outfp, datatypes &input_buffer, datatypes &output_buffer, datatypes &descriptor){
	string stride = "";
	fprintf(outfp, "void mkl_fft_forward(");
	fprintf(outfp, "%s %c%s, %s %c%s", input_buffer.ms_type, input_buffer.mc_argument_ptr, input_buffer.ms_name,
						output_buffer.ms_type, output_buffer.mc_argument_ptr, output_buffer.ms_name);
	fprintf(outfp, ", int N){\n");
	// if (input_buffer.mn_dim == 1){
	// 	fprintf(outfp, ", int n){\n");
	// }
	// else{
	// 	for (int i = 0; i < input_buffer.mn_dim; i++)
	// 		fprintf(outfp, ", int n%d", i);
	// 	fprintf(outfp, "){\n");
	// }
	stride += "\t";

	// if (global_options.parallel){
	// 	fprintf(outfp, "%s#pragma omp parallel for\n", stride.c_str());
	// }
	// fprintf(outfp, "%sfor (int i = 0; i < ", stride.c_str());
	// if (input_buffer.mn_dim == 1)
	// 	fprintf(outfp, "n; i++)\n");
	// else{
	// 	fprintf(outfp, "n0");
	// 	for (int i = 1; i < input_buffer.mn_dim; i++)
	// 		fprintf(outfp, "*n%d", i);
	// 	fprintf(outfp, "; i++)\n");
	// }
	// stride += "\t";
	// fprintf(outfp, "%s%s[i] = 0.0\n\n", stride.c_str(), output_buffer.ms_name);

	// stride.pop_back();
	fprintf(outfp, "%sDftiComputeForward(%s, %s, %s);\n", stride.c_str(), descriptor.ms_name, input_buffer.ms_name, output_buffer.ms_name);

	fprintf(outfp, "}\n\n");
}

void generate_fft_backward(FILE *outfp, datatypes &input_buffer, datatypes &output_buffer, datatypes &descriptor){
	string stride = "";
	fprintf(outfp, "void mkl_fft_backward(");
	fprintf(outfp, "%s %c%s, %s %c%s", input_buffer.ms_type, input_buffer.mc_argument_ptr, input_buffer.ms_name,
						output_buffer.ms_type, output_buffer.mc_argument_ptr, output_buffer.ms_name);
	
	fprintf(outfp, ", int N){\n");
	// if (input_buffer.mn_dim == 1){
	// 	fprintf(outfp, ", int n){\n");
	// }
	// else{
	// 	for (int i = 0; i < input_buffer.mn_dim; i++)
	// 		fprintf(outfp, ", int n%d", i);
	// 	fprintf(outfp, "){\n");
	// }

	stride += "\t";
	fprintf(outfp, "%sDftiComputeBackward(%s, %s, %s);\n\n", stride.c_str(), descriptor.ms_name, input_buffer.ms_name, output_buffer.ms_name);


	if (global_options.parallel){
		fprintf(outfp, "%s#pragma omp parallel for\n", stride.c_str());
	}
	fprintf(outfp, "%sfor (int i = 0; i < ", stride.c_str());
	if (input_buffer.mn_dim == 1)
		fprintf(outfp, "N; i++)\n");
	else{
		fprintf(outfp, "N");
		for (int i = 1; i < input_buffer.mn_dim; i++)
			fprintf(outfp, "*N");
		fprintf(outfp, "; i++)\n");
	}
	stride += "\t";
	fprintf(outfp, "%s%s[i] /= ", stride.c_str(), output_buffer.ms_name);
	fprintf(outfp, "(N");
	if (input_buffer.mn_dim > 1){
		for (int i = 1; i < input_buffer.mn_dim; i++)
			fprintf(outfp, "*N");
	}
	fprintf(outfp, ");\n");


	fprintf(outfp, "}\n\n");
}

void generate_fft_conv(FILE *outfp, datatypes &real, datatypes &input, datatypes &result,
							datatypes &a_complex, datatypes &odd_mults, datatypes &input_complex){

	fprintf(outfp, "void convolution_fft(%s %c%s, %s %c%s, %s %c%s){\n", 
					real.ms_type, real.mc_argument_ptr, real.ms_name,
					input.ms_type, input.mc_argument_ptr, input.ms_name,
					result.ms_type, result.mc_argument_ptr, result.ms_name);
	string stride = "\t";
	fprintf(outfp, "%sif (T == 0) return ;\n\n", stride.c_str());
	fprintf(outfp, "%smkl_fft_forward(%s, %s, N);\n\n", stride.c_str(), real.ms_name, a_complex.ms_name);

	fprintf(outfp, "%sbool is_initialized = false;\n", stride.c_str());
	fprintf(outfp, "%sint t = T;\n", stride.c_str());
	fprintf(outfp, "%swhile (t > 1){\n", stride.c_str());
	stride += "\t";
	fprintf(outfp, "%sif (t & 1){\n", stride.c_str());
	stride += "\t";
	fprintf(outfp, "%sif (is_initialized == false){\n", stride.c_str());
	stride += "\t";

	if (global_options.parallel){
		fprintf(outfp, "%s#pragma omp parallel for\n", stride.c_str());
	}
	fprintf(outfp, "%sfor (int i = 0; i < ", stride.c_str());
	fprintf(outfp, "N");
	for (int i = 1; i < input.mn_dim; i++)
		fprintf(outfp, "*N");
	fprintf(outfp, "; i++)\n");
	stride += "\t";
	fprintf(outfp, "%s%s[i] = %s[i];\n", stride.c_str(), odd_mults.ms_name, a_complex.ms_name);
	stride.pop_back();
	fprintf(outfp, "%sis_initialized = true;\n", stride.c_str());
	stride.pop_back();
	fprintf(outfp, "%s} else {\n", stride.c_str());
	stride += "\t";

	if (global_options.parallel){
		fprintf(outfp, "%s#pragma omp parallel for\n", stride.c_str());
	}
	fprintf(outfp, "%sfor (int i = 0; i < ", stride.c_str());
	fprintf(outfp, "N");
	for (int i = 1; i < input.mn_dim; i++)
		fprintf(outfp, "*N");
	fprintf(outfp, "; i++)\n");
	stride += "\t";
	fprintf(outfp, "%s%s[i] = %s[i] * %s[i];\n", stride.c_str(), odd_mults.ms_name, odd_mults.ms_name, a_complex.ms_name);
	stride.pop_back();
	stride.pop_back();
	fprintf(outfp, "%s}\n", stride.c_str());
	stride.pop_back();
	fprintf(outfp, "%s}\n", stride.c_str());

	if (global_options.parallel){
		fprintf(outfp, "%s#pragma omp parallel for\n", stride.c_str());
	}
	fprintf(outfp, "%sfor (int i = 0; i < ", stride.c_str());
	fprintf(outfp, "N");
	for (int i = 1; i < input.mn_dim; i++)
		fprintf(outfp, "*N");
	fprintf(outfp, "; i++)\n");
	stride += "\t";
	fprintf(outfp, "%s%s[i] = %s[i] * %s[i];\n", stride.c_str(), a_complex.ms_name, a_complex.ms_name, a_complex.ms_name);
	stride.pop_back();
	fprintf(outfp, "%st /= 2;\n", stride.c_str());
	stride.pop_back();
	fprintf(outfp, "%s}\n", stride.c_str());


	fprintf(outfp, "%sif (is_initialized){\n", stride.c_str());
	stride += "\t";
	if (global_options.parallel){
		fprintf(outfp, "%s#pragma omp parallel for\n", stride.c_str());
	}
	fprintf(outfp, "%sfor (int i = 0; i < ", stride.c_str());
	fprintf(outfp, "N");
	for (int i = 1; i < input.mn_dim; i++)
		fprintf(outfp, "*N");
	fprintf(outfp, "; i++)\n");
	stride += "\t";
	fprintf(outfp, "%s%s[i] = %s[i] * %s[i];\n", stride.c_str(), a_complex.ms_name, a_complex.ms_name, odd_mults.ms_name);
	stride.pop_back();
	stride.pop_back();
	fprintf(outfp, "%s}\n\n", stride.c_str());


	fprintf(outfp, "%smkl_fft_forward(%s, %s, N);\n\n", stride.c_str(), input.ms_name, input_complex.ms_name);

	if (global_options.parallel){
		fprintf(outfp, "%s#pragma omp parallel for\n", stride.c_str());
	}
	fprintf(outfp, "%sfor (int i = 0; i < ", stride.c_str());
	fprintf(outfp, "N");
	for (int i = 1; i < input.mn_dim; i++)
		fprintf(outfp, "*N");
	fprintf(outfp, "; i++)\n");
	stride += "\t";
	fprintf(outfp, "%s%s[i] = %s[i] * %s[i];\n", stride.c_str(), a_complex.ms_name, a_complex.ms_name, input_complex.ms_name);


	stride.pop_back();
	fprintf(outfp, "%smkl_fft_backward(%s, %s, N);\n", stride.c_str(), a_complex.ms_name, result.ms_name);

	fprintf(outfp, "}\n\n");
}

void generate_mkl_init(FILE *outfp, datatypes &descriptor_forward, datatypes &descriptor_backward){
	std::string stride = "";
	fprintf(outfp, "%svoid mkl_init(int n){\n", stride.c_str());

	stride += "\t";
	fprintf(outfp, "%sMKL_LONG status;\n", stride.c_str());
	fprintf(outfp, "%sMKL_LONG len[%d] = {n", stride.c_str(), global_dim);
	for (int i = 1; i < global_dim; i++)
		fprintf(outfp, ", n");
	fprintf(outfp, "};\n\n");

	fprintf(outfp, "%sstatus = DftiCreateDescriptor(&%s, DFTI_DOUBLE, DFTI_REAL, %d, len);\n", stride.c_str(), descriptor_forward.ms_name, global_dim);
	fprintf(outfp, "%sstatus = DftiSetValue(%s, DFTI_PLACEMENT, DFTI_NOT_INPLACE);\n", stride.c_str(), descriptor_forward.ms_name);
	fprintf(outfp, "%sstatus = DftiSetValue(%s, DFTI_CONJUGATE_EVEN_STORAGE, DFTI_COMPLEX_COMPLEX);\n", stride.c_str(), descriptor_forward.ms_name);
	fprintf(outfp, "%sstatus = DftiSetValue(%s, DFTI_PACKED_FORMAT, DFTI_CCE_FORMAT);\n", stride.c_str(), descriptor_forward.ms_name);
	fprintf(outfp, "%sstatus = DftiCommitDescriptor(%s);\n\n", stride.c_str(), descriptor_forward.ms_name);

	fprintf(outfp, "%sstatus = DftiCreateDescriptor(&%s, DFTI_DOUBLE, DFTI_REAL, %d, len);\n", stride.c_str(), descriptor_backward.ms_name, global_dim);
	fprintf(outfp, "%sstatus = DftiSetValue(%s, DFTI_CONJUGATE_EVEN_STORAGE, DFTI_COMPLEX_COMPLEX);\n", stride.c_str(), descriptor_backward.ms_name);
	fprintf(outfp, "%sstatus = DftiSetValue(%s, DFTI_PLACEMENT, DFTI_NOT_INPLACE);\n", stride.c_str(), descriptor_backward.ms_name);
	fprintf(outfp, "%sstatus = DftiSetValue(%s, DFTI_PACKED_FORMAT, DFTI_CCE_FORMAT);\n", stride.c_str(), descriptor_backward.ms_name);
	fprintf(outfp, "%sstatus = DftiCommitDescriptor(%s);\n", stride.c_str(), descriptor_backward.ms_name);

	stride.pop_back();
	fprintf(outfp, "%s}\n\n", stride.c_str());

}


void generate_initializer(FILE *outfp, datatypes &a_complex, datatypes &odd_mults, datatypes &input_complex){

	std::string stride = "";
	fprintf(outfp, "%svoid initialize(){\n", stride.c_str());
	stride += "\t";
	fprintf(outfp, "%smkl_init(N);\n", stride.c_str());

	fprintf(outfp, "%s%s = (%s %c)malloc(sizeof(%s) * N", stride.c_str(), a_complex.ms_name, a_complex.ms_type, a_complex.mc_argument_ptr, a_complex.ms_type);	
	for (int i = 1; i < a_complex.mn_dim; i++)
		fprintf(outfp, " * N");
	fprintf(outfp, ");\n");

	fprintf(outfp, "%s%s = (%s %c)malloc(sizeof(%s) * N", stride.c_str(), odd_mults.ms_name, odd_mults.ms_type, odd_mults.mc_argument_ptr, odd_mults.ms_type);	
	for (int i = 1; i < a_complex.mn_dim; i++)
		fprintf(outfp, " * N");
	fprintf(outfp, ");\n");

	fprintf(outfp, "%s%s = (%s %c)malloc(sizeof(%s) * N", stride.c_str(), input_complex.ms_name, input_complex.ms_type, input_complex.mc_argument_ptr, input_complex.ms_type);	
	for (int i = 1; i < a_complex.mn_dim; i++)
		fprintf(outfp, " * N");
	fprintf(outfp, ");\n\n");

	if (global_options.parallel){
		fprintf(outfp, "%s#pragma omp parallel for\n", stride.c_str());
	}
	for (int i = 0; i < global_dim; i++){
		fprintf(outfp, "%sfor (int i%d = 0; i%d < N; i%d++)\n", stride.c_str(), i, i, i);
		stride += "\t";
	}

	fprintf(outfp, "%sa1[i0]", stride.c_str());
	for (int i = 1; i < global_dim; i++){
		fprintf(outfp, "[i%d]", i);
		stride.pop_back();
	}
	fprintf(outfp, " = 1.0 * (rand() %% BASE);\n");
	stride.pop_back();

	stride.pop_back();
	fprintf(outfp, "%s}\n\n", stride.c_str());
}

void print_coefficients(FILE *outfp, datatypes &real, double *arr, int rem_dim, double *rem_dim_vals, int total_elements, std::string cur_N, int debug_i){
	std::string stride = "\t";
	if (rem_dim == 1){
		for (int i = 0; i < rem_dim_vals[0]; i++){
			// cout << "debug_i: " << debug_i + i << " " << arr[i] << endl;
			if (fabs(arr[i]) > 0)
				fprintf(outfp, "%s%s[%s%d] = %lf;\n", stride.c_str(), real.ms_name, cur_N==""?"":(cur_N + " + ").c_str(), i, arr[i]);
		}
	}
	else{

		std::string next_dim_no_elem = "N";
		for (int i = 1; i < rem_dim - 1; i++)
			next_dim_no_elem += "*N";

		for (int k = 0; k < rem_dim_vals[0]; k++){
			// cout << "rem_dim: " << rem_dim << " tot: " << total_elements << " jump: " << total_elements/rem_dim_vals[0] << " k: " << k << endl;
			print_coefficients(outfp, real, 
					arr + (int)(k*total_elements/rem_dim_vals[0]), 
					rem_dim-1, rem_dim_vals+1, total_elements/rem_dim_vals[0], 
					cur_N == ""?(std::to_string(k) + "*" + next_dim_no_elem): cur_N + "+" + (std::to_string(k) + "*" + next_dim_no_elem), debug_i + (int)(k*total_elements/rem_dim_vals[0]));
		}
	}
}

void generate_create_stencil(FILE *outfp, datatypes &real, double *arr){

	std::string stride = "";
	fprintf(outfp, "%svoid create_stencil(%s %c%s", stride.c_str(), real.ms_type, real.mc_argument_ptr, real.ms_name);
	// if (global_dim == 1){
	// 	fprintf(outfp, ", int n\n");
	// }
	// else{
	// 	for (int i = 0; i < global_dim; i++)
	// 		fprintf(outfp, ", int n%d", i);
	// }
	fprintf(outfp, "){\n");
	stride += "\t";

	if (global_options.parallel){
		fprintf(outfp, "%s#pragma omp parallel for\n", stride.c_str());
	}
	fprintf(outfp, "%sfor (int i = 0; i < ", stride.c_str());
	fprintf(outfp, "N");
	for (int i = 1; i < global_dim; i++)
		fprintf(outfp, "*N");
	fprintf(outfp, "; i++)\n");
	stride += "\t";
	fprintf(outfp, "%s%s[i] = 0.0;\n\n", stride.c_str(), real.ms_name);
	stride.pop_back();

	int offset = global_dim + 1;
	int num_coeff = 1;
	for (int i = 0; i < arr[0]; i++)
		num_coeff *= (arr[i + 1]);
	// cout << "num_coeff: " << num_coeff << " offset: " << offset << endl;
	print_coefficients(outfp, real, arr+offset, global_dim, arr+1, num_coeff, "", 0);

	fprintf(outfp, "%s// TODO: Shift Stencil Matrix to avoid rotation\n", stride.c_str());
	// for (int k = 0; k < num_coeff; k++){
	// 	if (arr[k + offset] > 0.0){
	// 		fprintf(outfp, "%s%s[] = %lf\n", stride.c_str(), real.ms_name, arr[k + offset]);	
	// 	}
	// }

	// for (int k = 0; k < global_dim; k++){
	// 	fprintf(outfp, "%sfor (int i%d = 0; i%d < %d; i%d++)\n", stride.c_str(), k, k, (int)(arr[k+1]*2+1), k);
	// 	stride += "\t";
	// }

	// fprintf(outfp, "%s%s[", stride.c_str(), real.ms_name);
	// for (int i = 0; i < global_dim; i++){
	// 	fprintf(outfp, "i%d", i);
	// 	for (int j = i + 1; j < )
	// }


	stride.pop_back();
	fprintf(outfp, "%s}\n\n", stride.c_str());

}

void generate_mkl_destroy(FILE *outfp, datatypes &descriptor_forward, datatypes &descriptor_backward,
				datatypes &a_complex, datatypes &odd_mults, datatypes &input_complex){

	std::string stride = "";
	fprintf(outfp, "%svoid mkl_destroy(){\n", stride.c_str());

	stride += "\t";
	fprintf(outfp, "%sMKL_LONG status;\n", stride.c_str());


	fprintf(outfp, "%sstatus = DftiFreeDescriptor(%c%s);\n", stride.c_str(), descriptor_forward.mc_argument_ptr, descriptor_forward.ms_name);
	fprintf(outfp, "%sstatus = DftiFreeDescriptor(%c%s);\n\n", stride.c_str(), descriptor_backward.mc_argument_ptr, descriptor_backward.ms_name);

	fprintf(outfp, "%sfree(%s);\n", stride.c_str(), a_complex.ms_name);
	fprintf(outfp, "%sfree(%s);\n", stride.c_str(), odd_mults.ms_name);
	fprintf(outfp, "%sfree(%s);\n", stride.c_str(), input_complex.ms_name);


	stride.pop_back();
	fprintf(outfp, "%s}\n\n", stride.c_str());
}



void generate_header_and_global_decl(FILE *outfp, datatypes &descriptor_forward, datatypes &descriptor_backward,
				datatypes &a_complex, datatypes &odd_mults, datatypes &input_complex){

	fprintf(outfp, "#include <iostream>\n#include <vector>\n#include <algorithm>\n#include <cstring>\n#include <complex.h>\n#include <string>\n#include <cstdlib>\n#include <cmath>\n#include <ctime>\n#include <sys/time.h>\n#include <cstdio>\n#include \"mkl_service.h\"\n#include \"mkl_dfti.h\"\n\n");
	if (global_options.parallel){
		fprintf(outfp, "#include <omp.h>\n\n");
	}

	fprintf(outfp, "#define MAXN  110\n");	

	fprintf(outfp, "int T, N, N_THREADS;\n");
	fprintf(outfp, "const int BASE = 1024;\n");
	fprintf(outfp, "double a1");
	for (int i = 0; i < global_dim; i++)
		fprintf(outfp, "[MAXN]");
	fprintf(outfp, "; // TODO: Use dynamic memory allocation\n");
	fprintf(outfp, "%s %s = NULL;\n", descriptor_forward.ms_type, descriptor_forward.ms_name);
	fprintf(outfp, "%s %s = NULL;\n", descriptor_backward.ms_type, descriptor_backward.ms_name);

	fprintf(outfp, "%s %c%s;\n", a_complex.ms_type, a_complex.mc_argument_ptr, a_complex.ms_name);
	fprintf(outfp, "%s %c%s;\n", odd_mults.ms_type, odd_mults.mc_argument_ptr, odd_mults.ms_name);
	fprintf(outfp, "%s %c%s;\n\n", input_complex.ms_type, input_complex.mc_argument_ptr, input_complex.ms_name);

}

void generate_main(FILE *outfp, datatypes &real, datatypes &input, datatypes &result){

	std::string stride = "";
	fprintf(outfp, "%sint main(int argc, char *argv[]){\n", stride.c_str());

	stride += "\t";
	fprintf(outfp, "%sint t, n, numThreads;\n", stride.c_str());
	fprintf(outfp, "%sif (argc < 4){\n%sstd::cout << \"Enter: N T numThreads\" << std::endl;\n%sreturn 1;\n%s}\n\n", stride.c_str(), (stride+"\t").c_str(), (stride+"\t").c_str(), stride.c_str());
	fprintf(outfp, "%sn = atoi(argv[1]);\n", stride.c_str());
	fprintf(outfp, "%st = atoi(argv[2]);\n", stride.c_str());
	fprintf(outfp, "%snumThreads = atoi(argv[3]);\n", stride.c_str());
	fprintf(outfp, "%somp_set_num_threads(numThreads);\n\n", stride.c_str());

	fprintf(outfp, "%sN = n; T = t; N_THREADS = numThreads;\n", stride.c_str());
	fprintf(outfp, "%s%s %c%s, %c%s, %c%s;\n", stride.c_str(), real.ms_type, real.mc_argument_ptr, real.ms_name,
									input.mc_argument_ptr, input.ms_name, result.mc_argument_ptr, result.ms_name);

	fprintf(outfp, "%s%s = (%s %c)malloc(sizeof(%s) * N", stride.c_str(), real.ms_name, real.ms_type, real.mc_argument_ptr, real.ms_type);	
	for (int i = 1; i < real.mn_dim; i++)
		fprintf(outfp, " * N");
	fprintf(outfp, ");\n");

	fprintf(outfp, "%s%s = (%s %c)malloc(sizeof(%s) * N", stride.c_str(), input.ms_name, input.ms_type, input.mc_argument_ptr, input.ms_type);	
	for (int i = 1; i < input.mn_dim; i++)
		fprintf(outfp, " * N");
	fprintf(outfp, ");\n");

	fprintf(outfp, "%s%s = (%s %c)malloc(sizeof(%s) * N", stride.c_str(), result.ms_name, result.ms_type, result.mc_argument_ptr, result.ms_type);	
	for (int i = 1; i < result.mn_dim; i++)
		fprintf(outfp, " * N");
	fprintf(outfp, ");\n\n");


	fprintf(outfp, "%sinitialize();\n", stride.c_str());
	fprintf(outfp, "%screate_stencil(%s);\n\n", stride.c_str(), real.ms_name);

	if (global_options.parallel){
		fprintf(outfp, "%s#pragma omp parallel for\n", stride.c_str());
	}
	for (int i = 0; i < global_dim; i++){
		fprintf(outfp, "%sfor (int i%d = 0; i%d < N; i%d++)\n", stride.c_str(), i, i, i);
		stride += "\t";
	}

	fprintf(outfp, "%s%s[i0", stride.c_str(), input.ms_name);
	for (int k = 0; k < global_dim - 1; k++)
		fprintf(outfp, "*N");
	for (int i = 1; i < global_dim; i++){
		fprintf(outfp, " + i%d", i);
		for (int k = 0; k < global_dim - i - 1; k++)
			fprintf(outfp, "*N");
	}

	fprintf(outfp, "] = a1[i0]");
	for (int i = 1; i < global_dim; i++){
		fprintf(outfp, "[i%d]", i);
		stride.pop_back();
	}
	fprintf(outfp, ";\n\n");
	stride.pop_back();

	fprintf(outfp, "%sconvolution_fft(%s, %s, %s);\n", stride.c_str(), real.ms_name, input.ms_name, result.ms_name);
	fprintf(outfp, "%smkl_destroy();\n", stride.c_str());

	stride.pop_back();
	fprintf(outfp, "%s}\n\n", stride.c_str());
}

int _gencode(double* arr, FILE* outfp=stdout){
	if (!outfp) {
		// print error code
	}

	global_options.parallel = true;
	global_dim = (int)arr[0];
	for (int i = 0; i < global_dim; i++)
		arr[i + 1] = arr[i + 1] * 2 + 1;


	datatypes descriptor_forward("my_desc1_handle", "DFTI_DESCRIPTOR_HANDLE", '&', global_dim),
				descriptor_backward("my_desc2_handle", "DFTI_DESCRIPTOR_HANDLE", '&', global_dim);

	datatypes a_complex("a_complex", "double complex", '*', global_dim),
				odd_mults("odd_mults", "double complex", '*', global_dim),
				input_complex("input_complex", "double complex", '*', global_dim);

	generate_header_and_global_decl(outfp, descriptor_forward, descriptor_backward, a_complex, odd_mults, input_complex);

	datatypes input_buffer_forward("input_buffer", "double", '*', global_dim), 
				output_buffer_forward("output_buffer", "double complex", '*', global_dim);
	generate_fft_forward(outfp, input_buffer_forward, output_buffer_forward, descriptor_forward);


	datatypes input_buffer_backward("input_buffer", "double complex", '*', global_dim), 
					output_buffer_backward("output_buffer", "double", '*', global_dim);
	generate_fft_backward(outfp, input_buffer_backward, output_buffer_backward, descriptor_backward);


	datatypes real_conv("stencil_real", "double", '*', global_dim), 
				input_conv("input", "double", '*', global_dim), 
				result_conv("result", "double", '*', global_dim);
	generate_fft_conv(outfp, real_conv, input_conv, result_conv, a_complex, odd_mults, input_complex);

	generate_create_stencil(outfp, real_conv, arr);

	generate_mkl_init(outfp, descriptor_forward, descriptor_backward);
	generate_initializer(outfp, a_complex, odd_mults, input_complex);
	generate_mkl_destroy(outfp, descriptor_forward, descriptor_backward, a_complex, odd_mults, input_complex);

	generate_main(outfp, real_conv, input_conv, result_conv);

	return 0;
}
