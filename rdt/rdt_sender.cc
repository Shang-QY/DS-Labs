/*
 * FILE: rdt_sender.cc
 * DESCRIPTION: Reliable data transfer sender.
 * NOTE: This implementation assumes there is no packet loss, corruption, or 
 *       reordering.  You will need to enhance it to deal with all these 
 *       situations.  In this implementation, the packet format is laid out as 
 *       the following:
 *       
 *       |<-  1 byte  ->|<-             the rest            ->|
 *       | payload size |<-             payload             ->|
 *
 *       The first byte of each packet indicates the size of the payload
 *       (excluding this single-byte header)
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "rdt_struct.h"
#include "rdt_sender.h"

#include <list>
#include <queue>
#include "protocal_util.h"

using namespace std;

#define TIMEOUT 0.3

struct timerInfo{
    seq_t seq;
    double timeout;
    double start_time;
};

struct packetWithTimer{
    packet* pkt;
    timerInfo timer_info;
};

seq_t next_packet_to_send = 0;
seq_t expect_ack = 0;
list<packetWithTimer*> window = list<packetWithTimer*>();
queue<packet> buffer;

int times = 0;
FILE *sender_pls = 0;

bool available(){
    return window.size() < WINDOW_SIZE;
}

void pack(packet *pkt, char* data_source, pls_t pls, last_packet_t flag)
{
    debug_printf("pack");
    memcpy(pkt->data + HEADER_SIZE, data_source, pls);
    *(ref_seq(pkt)) = next_packet_to_send;
    *(ref_pls(pkt)) = pls;
    *(ref_last_packet(pkt)) = flag;
    *(ref_checksum2(pkt)) = gen_checksum2(pkt);
    *(ref_checksum(pkt)) = gen_checksum(pkt);
}

void send(packet *pkt){

    if(available()){
        
        packetWithTimer *p = new packetWithTimer;
        p->pkt = new packet;
        memcpy(p->pkt->data, pkt->data, RDT_PKTSIZE);
        timerInfo ti;
        ti.seq = get_seq(pkt);
        ti.timeout = TIMEOUT;
        ti.start_time = GetSimulationTime();
        p->timer_info = ti;
        if (window.empty() && !Sender_isTimerSet()) {
            Sender_StartTimer(TIMEOUT);
        }
        // debug_printf("((");
        // list<packetWithTimer*>::iterator it;
        // for(it = window.begin(); it != window.end(); ++it){
        //     packetWithTimer* pkt_with_timer = *(it);
        //     debug_printf("Push: send seq %d, pls %d, flag %d, checksum %d", get_seq(pkt_with_timer->pkt), get_pls(pkt_with_timer->pkt), get_last_packet(pkt_with_timer->pkt), get_checksum(pkt_with_timer->pkt));
        // }
        // debug_printf("))");
        window.push_back(p);
        debug_printf("next_packet_to_send: %d", next_packet_to_send);
        debug_printf("window size: %d, buffer size: %d", window.size(), buffer.size());
        debug_printf("Push: send seq %d, pls %d, flag %d, checksum %d", get_seq(p->pkt), get_pls(p->pkt), get_last_packet(p->pkt), get_checksum(p->pkt));
        debug_printf("(");
        list<packetWithTimer*>::iterator it;
        for(it = window.begin(); it != window.end(); ++it){
            packetWithTimer* pkt_with_timer = *(it);
            debug_printf("Push: send seq %d, pls %d, flag %d, checksum %d", get_seq(pkt_with_timer->pkt), get_pls(pkt_with_timer->pkt), get_last_packet(pkt_with_timer->pkt), get_checksum(pkt_with_timer->pkt));
        }
        debug_printf(")");
        Sender_ToLowerLayer(pkt);
    }
    else{
        debug_printf("Push to buffer: next_packet_to_send: %d, buffer size: %d", next_packet_to_send, buffer.size());
        buffer.push(*pkt);
    }
}

/* sender initialization, called once at the very beginning */
void Sender_Init()
{
    myOpen();
    sender_pls = fopen("Sender_pls.txt", "w");
    debug_printf("Header Size: %d", HEADER_SIZE);
    fprintf(stdout, "At %.2fs: sender initializing ...\n", GetSimulationTime());
}

/* sender finalization, called once at the very end.
   you may find that you don't need it, in which case you can leave it blank.
   in certain cases, you might want to take this opportunity to release some 
   memory you allocated in Sender_init(). */
void Sender_Final()
{
    fprintf(stdout, "At %.2fs: sender finalizing ...\n", GetSimulationTime());
    fclose(sender_pls);
    debug_printf("Header Size: %d", HEADER_SIZE);
}

/* event handler, called when a message is passed from the upper layer at the 
   sender */
