#include "rdt_struct.h"
#include <stdio.h>

#define DEBUG 0
#define WRITELOG 0

#define REC_WINDOW_SIZE 4
#define WINDOW_SIZE (2 * REC_WINDOW_SIZE)
#define MAX_SEQ (3 * WINDOW_SIZE)

typedef unsigned short checksum_t;
#define CHECKSUM_SIZE sizeof(checksum_t)
typedef unsigned char seq_t;
#define SEQ_SIZE sizeof(seq_t)
typedef unsigned char pls_t;
#define PLS_SIZE sizeof(pls_t)
typedef unsigned char last_packet_t;
#define FLAG_SIZE sizeof(last_packet_t)

void myOpen();

// a <= b < c, circularly
bool between(seq_t a, seq_t b, seq_t c);
// a < b, circularly, in a window of size ws
bool less_than(seq_t a, seq_t b, int ws);
void inc_circularly(seq_t &seq);

struct packet_header
{
    // checksum_t checksum;
    char checksum[CHECKSUM_SIZE];
    char checksum2[CHECKSUM_SIZE];
    // seq_t seq;
    // pls_t payload_size;
    // last_packet_t last_packet;
    char seq[SEQ_SIZE];
    char payload_size[PLS_SIZE];
    char last_packet[FLAG_SIZE];
};

#define HEADER_SIZE sizeof(packet_header)
#define MAX_PLS RDT_PKTSIZE - HEADER_SIZE

checksum_t* ref_checksum(packet* pkt);
checksum_t *ref_checksum2(packet *pkt);
seq_t* ref_seq(packet* pkt);
pls_t* ref_pls(packet* pkt);
last_packet_t* ref_last_packet(packet* pkt);

checksum_t get_checksum(packet* pkt);
checksum_t get_checksum2(packet *pkt);
seq_t get_seq(packet* pkt);
pls_t get_pls(packet* pkt);
last_packet_t get_last_packet(packet* pkt);

checksum_t gen_checksum(packet* pkt);
checksum_t gen_checksum2(packet* pkt);
bool verify_checksum(packet* pkt);
bool verify_checksum2(packet* pkt);
bool sanity_check(packet *pkt);

double GetSimulationTime();
void debug_printf(const char *format, ...);
