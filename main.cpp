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
#include <cstring>


#define MAX_DATA_SIZE       512 // that size is in bytes
#define MAX_SOCKET_MSG_SIZE 516 // that size is in bytes
#define MAX_UNSIGNED_SHORT 65535

#define ERRCODE_UNKNOWN     7
#define ERRCODE_FILEEXISTS  6
#define ERRCODE_UNEXPECTED  4
#define ERRCODE_BADBLOCK    0
#define ERRCODE_TIMEOUT     0
#define MSG_UNKNOWN         "Unknown user"
#define MSG_FILEEXISTS      "File already exists"
#define MSG_UNEXPECTED      "Unexpected packet"
#define MSG_BADBLOCK        "Bad block number"
#define MSG_TIMEOUT         "Abandoning file transmission"

#define OPCODE_WRQ          2
#define OPCODE_ACK          4
#define OPCODE_DATA         3
#define OPCODE_ERROR        5

using std::FILE;
using std::string;
using std::cout;
using std::endl;
using std::cerr;
using std::ifstream;
using std::stringstream;


class ErrorMsg {
public:
    short opcode;
    short error_code;
    char error_message[MAX_DATA_SIZE];
    //string_terminator;

    ErrorMsg() : opcode(-1), error_code(-1)/*, string_terminator("\0")*/
    {
        memset(this->error_message, '\0', MAX_DATA_SIZE);
    }

    ErrorMsg(short opcode, short error_code, string error_message) : opcode(htons(opcode)), error_code(htons(error_code))/*,
                                                                     error_message(""), string_terminator("\0")*/ {
        memset(this->error_message, '\0', MAX_DATA_SIZE);
        strcpy(this->error_message, error_message.c_str());
    }


    ErrorMsg(const ErrorMsg& orig)
    {
        this->opcode = orig.opcode;
        this->error_code = orig.error_code;
        memset(this->error_message, '\0', MAX_DATA_SIZE);
        strcpy(this->error_message, orig.error_message);
    }


} __attribute__((packed));


class WRQ {
public:
    short opcode;
    string filename;
    string trans_mode;

    WRQ() {}

    WRQ(char buff[], int buff_size) {
        this->opcode = OPCODE_WRQ;
        this->filename = (char *) &buff[2];
        this->trans_mode = (char *) &buff[3 + strlen((char *) &buff[2])];
    }

    ~WRQ() {}
}__attribute__((packed));


class ACK {
public:
    short opcode;
    short block_number;

    ACK() {}

    ACK(short block_num) {
        this->opcode = htons(OPCODE_ACK);
        this->block_number = htons(block_num);
    }

    ~ACK() {}
}__attribute__((packed));


class Data {
public:
    short opcode;
    short block_number;
    char data[MAX_DATA_SIZE + 1];

    Data() {}

    Data(short block_num, char *data_in, int buff_size) {
        this->opcode = OPCODE_DATA;
        this->block_number = block_num;
        strncpy(this->data, data_in, buff_size - 4);
        this->data[MAX_DATA_SIZE] = '\0';
    }

    Data(char buff[], int buff_size) {
        this->opcode = OPCODE_DATA;
        this->block_number = (short) buff[2] << sizeof(char) | (short) buff[3];
        strncpy(this->data, &buff[4], buff_size - 4);
        this->data[MAX_DATA_SIZE] = '\0';
    }

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
                        fp(nullptr), current_session_socket_fd(-1) {
        this->curr_client = {0};
    }


    SessionMannager(const sockaddr_in &new_client, int &new_socket_fd, string &filename) {
        this->is_active = false;
        this->expected_block_num = 0;
        this->resends_num = 0;
        this->curr_client = new_client;
        this->current_session_socket_fd = new_socket_fd;
        this->filename = filename;
        this->fp = fopen((this->filename).c_str(), "r");

    }


