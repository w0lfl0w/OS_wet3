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
#include <sys/types.h>
//#include <netinet.h>
#include <netinet/in.h>
#include <netdb.h>



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

    /// init a socket
    int server_socket_fd = socket(AF_INET, SOCK_DGRAM, 0); /// SOCK_DGRAM = UDP

    if (server_socket_fd < 0) {
        perror("TTFTP_ERROR");
        exit(0);
    }

    /// optional stronger init to add after calling socket(), add if getting errors like “address already in use”
    //int setsockopt(int server_socket_fd, int level, int optname,  const void *optval, socklen_t optlen);

    /// init the environment for the sockaddr struct
    struct sockaddr_in server_address = {0};

    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port);
    server_address.sin_addr.s_addr = INADDR_ANY;

    /// assigns the address and port to the socket
    int bind_return_value = bind(server_socket_fd, (struct sockaddr *)&server_address, sizeof(server_address));

    if (bind_return_value < 0) {
        perror("TTFTP_ERROR");
        exit(0);
    }

    /// infinite run loop
    while (true) {

        /// listen on UDP PORT
        listen(server_socket_fd,5);
        /// WRQ request received, send an ack packet

        /// wait for packet

        /// send ack after each packet received

        /// end communication if got a data packet shorter than 516


    }


    return 0;
}

