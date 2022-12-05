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
int get_input_len(char * p)
{
    int count = 0;
    for(; *p; p++)
        count++;
    return count;
}
char isWindowFull(int base, int end)
{
    int count = 1;
    for(int i = base; i !=  end; i = (i+1) % QUE_SIZE)
        count++;
    return count == WINDOW_SIZE;
}
void form_and_que_packets(sender_que *q, char * inp, int size, uint32_t *seq)
{
    const int DATA_SIZE = 10;
    int i;
    int j;
    int k;
    for(i = 0; i < size/DATA_SIZE + 1; i++)
    {
        k = 0;
        q->last = (q->last + 1) % QUE_SIZE;
        q->isACKed[q->last] = 0;
        q->end = q->last;
        packet * cur_packet = &(q->que[q->last]);

        //if(q->last == q->base) {
        //    fprintf(stderr, "q->last == q->base, might be sender buffer overflow\n");
        //}


        for(j = i*DATA_SIZE; j < i*DATA_SIZE + 10; j++) {
            cur_packet->data[k++] = inp[j]; 
        }

        // q->que[q->last].checksum = checksum();
        q->que[q->last].seq_num = (*seq)++;
        if(*seq == UINT_MAX) {
           fprintf(stderr, "seq_num limit reached\n"); 

        }


    }
}

void print_sender_q(sender_que * q)
{
    for(int i = 0; i < 10; i++)
    {
        packet *p = &(q->que[i]);
        fprintf(stderr,"seq_num %u, data: %s\n", 
                p->seq_num, p->data);
    }
}
