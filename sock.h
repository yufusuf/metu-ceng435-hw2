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
#include <limits.h>
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
    uint32_t seq_num;
    uint16_t type;
    // 0 for acl 1 for data made 2 bytes so that packet struct is 16 bytes 
    char data[10];
} packet;

typedef struct sender_que{
    packet que[QUE_SIZE];
    char isACKed[QUE_SIZE];
    int base;
    int end;
    int last; // pointer to last added packet
    // end - base + 1 must be smaller than to WINDOW_SIZE 
}sender_que; 

// FUNCTION DECLS
int create_socket(int port_number);
void form_and_que_packets(sender_que *q, char * inp, int size, uint32_t *seq_num);
int get_input_len(char * inp_buf);
void print_sender_q(sender_que * q);
char isWindowFull(int base, int end);
#endif


