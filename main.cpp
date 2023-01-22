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


bool debug_flag = false;

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

    ErrorMsg(short error_code, string error_message) : 
        opcode(htons(OPCODE_ERROR)), error_code(htons(error_code))/*,
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
        this->filename = (char*)&buff[2];
        this->trans_mode = (char*)&buff[3 + strlen((char*)&buff[2])];
    }

    ~WRQ() {}
    //}__attribute__((packed));
};


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

    Data(short block_num, char* data_in, int buff_size) {
        this->opcode = OPCODE_DATA;
        this->block_number = block_num;
        memset(this->data, '\0', MAX_DATA_SIZE + 1);
        memcpy(this->data, data_in, buff_size - 4);
    }

    Data(char buff[], int buff_size) {
        this->opcode = OPCODE_DATA;
        //this->block_number = (short)buff[2] << sizeof(char) | (short)buff[3];
        this->block_number = ntohs( *(short*)(buff + sizeof(short)) );
        memset(this->data, '\0', MAX_DATA_SIZE + 1);
        memcpy(this->data, &buff[4], buff_size - 2 * sizeof(short));
    }

    ~Data() {}
}__attribute__((packed));


bool file_exists(const std::string& name) {
    FILE* file = fopen(name.c_str(), "r");
    if (NULL != file) {
        if (fclose(file) != 0) 
        {
            perror("TTFTP_ERROR: fclose failed");
            exit(1);
        }
        return true;
    }
    else 
    {
        //perror("TTFTP_ERROR");
        return false;
    }
}


class SessionMannager {
public:

    bool is_active;
    bool error_occurred;
    short expected_block_num;
    unsigned int resends_num;
    struct sockaddr_in curr_client = { 0 };
    int current_session_socket_fd;
    string filename;
    FILE* fp;


    SessionMannager() : is_active(false), error_occurred(false), expected_block_num(0), resends_num(0),
        current_session_socket_fd(-1), filename(""), fp(nullptr) {
        this->curr_client = { 0 };
    }


    SessionMannager(const sockaddr_in& new_client, int& new_socket_fd, string& filename, FILE * fp) : 
        is_active(false), error_occurred(false), expected_block_num(0), resends_num(0) {
        //this->is_active = false;
        
        //this->expected_block_num = 0;
        //this->resends_num = 0;
        this->curr_client = new_client;
        this->current_session_socket_fd = new_socket_fd;
        this->filename = filename;

        this->fp = fp;

    }


    SessionMannager(const SessionMannager& SM) {
        this->is_active = SM.is_active;
        this->error_occurred = SM.error_occurred;
        this->expected_block_num = SM.expected_block_num;
        this->resends_num = SM.resends_num;
        this->curr_client = SM.curr_client;
        this->current_session_socket_fd = SM.current_session_socket_fd;
        this->filename = SM.filename;
        this->fp = NULL;
    }


    ~SessionMannager() 
    {
        if (this->is_active)
        {
            this->close_session(false);
        }
    }

    bool is_curr_client(sockaddr_in &addr/*, int sock_fd*/) {
        //cout << "in is_curr_client" << endl;
        if (this->is_active) {
            if (false)
            {
                cout << "received data packet from user " << addr.sin_addr.s_addr << ":" << htons(addr.sin_port) << endl;
                cout << "but expected packet from user " << this->curr_client.sin_addr.s_addr << ":" << htons(this->curr_client.sin_port) << endl;
            }
            return (ntohs(addr.sin_addr.s_addr) == ntohs(this->curr_client.sin_addr.s_addr)) &&
                   (ntohs(addr.sin_port) == ntohs(this->curr_client.sin_port));
            //return this->current_session_socket_fd == sock_fd;
        }

        this->curr_client = addr;
        //this->current_session_socket_fd = sock_fd;
        return true;
    }


    void close_session(bool critical_error) {
        if (NULL != this->fp) 
        {
            if (fclose(this->fp) != 0)
            {
                perror("TTFTP_ERROR: fclose failed");
                exit(1);
            }
            this->fp = NULL;

            if (critical_error) // if some critical error occurred, delete the file.
            {
                unlink((this->filename).c_str());
            }
        }
        this->reset_session();
        return;
    }


    void reset_session() {
        this->is_active = false;
        this->error_occurred = false;
        this->expected_block_num = 0;
        this->resends_num = 0;
        this->curr_client = { 0 };
        this->current_session_socket_fd = -1;
        this->filename = "";
        if (NULL != this->fp)
        {
            if(fclose(this->fp) != 0)
            {
                perror("TTFTP_ERROR: fclose failed");
                exit(1);
            }
            this->fp = NULL;
        }
        return;
    }
    /// <summary>
    /// tries to opan a new file for writing
    /// </summary>
    /// <param name="name"></param>
    /// <returns>returns pointer to the new file if succeded, returns NULL if file already exists. exits if fopen failed to open new file.</returns>
    FILE* try_open_new(const std::string& name) {
        FILE* file = NULL;
        if (file_exists(name)) {
            return nullptr;
        }
        else {
            file = fopen(name.c_str(), "w");
            if (NULL == file)
            {
                perror("TTFTP_ERROR: fopen failed");
                exit(1);
            }
            this->filename = name;
            // free prev file pointed by fp before putting the new file to fp.
            if (nullptr != this->fp)
            {
                if (fclose(this->fp) != 0)
                {
                    perror("TTFTP_ERROR: fclose failed");
                    exit(1);
                }
                this->fp = NULL;
            }
            this->fp = file;
            return file;
        }
    }
};


