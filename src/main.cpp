#include <iostream>
#include <vector>
#include <string.h>

#include "codegen.hpp"
#include "datatypes.hpp"

using namespace std;

int main() {
    int n_dim, n_coeffs = 1;
    double c;
    vector <double> dimensions;
    vector <int> dim;
    vector <double> coeffs;

    // get number of dimensions
    if (!(cin >> n_dim)) {
        cerr << "invalid input!" << endl;
        return 1;
    }
    
    // get size of dimensions
    for (int i = 0; i < n_dim; i++) {
        if (!(cin >> c)) {
            cerr << "invalid input!" << endl;
            return 1;
        }
        dimensions.push_back(int((c - 1)) / 2);
        dim.push_back(c);
        n_coeffs *= c;
    }

    // get coefficients
    for (int i = 0; i < dimensions.size(); i++) {
        for (int j = 0; j < dimensions[i]; j++) {
            if (!(cin >> c)) {
                cerr << "invalid input!" << endl;
                return 1;
            }
            coeffs.push_back(c);
        }
    }

    double* arr = new double[n_coeffs];
    arr[0] = n_dim;
    memcpy(arr + 1, &dimensions[0], dimensions.size() * sizeof(double));
    memcpy(arr + 1 + dimensions.size(), &coeffs[0], coeffs.size() * sizeof(double));

    _gencode(arr);

    delete arr;
}