    SessionMannager(const SessionMannager &SM) {
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


    void close_session(bool finished_nice) {
        if (nullptr != this->fp) {
            fclose(this->fp);
            this->fp = nullptr;
        }
        if (!finished_nice) {
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

    FILE *try_open_new(const std::string &name) {
        FILE *file = NULL;
        if (file_exists(name)) {
            return nullptr;
        } else {
            file = fopen(name.c_str(), "w");
            this->filename = name;
            this->fp = file;
            return file;
        }
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
    short curr_op = 0;

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
    socklen_t curr_client_addr_len = sizeof(curr_client);
    socklen_t new_client_addr_len = sizeof(new_client);
    int max_sd, sd, activity, new_socket;
    int client_socket[SOMAXCONN];
    char buffer[1024];
    ErrorMsg current_error;
    ACK ack_msg;
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

    cout << "got till here" << endl;

    // init the array of sockets - named master
    fd_set master;
    FD_ZERO(&master);
    FD_SET(server_socket_listen_fd, &master);
    if (debug_flag) {
        printf("\nAdding listener to master \n");
    }

    /// infinite working loop
    while (1) {
        /// there is a live session
        if (session_manager.is_active) {
            cout << "active session\n";
            // use select with timeout
            // if received packet
            // if op != DATA_OP send error and kill session
            // if block num doenst match send error and kill session
            // check if
            // if reached timeout (didnt receive packet in time)
            // if didnt pass max resend
            // resend ACK
            // ++ resend counter
            // if passed max resend, send error and kill session
        }
            /// waiting for a session to start
        else {
            cout << "no session\n";
            //wait for an activity on one of the sockets
            //activity = select(max_sd + 1, &master, NULL, NULL, NULL);



            int recvfrom_return_val = recvfrom(server_socket_listen_fd, buffer, MAX_SOCKET_MSG_SIZE, MSG_WAITALL,
                                               (struct sockaddr *) &curr_client, &curr_client_addr_len);
            if (recvfrom_return_val < 0) {
                perror("TTFTP_ERROR");
                exit(0);
            }

            curr_op = (short) buffer[0] << sizeof(char) | (short) buffer[1];

            buffer[recvfrom_return_val] = '/0';

            if (debug_flag) {
                cout << "curr_client.sin_addr: " << curr_client.sin_addr.s_addr << endl;
                //<< endl;//<< (curr_client.sin_addr) << endl;
                cout << "recvfrom_return_val: " << recvfrom_return_val << endl;
                cout << "buffer: " << buffer << endl;
                cout << "op: " << curr_op << endl;
                cout << "file name: " << (char *) &buffer[2] << endl;
                cout << "transmission mode: " << (char *) &buffer[3 + strlen((char *) &buffer[2])] << endl;
            }
            // got packet, check if its a WRQ PACKET
            if (curr_op != OPCODE_WRQ) {
                cout << "got a non WRQ, send error\n";
                // send to
            } else {
                // if file already exist send error (use try_open_new)
                FILE *try_open_new_return_value = session_manager.try_open_new((char *) &buffer[2]);
                if (nullptr == try_open_new_return_value) {
                    // failed to create new file because it already exists
                    cout << "file already exists\n";
                    //current_error.error_code = ERRCODE_FILEEXISTS;
                    //current_error.error_message.assign(MSG_FILEEXISTS);
                    current_error = ErrorMsg(OPCODE_ERROR, ERRCODE_FILEEXISTS, MSG_FILEEXISTS);
                    curr_client_addr_len = sizeof(curr_client);
                    int bytes_sent = sendto(server_socket_listen_fd, &current_error, 4 + strlen(current_error.error_message) + 1, 0,
                                            (sockaddr *) &curr_client, curr_client_addr_len);
                    if (debug_flag)
                    {
                        cout << "bytes sent: " << bytes_sent << endl;
                    }
                    //sendto
                    //    short opcode;
                    // short error_code;
                    // string error_message;
                    // string string_terminator;

                } else {
                    cout << "file doesnt exists yet, open it and start a session.\n";
                    session_manager.is_active = true;
                    ack_msg.block_number = session_manager.expected_block_num;
                    ack_msg.opcode = OPCODE_ACK;
                    curr_client_addr_len = sizeof(curr_client);
                    sendto(server_socket_listen_fd, &ack_msg, 4, 0, (sockaddr *) &curr_client, curr_client_addr_len);

                }
            }
            // else - start session
            // open file, use try_open_new
            // if
            // if not send illegel command
        }

    }

    return 0;
}

/////////////////////////// good until here ////////////////////////////////////////////////////////////////////




// init the array of sockets - named master
//    fd_set master;
//    FD_ZERO(&master);
//    FD_SET(server_socket_listen_fd, &master);
//    if (debug_flag) {
//        printf("\nAdding listener to master \n");
//    }

//    /// infinite run loop
//    while (true) {
//        /// make the set ready - clear it and add the listening socket to it
//        FD_ZERO(&master);
//        FD_SET(server_socket_listen_fd, &master);
//        max_sd = server_socket_listen_fd;
//
//        /// add client sockets to set
//        for (int i = 0; i < SOMAXCONN; i++) {
//            //socket descriptor
//            sd = client_socket[i];
//
//            //if valid socket descriptor then add to read list
//            if (sd > 0)
//                FD_SET(sd, &master);
//
//            //highest file descriptor number, need it for the select function
//            max_sd = std::max(max_sd, sd);
//        }
//
//        //wait for an activity on one of the sockets
//        activity = select(max_sd + 1, &master, NULL, NULL, NULL);
//
//        if ((activity < 0) && (errno != EINTR)) {
//            perror("TTFTP_ERROR");
//            exit(0);
//        }
//
////        if (activity) {
////            cout << "got activity\n";
////        }
//
//        /// if something happend in the listening socket its an incoming connection
//        if (FD_ISSET(server_socket_listen_fd, &master)) {
//            /// if there is an ongoing sessions send the appropriate error message
//            if (session_manager.is_active) {
//                if (debug_flag) {
//                    cout << "got a new connection but we already have a session\n";
//                }
//                // send error message
//            }
//                /// start a session with this one
//            else {
//                if (debug_flag) {
//                    cout << "starting a new session\n";
//                }
//                session_manager.is_active = true;
//
//                new_socket = accept(server_socket_listen_fd,
//                                    (struct sockaddr *) &curr_client, (socklen_t *) &curr_client_addr_len);
//                if (new_socket < 0) {
//                    perror("TTFTP_ERROR");
//                    exit(0);
//                }
//
//                //session_manager.current_session_socket_fd = client_socket[i];
//                // start a session
//            }
//        }
//
//
//        for (int i = 0; i < SOMAXCONN; i++) {
//            ///found active socket
//            if (FD_ISSET(client_socket[i], &master)) {
//                if (debug_flag) {
//                    cout << "got new connection" << endl;
//                }
//                /// if something happend in the listening socket its an incoming connection
//                if (server_socket_listen_fd == client_socket[i]) {
//                    /// if there is an ongoing sessions send the appropriate error message
//                    if (session_manager.is_active) {
//                        // send error message
//                    }
//                        /// start a session with this one
//                    else {
//                        session_manager.is_active = true;
//                        session_manager.current_session_socket_fd = client_socket[i];
//                        // start a session
//                    }
//                }
//
//                    /// got something from client socket
//                else {
//                    /// find the active client
//                    if (FD_ISSET(client_socket[i], &master)) {
//                        /// check if the current socket is the one we are talking with
//                        if (session_manager.is_curr_client(client_socket[i])) {
//
//                        }
//                            /// if it isnt, send the correct error
//                        else {
//
//                        }
//                    }
//                }
//            }
//     }
//         /// WRQ request received, send an ack packet
//         /// run function for handling incoming messages

//         /// wait for packet

//         /// send ack after each packet received

//         /// end communication if got a data packet shorter than 516


//  }

// return 0;

//}//