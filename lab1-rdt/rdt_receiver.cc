/*
 * FILE: rdt_receiver.cc
 * DESCRIPTION: Reliable data transfer receiver.
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
#include "rdt_receiver.h"

#include <list>
#include "protocal_util.h"
#include <iostream>
#include <fstream>

using namespace std;

list<packet*> buffer;
seq_t expect_seq = 0;

int rtimes = 0;
FILE *receiver_pls;

void outputFalsePkt(){
    int sender_i, receiver_i;
    int sender_payload_size, receiver_payload_size;
    int sender_checksum, receiver_checksum;
    string sender_content, receiver_content;

    ifstream sender_in("Sender_pls.txt");
    ifstream receiver_in("Receiver_pls.txt");
    ofstream false_out("False_pkt");
    int times = 0;
    while(!sender_in.eof() && !receiver_in.eof()){
        sender_in >> sender_i;
        sender_in >> sender_payload_size;
        sender_in >> sender_checksum;
        sender_in >> sender_content;
        receiver_in >> receiver_i;
        receiver_in >> receiver_payload_size;
        receiver_in >> receiver_checksum;
        receiver_in >> receiver_content;
        if(sender_i != receiver_i){
            fprintf(stdout, "index is not same s: %d r: %d", sender_i, receiver_i);
        }
        if(sender_payload_size != receiver_payload_size){
            false_out << sender_i << "\t" << sender_payload_size << "\t" << receiver_payload_size << endl;
            false_out << "sender\t" << sender_content << endl;
            false_out << "receiver\t" << receiver_content << endl;
            false_out << "sender origin checksum\t" << sender_checksum << /*"calcu checksum\t" << gen_checksum((packet *)(sender_content.c_str())) << "suit\t" << (int)verify_checksum((packet *)(sender_content.c_str())) <<*/ endl;
            false_out << "receiver origin checksum\t" << receiver_checksum << /*"calcu checksum\t" << gen_checksum((packet *)(receiver_content.c_str())) << "suit\t" << (int)verify_checksum((packet *)(receiver_content.c_str())) <<*/ endl;

        }
        debug_printf("time %d", times);
        times++;
        // if(times == 20) break;
    }
    sender_in.close();
    receiver_in.close();
    false_out.close();
}

void ack(seq_t seq){
    packet pkt;
    *(ref_seq(&pkt)) = seq;
    *(ref_pls(&pkt)) = 0;
    *(ref_last_packet(&pkt)) = 2;
    *(ref_checksum2(&pkt)) = gen_checksum2(&pkt);
    *(ref_checksum(&pkt)) = gen_checksum(&pkt);
    Receiver_ToLowerLayer(&pkt);
}

/* receiver initialization, called once at the very beginning */
void Receiver_Init()
{
    fprintf(stdout, "At %.2fs: receiver initializing ...\n", GetSimulationTime());
    receiver_pls = fopen("Receiver_pls.txt", "w");
}

/* receiver finalization, called once at the very end.
   you may find that you don't need it, in which case you can leave it blank.
   in certain cases, you might want to use this opportunity to release some 
   memory you allocated in Receiver_init(). */
void Receiver_Final()
{
    fprintf(stdout, "At %.2fs: receiver finalizing ...\n", GetSimulationTime());
    fclose(receiver_pls);
    outputFalsePkt();
}

/* event handler, called when a packet is passed from the lower layer at the 
   receiver */
void Receiver_FromLowerLayer(struct packet *pkt)
{
    debug_printf("*Receiver_FromLowerLayer pkt %d", get_seq(pkt));
    if(!sanity_check(pkt)) {
        debug_printf("drop packet %d", get_seq(pkt));
        return;
    }
    seq_t seq = get_seq(pkt);
    if(seq == expect_seq){
        inc_circularly(expect_seq);
        debug_printf("Handle: seq %d as expected", seq);
        ack(seq);
        
        /***********************************************************/
        /* immidiately put packet to Upper layer */
        struct message *msg = (struct message*) malloc(sizeof(struct message));
        ASSERT(msg!=NULL);
        msg->size = get_pls(pkt);
        msg->data = (char*) malloc(msg->size);
        ASSERT(msg->data!=NULL);
        memcpy(msg->data, pkt->data+HEADER_SIZE, msg->size);
        Receiver_ToUpperLayer(msg);
        debug_printf("receiver receive %d, seq %d", rtimes, seq);

        // char packet_content[RDT_PKTSIZE + 1];
        // packet_content[RDT_PKTSIZE] = '\0';
        // memcpy(packet_content, pkt->data, RDT_PKTSIZE);
        fprintf(receiver_pls, "%d\t%d\t%d\t", rtimes, msg->size, get_checksum(pkt));
        unsigned int *ptr = (unsigned int *)pkt->data;
        for(int i = 0; i < RDT_PKTSIZE / 4; ++i){
            fprintf(receiver_pls, "%x", *ptr++);
        }
        fprintf(receiver_pls, "\n");

        rtimes++;

        /* don't forget to free the space */
        if (msg->data!=NULL) free(msg->data);
        if (msg!=NULL) free(msg);
        debug_printf("___________________________________________seq: %d, upload message size: %d", seq, get_pls(pkt));
        /***********************************************************/

        /***********************************************************/
        /* join packet together to form a message */
        // if(get_last_packet(pkt) == 0){
        //     packet *p = new packet;
        //     memcpy(p->data, pkt->data, RDT_PKTSIZE);
        //     buffer.push_back(p);
        // }
        // if(get_last_packet(pkt) == 1){
        //     packet *p = new packet;
        //     memcpy(p->data, pkt->data, RDT_PKTSIZE);
        //     buffer.push_back(p);

        //     /* construct a message and deliver to the upper layer */
        //     struct message *msg = (struct message*) malloc(sizeof(struct message));
        //     ASSERT(msg!=NULL);

        //     int s = 0;
        //     for(list<packet*>::iterator it = buffer.begin(); it != buffer.end(); ++it){
        //         s += get_pls(*it);
        //     }
        //     msg->size = s;
        //     msg->data = (char*) malloc(msg->size);
        //     ASSERT(msg->data!=NULL);

        //     int cursor = 0;
        //     for(list<packet*>::iterator it = buffer.begin(); it != buffer.end(); ++it){
        //         packet *it_pkt = *it;
        //         memcpy(msg->data + cursor, it_pkt->data + HEADER_SIZE, get_pls(it_pkt));
        //         free(it_pkt);
        //         cursor += get_pls(*it);
        //     }
        //     Receiver_ToUpperLayer(msg);
        //     debug_printf("___________________________________________seq: %d, buffer size: %d, upload message size: %d", seq, buffer.size(), cursor);
        //     /* don't forget to free the space */
        //     if (msg->data!=NULL) free(msg->data);
        //     if (msg!=NULL) free(msg);
        //     buffer.clear();
        // }
        /***************************************************************/
    }
    else if(less_than(seq, expect_seq, REC_WINDOW_SIZE)){
        ack(seq);
        debug_printf("Handle: stale seq %d, expect_seq: %d, resend ack", seq, expect_seq);
    }
}
