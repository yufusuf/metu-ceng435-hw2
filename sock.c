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
    for(int i = base; i !=  end; INCR(i))
        count++;

    return count == WINDOW_SIZE + 1;
}
void form_and_que_packets(sender_que *q, char * inp, int size, uint32_t *seq)
{
    int i;
    int j;
    int k;
    int packet_count = size % DATA_SIZE == 0 ? size/DATA_SIZE : size/DATA_SIZE + 1;

    for(i = 0; i < packet_count; i++)
    {
        k = 0;
        INCR(q->last);
        packet * cur_packet = &(q->que[q->last]);

        //if(q->last == q->base && q->base != 0) {
        //    fprintf(stderr, "q->last == q->base, might be sender buffer overflow\n");
        //}


        for(j = i*DATA_SIZE; j < (i + 1)*DATA_SIZE; j++) {
            cur_packet->data[k] = inp[j]; 
            k++;
            
        }

        // q->que[q->last].checksum = checksum();
        q->que[q->last].seq_num = (*seq);
        *seq = *seq + 1;
        q->que[q->last].type = TYPE_DATA;
        if(*seq == UINT_MAX) {
           fprintf(stderr, "seq_num limit reached\n"); 

        }


    }
}

void print_sender_q(sender_que * q)
{
    for(int i = q->base; i <= q->last; i++)
    {
        packet *p = &(q->que[i]);
        fprintf(stderr,"seq_num %u, data: %s\n", 
                p->seq_num, p->data);
    }
}
