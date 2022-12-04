// SERVER CODE

#include "sock.h"

// CLIENT_IP "172.24.0.20"

//FUNCTION DEFNS
void* send_data_thread(void*);
void* receive_data_thread(void*);
void* get_input_thread(void*);
// GLOBALS
int sd;
struct sockaddr_in c_addr;
char sender_buf[SENDER_BUF_SIZE];

// CONDS AND MUTEXES
pthread_cond_t send_wait = PTHREAD_COND_INITIALIZER;
pthread_mutex_t send_mutex = PTHREAD_MUTEX_INITIALIZER;

int main(int argc, char**argv)
{
    // argv[1] port num, recv data from here
    const int SERVER_PORT = atoi(argv[1]);
    sd = create_socket(SERVER_PORT);
    pthread_t send_thread;
    pthread_t receive_thread;
    pthread_t get_input_t;

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

        if(sendto(sd, sender_buf, 100, 0, (struct sockaddr *)&c_addr,
                    sizeof(c_addr)) == -1)
        {
            perror("sendto");
            exit(1);
        }
        pthread_mutex_unlock(&send_mutex);
    }
    return NULL;
}
void* receive_data_thread(void* args)
{
    // fprintf(stderr, "receive thread online\n");
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
        fprintf(stderr, "server got : %s", buf);
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

