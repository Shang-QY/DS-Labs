#include "protocal_util.h"
#include <cstdarg>
#include <stdio.h>

FILE *logFile;

void myOpen() {
    logFile = fopen("log.txt", "w");
    if(logFile == NULL){
        fprintf(stdout, "can't open log file\n");
    }
}

// a <= b < c, circularly
bool between(seq_t a, seq_t b, seq_t c) {
    return (a <= b && b < c) || (c < a && a <= b) || (b < c && c < a);
}

bool less_than(seq_t expected, seq_t in, int ws) {
    return expected != in && between(expected, in, (expected+ws)%MAX_SEQ);
}

void inc_circularly(seq_t &seq) {
    seq++;
    if (seq >= MAX_SEQ) {
        seq = 0;
    }
}

pls_t *ref_pls(packet *pkt) {
    return (pls_t *)((packet_header *)pkt)->payload_size;
}

seq_t *ref_seq(packet *pkt) {
    return (seq_t *)((packet_header *)pkt)->seq;
}

checksum_t *ref_checksum(packet *pkt) {
    return (checksum_t *)((packet_header *)pkt)->checksum;
}

checksum_t *ref_checksum2(packet *pkt) {
    return (checksum_t *)((packet_header *)pkt)->checksum2;
}

last_packet_t *ref_last_packet(packet* pkt) {
    return (last_packet_t *)((packet_header *)pkt)->last_packet;
}

pls_t get_pls(packet *pkt) {
    return *(ref_pls(const_cast<packet *>(pkt)));
}

seq_t get_seq(packet *pkt) {
    return *(ref_seq(const_cast<packet *>(pkt)));
}

checksum_t get_checksum(packet *pkt) {
    return *(ref_checksum(const_cast<packet *>(pkt)));
}

checksum_t get_checksum2(packet *pkt) {
    return *(ref_checksum2(const_cast<packet *>(pkt)));
}

last_packet_t get_last_packet(packet *pkt) {
    return *(ref_last_packet(const_cast<packet *>(pkt)));
}

checksum_t gen_checksum(packet* pkt)
{
    char *buf = pkt->data + CHECKSUM_SIZE;
    int byte_number = get_pls(pkt) + HEADER_SIZE - CHECKSUM_SIZE;
    // int byte_number = RDT_PKTSIZE - CHECKSUM_SIZE;
    unsigned long sum;
 
    for(sum = 0; byte_number > 0; byte_number--)
        sum += *buf++;             
 
    sum  = (sum>>16) + (sum&0xffff);
    sum += (sum>>16);
 
    return ~sum;
}

checksum_t gen_checksum2(packet* pkt)
{
    char *buf = pkt->data + CHECKSUM_SIZE + CHECKSUM_SIZE;
    int byte_number = get_pls(pkt) + HEADER_SIZE - CHECKSUM_SIZE - CHECKSUM_SIZE;
    // int byte_number = RDT_PKTSIZE - CHECKSUM_SIZE;
    unsigned long sum;
 
    for(sum = 0; byte_number > 0; byte_number--)
        sum += *buf++;             
 
    sum  = (sum>>16) + (sum&0xffff);
    sum += (sum>>16);
 
    return ~sum;
}

bool verify_checksum(packet* pkt)
{
    char *buf = pkt->data + CHECKSUM_SIZE;
    int byte_number = get_pls(pkt) + HEADER_SIZE - CHECKSUM_SIZE;
    // int byte_number = RDT_PKTSIZE - CHECKSUM_SIZE;
    checksum_t checksum = get_checksum(pkt);

    unsigned long sum;
 
    for(sum = 0; byte_number > 0; byte_number--)
        sum += *buf++;             
 
    sum  = (sum>>16) + (sum&0xffff);
    sum += (sum>>16);

    return (((sum + checksum) & 0xff) == 0xff);
}

bool verify_checksum2(packet* pkt)
{
    char *buf = pkt->data + CHECKSUM_SIZE + CHECKSUM_SIZE;
    int byte_number = get_pls(pkt) + HEADER_SIZE - CHECKSUM_SIZE - CHECKSUM_SIZE;
    // int byte_number = RDT_PKTSIZE - CHECKSUM_SIZE;
    checksum_t checksum = get_checksum2(pkt);

    unsigned long sum;
 
    for(sum = 0; byte_number > 0; byte_number--)
        sum += *buf++;             
 
    sum  = (sum>>16) + (sum&0xffff);
    sum += (sum>>16);

    return (((sum + checksum) & 0xff) == 0xff);
}

bool sanity_check(packet *pkt) {
    pls_t pls = get_pls(pkt);
    seq_t seq = get_seq(pkt);
    last_packet_t flag = get_last_packet(pkt);
    return pls <= MAX_PLS && seq <= MAX_SEQ && (flag == 0 || flag == 1 || flag == 2) && verify_checksum2(pkt) && verify_checksum(pkt);
}

void debug_printf(const char *format, ...) {
    if (DEBUG) {
        va_list args;
        va_start(args, format);
        fprintf(stdout, "At %.2fs: ", GetSimulationTime());
        vfprintf(stdout, format, args);
        fprintf(stdout, "\n");
        va_end(args);
    }
    if (WRITELOG) {
        va_list args;
        va_start(args, format);
        fprintf(logFile, "At %.2fs: ", GetSimulationTime());
        vfprintf(logFile, format, args);
        fprintf(logFile, "\n");
        va_end(args);
    }
}
