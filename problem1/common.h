#ifndef COMMON_H
#define COMMON_H

#include <stdbool.h>

#define PACKET_SIZE					100
#define PDR							10 // packet drop rate (in percentage)
#define PKT_TIMEOUT					2 // retransmission time (in seconds - use decimal for ms)
#define BUF_SIZE					5 // in terms of packets that can fit

#define INPUT_FILE					"input.txt"
#define OUTPUT_FILE					"output.txt"
#define SERVER_PORT					5001
#define MAX_LISTEN_QUEUE			10

typedef struct _PACKET_ {
	int size;
	int seqno;
	bool is_last;
	bool is_ack;
	int channel_id;
	char payload[PACKET_SIZE];
} PACKET;

#define MAX_PACKET_SIZE				sizeof(PACKET)


// for validity of packet buffer index
enum State {
	valid,
	invalid
};

// prototypes for functions in utils.c (used both in server and client)
PACKET create_new_packet (int size, int seqno, bool is_last, bool is_ack, int cid, char *payload);
void print_packet (PACKET pkt, char *append);
bool should_drop ();

#endif
