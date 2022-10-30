#include "outfile.h"

void output_to_file(string output_file) {
    freopen(output_file.c_str(), "w", stdout);
}