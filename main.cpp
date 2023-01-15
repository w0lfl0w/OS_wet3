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
#include <sys/select.h>
#include <unistd.h>

#include <sys/types.h>
//#include <netinet.h>
#include <netinet/in.h>
#include <netdb.h>


#define MAX_DATA_SIZE        512 // that size is in bytes
#define MAX_SOCKET_MSG_SIZE 516 // that size is in bytes
#define MAX_UNSIGNED_SHORT 65535

using std::FILE;
using std::string;
using std::cout;
using std::endl;
using std::cerr;
using std::ifstream;
using std::stringstream;



class ErrorMsg {
    short opcode;
    short error_code;
    string error_message;
    string string_terminator;

    ErrorMsg() : opcode(-1), error_code(-1), error_message(""), string_terminator("\0") {}
} __attribute__((packed));


class WRQ {
public:
    short opcode;
    string filename;
    string trans_mode;

    WRQ() {}

    ~WRQ() {}
}__attribute__((packed));


class ACK {
public:
    short opcode;
    short block_number;

    ACK() {}

    ~ACK() {}
}__attribute__((packed));


class Data {
public:
    short opcode;
    short block_number;
    char data[MAX_DATA_SIZE];

    Data() {}

    ~Data() {}
}__attribute__((packed));


bool file_exists(const std::string &name) {
    if (FILE *file = fopen(name.c_str(), "r")) {
        fclose(file);
        return true;
    } else {
        return false;
    }
}


class SessionMannager {
public:

    bool is_active;
    short expected_block_num;
    int resends_num;
    struct sockaddr_in curr_client = {0};
    int current_session_socket_fd;
    string filename;
    FILE *fp;


    SessionMannager() : is_active(false), expected_block_num(0), resends_num(0),
                        fp(nullptr), current_session_socket_fd(-1)
    {
        this->curr_client = {0};
    }


    SessionMannager(const sockaddr_in & new_client, int & new_socket_fd, string & filename)
    {
        this->is_active = false;
        this->expected_block_num = 0;
        this->resends_num = 0;
        this->curr_client = new_client;
        this->current_session_socket_fd = new_socket_fd;
        this->filename = filename;
        this->fp = fopen((this->filename).c_str(), "r");

    }


    SessionMannager(const SessionMannager& SM)
    {
        this->is_active = SM.is_active;
        this->expected_block_num = SM.expected_block_num;
        this->resends_num = SM.resends_num;
        this->curr_client = SM.curr_client;
        this->current_session_socket_fd = SM.current_session_socket_fd;
        this->filename = SM.filename;
        this->fp = fopen((this->filename).c_str(), "r");
    }


    ~SessionMannager() {}

    bool is_curr_client(/*sockaddr_in &addr,*/ int sock_fd) {
        if (this->is_active) {
            //return (addr.sin_addr.s_addr == this->curr_client.sin_addr.s_addr) &&
            //       (addr.sin_port == this->curr_client.sin_port);
            return this->current_session_socket_fd == sock_fd;
        }

        //this->curr_client = addr;
        //this->current_session_socket_fd = sock_fd;
        return true;
    }



    void close_session(bool finished_nice)
    {
        if (nullptr != this->fp)
        {
            fclose(this->fp);
            this->fp = nullptr;
        }
        if (!finished_nice)
        {
            unlink((this->filename).c_str());
        }
        this->reset_session();
        return;
    }


    void reset_session() {
        this->is_active = false;
        this->expected_block_num = 0;
        this->resends_num = 0;
        this->current_session_socket_fd = -1;
//        if (this->fp.isopen())
        return;
    }

    void handle_connection_request() {

    }

};