/// single thread \ process implementation
int main(int argc, char** argv) {
    /// checkig for valid arguments - the command line is in the for of ./ttftps <port> <timeout> <max_num_of_resends>
    bool arguments_flag = true;
    bool according_to_lior = true; 
    // lior says here https://moodle2223.technion.ac.il/mod/forum/discuss.php?d=17992 something different 
    // from here https://moodle2223.technion.ac.il/mod/resource/view.php?id=13262
    // set to false to get results as requested in original PDF.

    

    ////////////////////////////////////////// debuging ///////////////////////////////////////////////////////////////
    //bool debug_flag = true;
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    arguments_flag = (argc == 4);

    if (!arguments_flag) {
        cerr << "TTFTP_ERROR: illegal arguments" << endl;
        exit(1);
    }

    unsigned int port = atoi(argv[1]);
    unsigned int timeout = atoi(argv[2]);
    unsigned int max_num_of_resends = atoi(argv[3]);
    SessionMannager session_manager;
    short curr_op = 0;

    arguments_flag = (port <= MAX_UNSIGNED_SHORT) && (port >= 0) &&
        (timeout <= MAX_UNSIGNED_SHORT) && (timeout >= 0) &&
        (max_num_of_resends <= MAX_UNSIGNED_SHORT) && (max_num_of_resends >= 0);
    /// init a listening socket
    if (!arguments_flag) {
        cerr << "TTFTP_ERROR: illegal arguments" << endl;
        exit(1);
    }

    struct timeval tv = {0}; // timeout of <timeout> seconds and 0 ms
    tv.tv_sec = timeout;
    
    int server_socket_listen_fd = socket(AF_INET, SOCK_DGRAM, 0); /// SOCK_DGRAM = UDP

    if (server_socket_listen_fd < 0) {
        perror("TTFTP_ERROR: socket failed");
        exit(1);
    }

    

    /// optional stronger init to add after calling socket(), add if getting errors like “address already in use”
    //int setsockopt(int server_socket_listen_fd, int level, int optname,  const void *optval, socklen_t optlen);

    /// init the environment for the sockaddr struct
    struct sockaddr_in server_address = { 0 };
    struct sockaddr_in curr_client = { 0 };
    //struct sockaddr_in new_client = { 0 };
    socklen_t curr_client_addr_len = sizeof(curr_client);
    //socklen_t new_client_addr_len = sizeof(new_client);
    //int max_sd, sd, activity, new_socket;
    int activity = 0;
    int recvfrom_return_val = 0;
    //int client_socket[SOMAXCONN];
    char buffer[MAX_SOCKET_MSG_SIZE + 1];
    ErrorMsg current_error;
    ACK ack_msg;
    Data curr_data;
    for (int i = 0; i < SOMAXCONN; i++) { /// SOMAXCONN = max_clients
        //client_socket[i] = 0;
    }
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port);
    server_address.sin_addr.s_addr = INADDR_ANY;

    /// assigns the address and port to the socket
    int bind_return_value = bind(server_socket_listen_fd, (struct sockaddr*)&server_address, sizeof(server_address));


    if (bind_return_value < 0) {
        perror("TTFTP_ERROR: bind failed");
        exit(1);
    }

    if (debug_flag) {
        cout << "got till here" << endl;
    }

    // init the array of sockets - named master
    fd_set master;
    FD_ZERO(&master);
    FD_SET(server_socket_listen_fd, &master);
    if (debug_flag) {
        printf("\nAdding listener to master \n");
    }

    /// infinite working loop
    while (1) {
        //sleep(1);
        memset(buffer, '\0', MAX_SOCKET_MSG_SIZE);
        FD_ZERO(&master);
        FD_SET(server_socket_listen_fd, &master);
        tv.tv_sec = timeout; // reset tv to original count because select ruined it

        /// there is a live session
        if (session_manager.is_active) {
            if (debug_flag)
            {
                cout << "active session\n";
            }

            
            // use select with timeout
            activity = select(server_socket_listen_fd + 1, &master, NULL, NULL, &tv);
            
            if (-1 == activity) // if select failed
            {
                perror("TTFTP_ERROR: select failed");
                exit(1);
            }
            
            else if (activity) // if received packet
            {
                recvfrom_return_val = recvfrom(server_socket_listen_fd, buffer, MAX_SOCKET_MSG_SIZE, 0,
                    (struct sockaddr*)&curr_client, &curr_client_addr_len);
                if (recvfrom_return_val < 0) {
                    perror("TTFTP_ERROR: recvfrom failed");
                    exit(1);
                }
                // get opcode from packet
                //curr_op = (short)buffer[0] << sizeof(char) | (short)buffer[1];
                curr_op = ntohs(*(short*)(buffer));

                // TODO: maybe should switch order between OPCODE_WRQ != curr_op and OPCODE_WRQ == curr_op.
                if (OPCODE_WRQ != curr_op && !session_manager.is_curr_client(curr_client)) // if client that is not in curr session tries to send data
                {
                    current_error = ErrorMsg(ERRCODE_UNKNOWN, MSG_UNKNOWN); // generate error msg for unknown user
                    if (according_to_lior) // send error and dont kill original session
                    {
                        curr_client_addr_len = sizeof(curr_client);
                        int bytes_sent = sendto(server_socket_listen_fd, &current_error, 4 + strlen(current_error.error_message) + 1, 0,
                            (sockaddr*)&curr_client, curr_client_addr_len);
                        if (0 > bytes_sent)
                        {
                            perror("TTFTP_ERROR: sendto failed");
                            exit(1);
                        }
                    }
                    else // according to PDF - send error to both clients and kill orig session.
                    {
                        session_manager.error_occurred = true;
                    }
                    if (debug_flag)
                    {
                        cout << "curr_op is: " << curr_op << endl;
                        cout << "received data packet from user " << curr_client.sin_addr.s_addr << ":" << htons(curr_client.sin_port) << endl;
                        cout << "but expected packet from user " << session_manager.curr_client.sin_addr.s_addr << ":" << htons(session_manager.curr_client.sin_port) << endl;
                    }
                }
                else if (OPCODE_WRQ == curr_op && !session_manager.is_curr_client(curr_client)) // if another client tries to start a session
                {
                    current_error = ErrorMsg(ERRCODE_UNEXPECTED, MSG_UNEXPECTED);
                    if (according_to_lior) // send error and dont kill original session
                    {
                        curr_client_addr_len = sizeof(curr_client);
                        int bytes_sent = sendto(server_socket_listen_fd, &current_error, 4 + strlen(current_error.error_message) + 1, 0,
                            (sockaddr*)&curr_client, curr_client_addr_len);
                        if (0 > bytes_sent)
                        {
                            perror("TTFTP_ERROR: sendto failed");
                            exit(1);
                        }
                    }
                    else // according to PDF - send error to both clients and kill orig session.
                    {
                        session_manager.error_occurred = true;
                    }
                }
                // if op != DATA_OP, send error and kill session
                else if (OPCODE_DATA != curr_op) 
                {
                    
                    if (debug_flag)
                    {
                        cout << "received unexpected packet. closing session " << endl;
                    }
                    current_error = ErrorMsg(ERRCODE_UNEXPECTED, MSG_UNEXPECTED); // generate error msg for out of oreder packet
                    session_manager.error_occurred = true; // will send the packet at the end of the loop

                }
                else if (OPCODE_DATA == curr_op) // received data packet
                {
                    curr_data = Data(buffer, recvfrom_return_val);
                    if (session_manager.expected_block_num == curr_data.block_number) // if block num is good
                    {
                        
                        // write data to file!
                        size_t wrote = 0;
                        if ((wrote = fwrite(curr_data.data, sizeof(char), recvfrom_return_val - 2 * sizeof(short), session_manager.fp)))
                        {
                            if (debug_flag)
                            {
                                cout << "wrote " << wrote << " bytes of data to the file" << endl;
                            }
                        }
                        if (wrote != (recvfrom_return_val - 2 * sizeof(short))) // fwrite failed! according to fwrite man, if retval and size given are different, fwrite failed.
                        {
                            perror("TTFTP_ERROR: fwrite failed");
                            exit(1);
                        }
                        if (debug_flag)
                        {
                            cout << "writing data to file" << endl;
                        }
                        // send ACK for data
                        ack_msg = ACK(session_manager.expected_block_num);
                        curr_client_addr_len = sizeof(curr_client);
                        int bytes_sent = sendto(server_socket_listen_fd, &ack_msg, 4, 0,
                            (sockaddr*)&curr_client, curr_client_addr_len);
                        if (0 > bytes_sent)
                        {
                            perror("TTFTP_ERROR: sendto failed");
                            exit(1);
                        }
                        if (debug_flag)
                        {
                            cout << "sending ACK number " << session_manager.expected_block_num << endl;
                            cout << "bytes sent: " << bytes_sent << endl;
                        }
                        // if reached end of session 
                        if (recvfrom_return_val < MAX_SOCKET_MSG_SIZE)
                            // if data size is not MAX it means this is the last data packet, so end session.
                            // if file size % MAX_DATA_SIZE == 0 client will send empty data packet to finish session.
                        {
                            if (debug_flag)
                            {
                                cout << "ending session!" << endl;
                            }
                            session_manager.close_session(false);
                        }
                        else // if session didnt end yuet, count the blocks arrived
                        {
                            session_manager.expected_block_num++;
                        }
                    }
                    else // if block num doenst match send error and kill session
                    {
                        current_error = ErrorMsg(ERRCODE_BADBLOCK, MSG_BADBLOCK); // generate error msg for bad block
                        session_manager.error_occurred = true; // will send the packet at the end of the loop

                        if (debug_flag) // tested by intentionally miscalculating the expected_block_num member on server side and getting correct from client.
                        {
                            cout << "received bad block. expected blok num " << session_manager.expected_block_num << "but received " << curr_data.block_number << endl;
                        }
                    }
                }
            }
            else // if reached timeout (didnt receive packet in time)
            {
                
                if (max_num_of_resends > session_manager.resends_num) // didnt reach maximux resends
                {
                    //resend Ack! dont ++ block counter.
                    ack_msg = ACK(session_manager.expected_block_num - 1);
                    curr_client_addr_len = sizeof(curr_client);
                    int bytes_sent = sendto(server_socket_listen_fd, &ack_msg, 4, 0,
                        (sockaddr*)&curr_client, curr_client_addr_len);
                    if (0 > bytes_sent)
                    {
                        perror("TTFTP_ERROR: sendto failed");
                        exit(1);
                    }
                    if (debug_flag)
                    {
                        cout << "resending ACK number " << session_manager.expected_block_num -1  << endl;
                        cout << "bytes sent: " << bytes_sent << endl;
                    }
                    session_manager.resends_num++;
                }
                else // reached max resends
                {
                    current_error = ErrorMsg(ERRCODE_TIMEOUT, MSG_TIMEOUT); // generate error msg for timeout
                    session_manager.error_occurred = true; // will send the packet at the end of the loop
                }
            }
        }
        /// waiting for a session to start
        else {
            if (debug_flag)
            {
                cout << "no session\n";
            }
            //wait for an activity on one of the sockets

            recvfrom_return_val = recvfrom(server_socket_listen_fd, buffer, MAX_SOCKET_MSG_SIZE, MSG_WAITALL,
                (struct sockaddr*)&curr_client, &curr_client_addr_len);
            if (recvfrom_return_val < 0) {
                perror("TTFTP_ERROR: recvfrom failed");
                exit(1);
            }

            //curr_op = (short)buffer[0] << sizeof(char) | (short)buffer[1];
            curr_op = ntohs(*(short*)(buffer));


            if (debug_flag) {
                cout << "curr_client.sin_addr: " << curr_client.sin_addr.s_addr << endl;
                //<< endl;//<< (curr_client.sin_addr) << endl;
                cout << "recvfrom_return_val: " << recvfrom_return_val << endl;
                cout << "buffer: " << buffer << endl;
                cout << "op: " << curr_op << endl;
                cout << "file name: " << (char*)&buffer[2] << endl;
                cout << "transmission mode: " << (char*)&buffer[3 + strlen((char*)&buffer[2])] << endl;
            }
            // got packet, check if its a WRQ PACKET
            if (OPCODE_WRQ != curr_op) 
            {
                if (debug_flag)
                {
                    cout << "got a non WRQ, send error\n";
                }
                current_error = ErrorMsg(ERRCODE_UNKNOWN, MSG_UNKNOWN); // generate error msg for unknown user
                session_manager.error_occurred = true; // will send the packet at the end of the loop
                
            }
            else {
                // if file already exist send error (use try_open_new)
                FILE* try_open_new_return_value = session_manager.try_open_new((char*)&buffer[2]);
                if (nullptr == try_open_new_return_value) {
                    // failed to create new file because it already exists
                    if (debug_flag)
                    {
                        cout << "file already exists\n";
                    }
                    
                    current_error = ErrorMsg(ERRCODE_FILEEXISTS, MSG_FILEEXISTS);
                    curr_client_addr_len = sizeof(curr_client);
                    int bytes_sent = sendto(server_socket_listen_fd, &current_error, 4 + strlen(current_error.error_message) + 1, 0,
                        (sockaddr*)&curr_client, curr_client_addr_len);
                    if (0 > bytes_sent)
                    {
                        perror("TTFTP_ERROR: sendto failed");
                        exit(1);
                    }
                    if (debug_flag)
                    {
                        cout << "bytes sent: " << bytes_sent << endl;
                    }
                }
                else { // the file doesnt exist, we can continue to start new session
                    if (debug_flag)
                    {
                        cout << "file doesnt exists yet, open it and start a session.\n";
                    }
                    session_manager = SessionMannager(curr_client, server_socket_listen_fd, session_manager.filename, session_manager.fp);
                    session_manager.is_active = true;
                    //ack_msg.block_number = session_manager.expected_block_num;
                    //ack_msg.opcode = OPCODE_ACK;
                    ack_msg = ACK(session_manager.expected_block_num);
                    
                    curr_client_addr_len = sizeof(curr_client);
                    int bytes_sent = sendto(server_socket_listen_fd, &ack_msg, 4, 0, (sockaddr*)&curr_client, curr_client_addr_len);
                    if (0 > bytes_sent)
                    {
                        perror("TTFTP_ERROR: sendto failed");
                        exit(1);
                    }
                    session_manager.expected_block_num++; // ++ after sent first ACK
                }
            }
        }

        if (session_manager.error_occurred)
        {
            curr_client_addr_len = sizeof(curr_client);
            int bytes_sent = sendto(server_socket_listen_fd, &current_error, 4 + strlen(current_error.error_message) + 1, 0,
                (sockaddr*)&curr_client, curr_client_addr_len);
            if (0 > bytes_sent)
            {
                perror("TTFTP_ERROR: sendto failed");
                exit(1);
            }
            if (debug_flag)
            {
                cout << "sending error message: " << current_error.error_message << " to client " << curr_client.sin_addr.s_addr << ":" << htons(curr_client.sin_port) << endl;
                cout << "bytes sent: " << bytes_sent << endl;
            }
            session_manager.close_session(session_manager.error_occurred); // close session and delete the file that was created.
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