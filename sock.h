#ifndef __SOCKH
#define __SOCKH

//included libraries and common functions are defined in this file
#include <sys/socket.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <pthread.h>

// CONSTANTS
#define true 1
#define false 0 
#define C_PORT 2002
#define PAYLOAD_SIZE 16
#define QUE_SIZE 2048
#define WINDOW_SIZE 10

#define SENDER_BUF_SIZE PAYLOAD_SIZE * 32
// TYPEDEFS
typedef struct addr{
    int port_number;
    char ip[INET_ADDRSTRLEN];
} addr;

typedef struct packet{
    uint16_t seq_num;
    char data[14];
} packet;

typedef struct sender_que{
    packet que[QUE_SIZE];
    int base;
    int end;
    // end - base + 1 must be equal to WINDOW_SIZE 
}sender_que; 

// FUNCTION DECLS
int create_socket(int port_number);


#endif