/// single thread \ process implementation
int main(int argc, char **argv) {
    /// checkig for valid arguments - the command line is in the for of ./ttftps <port> <timeout> <max_num_of_resends>
    bool arguments_flag = true;

    unsigned int port = atoi(argv[1]);
    unsigned int timeout = atoi(argv[2]);
    unsigned int max_num_of_resends = atoi(argv[3]);
    SessionMannager session_manager;

    ////////////////////////////////////////// debuging ///////////////////////////////////////////////////////////////
    bool debug_flag = true;
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    arguments_flag = (argc == 4) && (port <= MAX_UNSIGNED_SHORT) && (port >= 0) &&
                     (timeout <= MAX_UNSIGNED_SHORT) && (timeout >= 0) &&
                     (max_num_of_resends <= MAX_UNSIGNED_SHORT) && (max_num_of_resends >= 0);

    if (!arguments_flag) {
        perror("TTFPT_ERROR: illegal arguments");
        exit(0);
    }

    /// init a listening socket
    int server_socket_listen_fd = socket(AF_INET, SOCK_DGRAM, 0); /// SOCK_DGRAM = UDP

    if (server_socket_listen_fd < 0) {
        perror("TTFTP_ERROR");
        exit(0);
    }

    /// optional stronger init to add after calling socket(), add if getting errors like “address already in use”
    //int setsockopt(int server_socket_listen_fd, int level, int optname,  const void *optval, socklen_t optlen);

    /// init the environment for the sockaddr struct
    struct sockaddr_in server_address = {0};
    struct sockaddr_in curr_client = {0};
    struct sockaddr_in new_client = {0};
    int curr_client_addr_len = 0;
    int new_client_addr_len = 0;
    int max_sd, sd, activity;
    int client_socket[SOMAXCONN];
    for (int i = 0; i < SOMAXCONN; i++) { /// SOMAXCONN = max_clients
        client_socket[i] = 0;
    }
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port);
    server_address.sin_addr.s_addr = INADDR_ANY;

    /// assigns the address and port to the socket
    int bind_return_value = bind(server_socket_listen_fd, (struct sockaddr *) &server_address, sizeof(server_address));


    if (bind_return_value < 0) {
        perror("TTFTP_ERROR");
        exit(0);
    }

    /// listen on UDP PORT
    if (debug_flag) {
        printf("\nListening on port %d \n", port);
    }
    listen(server_socket_listen_fd, SOMAXCONN);

    // init the array of sockets - named master
    fd_set master;
    FD_ZERO(&master);
    FD_SET(server_socket_listen_fd, &master);
    if (debug_flag) {
        printf("\nAdding listener to master \n");
    }

    /// infinite run loop
    while (true) {
        /// make the set ready - clear it and add the listening socket to it
        FD_ZERO(&master);
        FD_SET(server_socket_listen_fd, &master);
        max_sd = server_socket_listen_fd;

        /// add client sockets to set
        for (int i = 0; i < SOMAXCONN; i++) {
            //socket descriptor
            sd = client_socket[i];

            //if valid socket descriptor then add to read list
            if (sd > 0)
                FD_SET(sd, &master);

            //highest file descriptor number, need it for the select function
            max_sd = std::max(max_sd, sd);
        }

        //wait for an activity on one of the sockets
        activity = select(max_sd + 1, &master, NULL, NULL, NULL);

        if ((activity < 0) && (errno != EINTR)) {
            perror("TTFTP_ERROR");
            exit(0);
        }

        for (int i = 0; i < SOMAXCONN; i++) {
            ///found active socket
            if (FD_ISSET(client_socket[i], &master)) {
                if (debug_flag){
                    cout << "got new connection" << endl;
                }
                /// if something happend in the listening socket its an incoming connection
                if (server_socket_listen_fd == client_socket[i]) {
                    /// if there is an ongoing sessions send the appropriate error message
                    if (session_manager.is_active) {
                        // send error message
                    }
                        /// start a session with this one
                    else {
                        session_manager.is_active = true;
                        session_manager.current_session_socket_fd = client_socket[i];
                        // start a session
                    }
                }

                    /// got something from client socket
                else {
                    for (int i = 0; i < SOMAXCONN; i++) {
                        /// find the active client
                        if (FD_ISSET(client_socket[i], &master)) {
                            /// check if the current socket is the one we are talking with
                            if (session_manager.is_curr_client(client_socket[i])) {

                            }
                                /// if it isnt, send the correct error
                            else {

                            }
                        }
                    }
                }
            }
//         /// WRQ request received, send an ack packet
//         /// run function for handling incoming messages

//         /// wait for packet

//         /// send ack after each packet received

//         /// end communication if got a data packet shorter than 516


        }
    }

}