#include "sock.h"

int create_socket(int port_number)
{
    struct sockaddr_in sain; memset(&sain, 0, sizeof(sain));
    int sd;
    if((sd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        perror("socket");
        exit(1);
    }

    sain.sin_family = AF_INET;
    sain.sin_port  = htons(port_number);
    sain.sin_addr.s_addr = htonl(INADDR_ANY);

    if(bind(sd, (struct sockaddr *)&sain, sizeof(sain)) < 0) {
        perror("bind");
        exit(1);
    }
    return sd;
}
