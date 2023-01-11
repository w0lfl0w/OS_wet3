//
// Created by wolflow on 1/11/23.
//

#include <cstdlib>
#include <iostream>
#include <fstream>
#include <sstream>
#include <list>
#include <cmath>
#include <sys/socket.h>


#define MAX_SOCKET_MSG_SIZE 516 // that size is in bytes
#define MAX_UNSIGNED_SHORT 65535

using std::FILE;
using std::string;
using std::cout;
using std::endl;
using std::cerr;
using std::ifstream;
using std::stringstream;

/// single thread \ process implementation
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

    unsigned short socket_protocol = atoi(argv[1]);
    ///                                              SOCK_DGRAM = UDP
    int server_socket_fd = socket(AF_INET , SOCK_DGRAM, socket_protocol);

    /// infinite run loop
    while (true) {

        /// listen on UDP PORT

        /// WRQ request received, send an ack packet

        /// wait for packet

        /// send ack after each packet received

        /// end communication if got a data packet shorter than 516


    }


    return 0;
}

