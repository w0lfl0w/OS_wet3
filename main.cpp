//
// Created by wolflow on 1/11/23.
//

#include <cstdlib>
#include <iostream>
#include <fstream>
#include <sstream>
#include <list>
#include <cmath>


#define MAX_SOCKET_MSG_SIZE 516 // that size is in bytes
#define MAX_UNSIGNED_SHORT 65535

using std::FILE;
using std::string;
using std::cout;
using std::endl;
using std::cerr;
using std::ifstream;
using std::stringstream;


int main(int argc, char **argv) {

    /// checkig for valid arguments - the command line is in the for of ./ttftps <port> <timeout> <max_num_of_resends>
    bool arguments_flag = true;
    arguments_flag = (argc == 4) && (atoi(argv[1]) <= MAX_UNSIGNED_SHORT) && (atoi(argv[1]) >= 0) &&
                     (atoi(argv[2]) <= MAX_UNSIGNED_SHORT) && (atoi(argv[2]) >= 0) &&
                     (atoi(argv[3]) <= MAX_UNSIGNED_SHORT) && (atoi(argv[3]) >= 0);

    if (!arguments_flag) {
        cerr << "TTFPT_ERROR: illegal arguments" << endl;
        exit(0);
    }


    return 0;
}

