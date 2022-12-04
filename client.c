// CLIENT CODE

#include "sock.h"


// send data to this ip
// SERVER_IP "172.24.0.10"
int sd;
struct sockaddr_in s_addr;

void* send_data_thread(void*);
void* receive_data_thread(void*);

int main(int argc, char** argv)
{   
    // client uses a random port 
    // in this case its defined in sock.h as 2002
    
    sd = create_socket(C_PORT);
    
    pthread_t send_thread;
    pthread_t receive_thread;

    // client sends to server ip and address

    s_addr.sin_family = AF_INET;
    s_addr.sin_port = htons(atoi(argv[2]));
    s_addr.sin_addr.s_addr = inet_addr(argv[1]);
    
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
        if(sendto(sd, buf, 100, 0, (struct sockaddr *)&s_addr,
                    sizeof(s_addr)) == -1)
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
    socklen_t addr_len;
    while(true)
    {
        char buf[100]; memset(buf,0,100);
        if(recvfrom(sd,
                    buf,
                    100,
                    0,
                    (struct sockaddr *)&s_addr, &addr_len) == -1)
        {
            perror("recvfrom");
            exit(1);
        }
        fprintf(stderr,"client got: %s\n", buf);

    }

    return NULL;
}