void Sender_FromUpperLayer(struct message *msg)
{
    debug_printf("*******************************Sender_FromUpperLayer**************************************");
    /* maximum payload size */
    int maxpayload_size = MAX_PLS;
    
    /* split the message if it is too big */

    /* reuse the same packet data structure */
    packet pkt;

    /* the cursor always points to the first unsent byte in the message */
    int cursor = 0;

    while (msg->size-cursor > maxpayload_size) {
        /* fill in the packet */
        pack(&pkt, msg->data + cursor, (pls_t)maxpayload_size, 0);
        debug_printf("sender pack %d, seq %d", times, next_packet_to_send);
        inc_circularly(next_packet_to_send);
        /* send it out through the lower layer */
        send(&pkt);
        // char packet_content[RDT_PKTSIZE + 1];
        // packet_content[RDT_PKTSIZE] = '\0';
        // memcpy(packet_content, pkt.data, RDT_PKTSIZE);
        fprintf(sender_pls, "%d\t%d\t%d\t", times, maxpayload_size, get_checksum(&pkt));
        unsigned int *ptr = (unsigned int *)pkt.data;
        for(int i = 0; i < RDT_PKTSIZE / 4; ++i){
            fprintf(sender_pls, "%x", *ptr++);
        }
        fprintf(sender_pls, "\n");

        times++;
        /* move the cursor */
        cursor += maxpayload_size;

        /* clean packet memory */
        // memset(pkt.data, 0, RDT_PKTSIZE);
    }
    
    /* send out the last packet */
    if (msg->size > cursor) {
        /* fill in the packet */
        pack(&pkt, msg->data + cursor, (pls_t)(msg->size - cursor), 1);
        debug_printf("sender pack %d, seq %d", times, next_packet_to_send);
        inc_circularly(next_packet_to_send);
        /* send it out through the lower layer */
        send(&pkt);
        char packet_content[RDT_PKTSIZE + 1];
        packet_content[RDT_PKTSIZE] = '\0';
        memcpy(packet_content, pkt.data, RDT_PKTSIZE);
        fprintf(sender_pls, "%d\t%d\t%d\t", times, msg->size - cursor, get_checksum(&pkt));
        unsigned int *ptr = (unsigned int *)pkt.data;
        for(int i = 0; i < RDT_PKTSIZE / 4; ++i){
            fprintf(sender_pls, "%x", *ptr++);
        }
        fprintf(sender_pls, "\n");

        times++;
    }
    debug_printf("*******************************Sender_FromUpperLayer_End**************************************");
}

/* event handler, called when a packet is passed from the lower layer at the 
   sender */
void Sender_FromLowerLayer(struct packet *pkt)
{
    debug_printf("** Sender_FromLowerLayer Start**");
    // debug_printf("pkt address %x", pkt);
    if(!sanity_check(pkt) || get_last_packet(pkt) != 2) {
        debug_printf("drop ack %d", get_seq(pkt));
        return;
    }
    seq_t seq = get_seq(pkt);
    seq_t next_seq = (window.empty()) ? next_packet_to_send : (window.back()->timer_info.seq + 1) % MAX_SEQ;
    debug_printf("ack: expected_ack %d, actual_ack %d, next_seq %d", expect_ack, seq, next_seq);
    while(between(expect_ack, seq, next_seq)){
        debug_printf("while true");
        if(Sender_isTimerSet()) Sender_StopTimer();
        packetWithTimer* pkt_with_timer = window.front();
        window.erase(window.begin());
        debug_printf("~~~~~~~~~~~~~~~~~~free~seq %d ~~~address %x~~~~~~~~~~~~~~", pkt_with_timer->timer_info.seq, pkt_with_timer->pkt);
        free(pkt_with_timer->pkt);
        free(pkt_with_timer);
        inc_circularly(expect_ack);
        debug_printf("window size now: %d", window.size());
    }
    if(!window.empty()){
        packetWithTimer* pkt_with_timer = *(window.begin());
        Sender_StartTimer(pkt_with_timer->timer_info.start_time + pkt_with_timer->timer_info.timeout - GetSimulationTime());
    }
    debug_printf("total packet numbers: %d", times);
    debug_printf("buffer size: %d", buffer.size());
    while(available() && !buffer.empty()){
        packet p = buffer.front();
        buffer.pop();
        send(&p);
    }
    debug_printf("** Sender_FromLowerLayer End**");
}

/* event handler, called when the timer expires */
void Sender_Timeout()
{
    // if(times > 0) return;

    list<packetWithTimer*> old_window = window;
    
    window.clear();
    debug_printf("__________________________________________________timeout___________________________________________________");
    list<packetWithTimer*>::iterator it = old_window.begin();
    for(; it != old_window.end(); ++it){
        packetWithTimer* pkt_with_timer = *it;
        send(pkt_with_timer->pkt);
        debug_printf("~~~~~~~~~~~~~~~~~~free~~~~~~~~~~~~~~~~~~~__________");
        free(pkt_with_timer->pkt);
        free(pkt_with_timer);
    }
    // int ws = window.size();
    // for(int i = 0; i < ws; ++i){
    //     packetWithTimer* pkt_with_timer = window.front();
    //     window.erase(window.begin());
    //     send(pkt_with_timer->pkt);
    // }
    debug_printf("_____________timeout___________________________________________________windowsize: %d", window.size());
    // times++;
}
