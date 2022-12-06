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
char *input_buf;
sender_que sender_buf;

// before sender thread is signalled this variable is set
// 0 -> packets to be sent
// 1 -> timer went off
#define SEND_PACKET 0
#define TIMEOUT 1
char WAKE_STATUS;

// CONDS AND MUTEXES
pthread_cond_t send_wait = PTHREAD_COND_INITIALIZER;
pthread_mutex_t send_mutex = PTHREAD_MUTEX_INITIALIZER;

int main(int argc, char** argv)
{   
    memset(&sender_buf, 0, sizeof(sender_buf));

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
    
    sender_buf.base = 0;
    sender_buf.end = 0;
    sender_buf.last = -1;
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
        // sender waits on condition
        //     condition variable can be signalled in 2 ways
        //     1: timer thread's alarm wents off 
        //          send all packets in current window again
        //          restart timer threads alarm
        //     2: input thread sends signal 
        //          send untill base window size is reached
        switch (WAKE_STATUS){
            case SEND_PACKET:
                for(; !isWindowFull(sender_buf.base, sender_buf.end)
                        && sender_buf.end != sender_buf.last + 1
                        ; INCR(sender_buf.end))
                {
                    //TODO MAKE A WRAPPER FOR THESE FUNCTIONS
                    fprintf(stderr, "end:%d, %u sending %s \n", sender_buf.end,
                            sender_buf.que[sender_buf.end].seq_num,
                            sender_buf.que[sender_buf.end].data
                            );

                    if(sendto(  sd, 
                                (void*)&(sender_buf.que[sender_buf.end]), 
                                PAYLOAD_SIZE, 
                                0, 
                                (struct sockaddr *)&s_addr,
                                sizeof(s_addr)) == -1
                            ) { perror("sendto"); exit(1);}
                }
                break;
            case TIMEOUT:
                break;
        }
        pthread_mutex_unlock(&send_mutex);
    }
    return NULL;
}
void* receive_data_thread(void* args)
{
    // fprintf(stderr, "receive thread online\n");
    socklen_t addr_len;
    packet *p = malloc(sizeof(packet)); 
    while(true)
    {
        memset(p, 0, PAYLOAD_SIZE);
        if(recvfrom(sd,
                    p,
                    PAYLOAD_SIZE,
                    0,
                    (struct sockaddr *)&s_addr, &addr_len) == -1)
        {
            perror("recvfrom");
            exit(1);
        }
        // resolve package check ack if its in order
        if(p->type == TYPE_ACK)
        {
            // ACK PACKET RECEIVED
            pthread_mutex_lock(&send_mutex); 
            // dont know surely that  if locking is necessary
            // better safe than sorry
            fprintf(stderr, "base : %d, end: %d, base_seq %d, end_seq %d, p_seq %d\n",
                 sender_buf.base, sender_buf.end, 
                 sender_buf.que[sender_buf.base].seq_num,   
                 sender_buf.que[sender_buf.end].seq_num,
                 p->seq_num); 

            if (p->seq_num >= sender_buf.que[sender_buf.base].seq_num && 
                    p->seq_num <= sender_buf.que[sender_buf.end - 1].seq_num
            )
            {
                    // slide window up to received packets seq num
                    while(sender_buf.que[sender_buf.base].seq_num <= p->seq_num
                            && sender_buf.base != sender_buf.end)
                    {
                        fprintf(stderr, "acked %u\n",  
                                sender_buf.que[sender_buf.base].seq_num);
                        // delete acked packet
                         
                        //memset(&sender_buf.que[sender_buf.base], 0, sizeof(packet));
                        INCR(sender_buf.base);
                    }


                    //send a signal to timer thread to stop timer
                    WAKE_STATUS = SEND_PACKET;
                    pthread_cond_signal(&send_wait);
            }
            pthread_mutex_unlock(&send_mutex);
        }
        else
        {
            // DATA PACKET RECEIVED
            // check if expected sequence number == p.seq_num
            // send ack with p.seq_num 
            // else resend ack for expected seq num
            continue; // client only receives ack packacages for now  
        }
        //fprintf(stdout,"%s", input_buf);

    }

    return NULL;
}
void * get_input_thread(void * args)
{
    input_buf = malloc(1024 * sizeof(char));
    memset(input_buf, 0, 1024);
    uint32_t seq_num = 0;
    while(true)
    {
        memset(input_buf, 0, 1024);
        fgets(input_buf, 1024, stdin);
        int input_len = get_input_len(input_buf);

        pthread_mutex_lock(&send_mutex);
        form_and_que_packets(
                &sender_buf, 
                input_buf, 
                input_len,
                &seq_num
                    );
        pthread_mutex_unlock(&send_mutex);

        fprintf(stderr, "input_len: %d input_echo %s", input_len, input_buf);
        print_sender_q(&sender_buf);
        fprintf(stderr, "base %d, end %d, last %d\n",
               sender_buf.base, sender_buf.end, sender_buf.last);
        // signals the sender thread 
        //if(sender_buf.end - sender_buf.base + 1 != WINDOW_SIZE)
        WAKE_STATUS  = SEND_PACKET;
        pthread_cond_signal(&send_wait);  
    }

}
