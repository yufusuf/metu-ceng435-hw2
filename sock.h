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

#define true 1
#define false 1
#define C_PORT 2002

int create_socket(int port_number);
typedef struct addr{
    int port_number;
    char ip[INET_ADDRSTRLEN];
} addr;
#endif


