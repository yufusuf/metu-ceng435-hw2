// CLIENT CODE

#include "sock.h"


// send data to this ip
// SERVER_IP "172.24.0.10"

// FUNCTION DEFNS
void* send_data_thread(void*);
void* receive_data_thread(void*);
void* get_input_thread(void*);

// GLOBALS
int sd;
struct sockaddr_in s_addr;
char sender_buf[SENDER_BUF_SIZE];

// CONDS AND MUTEXES
pthread_cond_t send_wait = PTHREAD_COND_INITIALIZER;
pthread_mutex_t send_mutex = PTHREAD_MUTEX_INITIALIZER;

int main(int argc, char** argv)
{   
    // client uses a random port 
    // in this case its defined in sock.h as 2002
    sd = create_socket(C_PORT);


    const int SERVER_PORT = atoi(argv[2]);
    char SERVER_IP[INET_ADDRSTRLEN];
    strncpy(SERVER_IP, argv[1], INET_ADDRSTRLEN); 

    
    pthread_t send_thread;
    pthread_t receive_thread;
    pthread_t get_input_t;

    // client sends to server ip and address

    s_addr.sin_family = AF_INET;
    s_addr.sin_port = htons(SERVER_PORT);
    s_addr.sin_addr.s_addr = inet_addr(SERVER_IP);
    
    pthread_create(&send_thread, NULL, send_data_thread, NULL);

    pthread_create(&receive_thread, NULL, receive_data_thread, NULL);

    pthread_create(&get_input_t, NULL, get_input_thread, NULL);


    pthread_join(send_thread, NULL);
    pthread_join(receive_thread, NULL);
    pthread_join(get_input_t, NULL);

    return 0;
}

void* send_data_thread(void* args)
{
    // fprintf(stderr, "send thread online\n");

    while(true)
    {
        // sender waits to be signalled to send packet
        pthread_mutex_lock(&send_mutex);
        pthread_cond_wait(&send_wait, &send_mutex);
        // sender waits on timed condition
        //     condition variable can be signalled in 2 ways
        //     1: timer finishes
        //          send all packets in current window again
        //          restart timer
        //     2: input comes 
        //          packetize the input 
        //          and add them to the que
        //     3: waken up to restart the timer
        //          correct ack received from receiver thread
        //          increment base pointer
        if(sendto(sd, sender_buf, 100, 0, (struct sockaddr *)&s_addr,
                    sizeof(s_addr)) == -1)
        {
            perror("sendto");
            exit(1);
        }
        memset(sender_buf, 0, SENDER_BUF_SIZE);
        pthread_mutex_unlock(&send_mutex);
    }
    return NULL;
}
void* receive_data_thread(void* args)
{
    // fprintf(stderr, "receive thread online\n");
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
        fprintf(stderr,"client got: %s", buf);

    }

    return NULL;
}
void * get_input_thread(void * args)
{
   while(true)
   {
        fgets(sender_buf, SENDER_BUF_SIZE, stdin);
        // signals the sender thread 
        pthread_cond_signal(&send_wait);  
   }

}
