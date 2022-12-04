// SERVER CODE

#include "sock.h"

// CLIENT_IP "172.24.0.20"

void* send_data_thread(void*);
void* receive_data_thread(void*);

int sd;
struct sockaddr_in c_addr;

int main(int argc, char**argv)
{
    // argv[1] port num, recv data from here
    sd = create_socket(atoi(argv[1]));
    pthread_t send_thread;
    pthread_t receive_thread;

    // server 
    //addr *dest = malloc(sizeof(addr));
    //memset(dest, 0, sizeof(addr));
    //dest->port_number = C_PORT;

    pthread_create(&send_thread, NULL, send_data_thread, NULL);

    pthread_create(&receive_thread, NULL, receive_data_thread, NULL);
    
    pthread_join(send_thread, NULL);
    pthread_join(receive_thread, NULL);
    return 0;
}

void* send_data_thread(void* args)
{
    fprintf(stderr, "send thread online\n");

    while(true)
    {
        char buf[100]; memset(buf, 0, 100);
        scanf("%s", buf);
        if(sendto(sd, buf, 100, 0, (struct sockaddr *)&c_addr,
                    sizeof(c_addr)) == -1)
        {
            perror("sendto");
            exit(1);
        }
    }
    return NULL;
}
void* receive_data_thread(void* args)
{
    fprintf(stderr, "receive thread online\n");
    socklen_t addr_len = sizeof c_addr;
    while(true)
    {
        char buf[100]; memset(buf, 0, 100);
        if(recvfrom(sd, 
                    buf, 
                    100, 
                    0, 
                    (struct sockaddr *) &c_addr, &addr_len) == -1)
        {
            perror("recvfrom");
            exit(1);
        }
        fprintf(stderr, "server got : %s\n", buf);
    }

    return NULL;
}
