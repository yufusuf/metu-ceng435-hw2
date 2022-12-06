// CLIENT CODE

#include "sock.h"

// while i was implementing i was influenced by the psuedocode given in this link:
// https://www.tutorialspoint.com/a-protocol-using-go-back-n

// FUNCTION DEFNS
void* send_data_thread(void*);
void* receive_data_thread(void*);
void* get_input_thread(void*);
void* timer_thread(void*);

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
pthread_cond_t alarm = PTHREAD_COND_INITIALIZER;
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
    pthread_t timer_t;
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

    pthread_create(&timer_t, NULL, timer_thread, NULL);

    pthread_join(send_thread, NULL);
    pthread_join(receive_thread, NULL);
    pthread_join(get_input_t, NULL);
    pthread_join(timer_t, NULL);
    
    return 0;
}
void * timer_thread(void*args)
{
    
    pthread_mutex_lock(&send_mutex);
    pthread_cond_wait(&send_wait, &send_mutex);
    pthread_mutex_unlock(&send_mutex);
    while(true)
    {
        pthread_mutex_lock(&send_mutex);
        struct timespec alarm_ms = alarm_time(100);
        int reason = pthread_cond_timedwait(&alarm, &send_mutex, &alarm_ms);
        if(reason == ETIMEDOUT)
        {
            fprintf(stderr, "alarm went off \n");
            // fprintf(stderr, "t: %9ld\n ", get_timestamp()); 

            //signal the sender thread to send messages in window again
            WAKE_STATUS = TIMEOUT;
            pthread_cond_signal(&send_wait);
        }
        else if(reason == 0)
        {
            // this will happen only if timer is restarted via signaling
            // no need to do anything timer will restart itself
            fprintf(stderr, "alarm restart \n");
        }
        pthread_mutex_unlock(&send_mutex);
    }
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
        if(WAKE_STATUS == SEND_PACKET)
        {
            // send the packets only if they are ready to send
            for(; !isWindowFull(sender_buf.base, sender_buf.end)
                    && sender_buf.end != sender_buf.last + 1
                    ; INCR(sender_buf.end))
            {
                pthread_cond_signal(&alarm);
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
        }
        else if(WAKE_STATUS == TIMEOUT)
        {
            // send all the packets again
            // restarts timer
            pthread_cond_signal(&alarm);
            for(int i = sender_buf.base; 
                    i < sender_buf.end && i <= sender_buf.last ; i++)
            {
                
                fprintf(stderr, "end:%d, %u sending %s \n", sender_buf.end,

                        sender_buf.que[i].seq_num,
                        sender_buf.que[i].data
                        );
                if(sendto(  sd, 
                            (void*)&(sender_buf.que[i]), 
                            PAYLOAD_SIZE, 
                            0, 
                            (struct sockaddr *)&s_addr,
                            sizeof(s_addr)) == -1
                        ) { perror("sendto"); exit(1);}
            }
        }
        else
        { fprintf(stderr, "something went wrong\n");}
        pthread_mutex_unlock(&send_mutex);
    }
    return NULL;
}
void* receive_data_thread(void* args)
{
    // fprintf(stderr, "receive thread online\n");
    socklen_t addr_len;
    uint32_t expected_seqnum  = 0;
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
                    //TODO maybe use flag here to check if window is slided
                    while(sender_buf.que[sender_buf.base].seq_num <= p->seq_num
                            && sender_buf.base != sender_buf.end)
                    {
                        fprintf(stderr, "acked %u\n",  
                                sender_buf.que[sender_buf.base].seq_num);
                        // delete acked packet
                         
                        memset(&sender_buf.que[sender_buf.base], 0, sizeof(packet));
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
            if(p->seq_num == expected_seqnum)
            {
                // since printf buffers data untill it sees a '\n' 
                // i can accumulate data in its buffer
                // this sort of corresponds to "delivering data to upper layer"
                fprintf(stdout, "%s", p->data);

                // prepare ack packet
                packet ack_p;
                memset(&ack_p, 0, sizeof(packet));
                ack_p.type = TYPE_ACK;
                ack_p.seq_num = expected_seqnum;
                fprintf(stderr, "sending ack %u to pack %s\n", ack_p.seq_num, p->data);

                // send ack with expected_seqnum
                if(sendto(  sd, 
                            (void*)&ack_p, 
                            PAYLOAD_SIZE, 
                            0,
                            (struct sockaddr *) &s_addr,
                            sizeof(s_addr)) == -1)
                {perror("sendto"); exit(1);}
                expected_seqnum++; 
            }

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

        //fprintf(stderr, "input_len: %d input_echo %s", input_len, input_buf);
        //print_sender_q(&sender_buf);
        //fprintf(stderr, "base %d, end %d, last %d\n",
        // sender_buf.base, sender_buf.end, sender_buf.last);
        

        // signals the sender thread 
        // if(sender_buf.end - sender_buf.base + 1 != WINDOW_SIZE)

        // initially send and timer threads are doing nothing so we wake them up by
        // signalling broadcast
        // since after initial input timer thread never waits on send_cond
        // broadcasting only effects send thread
        WAKE_STATUS  = SEND_PACKET;
        pthread_cond_broadcast(&send_wait);  
    }

}
