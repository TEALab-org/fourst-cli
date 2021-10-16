#include <string>
#include <vector>
using namespace std;

class datatypes
{
public:
	// prefix m is for member variable followed by datatype: s for string, n for integer, v for vector
	char *ms_name;
	char *ms_type;
	char mc_argument_ptr;
	int mn_dim;
	std::vector<int> mv_size;

	datatypes();
	datatypes(std::string s_name, std::string s_type, char c_argument_ptr,
		int s_dim = 0, ...);

	void set_str(char **arr, std::string sName);
	
	
	~datatypes();	
};

class options
{
public:
	bool parallel;
};