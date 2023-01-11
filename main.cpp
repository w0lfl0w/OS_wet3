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

    unsigned int port = atoi(argv[1]);
    unsigned int timeout = atoi(argv[2]);
    unsigned int max_num_of_resends = atoi(argv[3]);

    ////////////////////////////////////////// debuging ///////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    arguments_flag = (argc == 4) && (port <= MAX_UNSIGNED_SHORT) && (port >= 0) &&
                     (timeout <= MAX_UNSIGNED_SHORT) && (timeout >= 0) &&
                     (max_num_of_resends <= MAX_UNSIGNED_SHORT) && (max_num_of_resends >= 0);

    if (!arguments_flag) {
        perror("TTFPT_ERROR: illegal arguments");
        exit(0);
    }

    int server_socket_fd = socket(AF_INET, SOCK_DGRAM, 0); /// SOCK_DGRAM = UDP

    if (server_socket_fd < 0) {
        perror("TTFTP_ERROR");
        exit(0);
    }


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

