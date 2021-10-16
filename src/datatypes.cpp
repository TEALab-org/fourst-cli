#include "datatypes.hpp"
#include <cstdarg>
#include <cstring>

datatypes::datatypes(std::string s_name, std::string s_type, char c_argument_ptr,
	int s_dim, ...){
	set_str(&ms_name, s_name);
	set_str(&ms_type, s_type);
	mc_argument_ptr = c_argument_ptr;
	mn_dim = s_dim;
	
	// std::va_list valist;
	// std::va_start(valist, mn_dim);
	// for (int i = 0; i < mn_dim; i++) { //access all the arguments assigned to valist
	// 	mv_sizes.push_back(std::va_arg(valist, int));
	// }
	// std::va_end(valist);
}

void datatypes::set_str(char **arr, std::string s_value){
	*arr = (char*) malloc(sizeof (char) * ((int)s_value.size() + 1));

	if (*arr == NULL)
		printf("Failed to allocate memory for variables.");
	std::strcpy(*arr, s_value.c_str());
}

datatypes::~datatypes(){
	free(ms_name);
	free(ms_type);
}