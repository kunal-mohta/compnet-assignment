#ifndef COMMON_H
#define COMMON_H

#include <stdbool.h>

#define SERVER_PORT					5001
#define ODD_RELAY_PORT				6001
#define EVEN_RELAY_PORT				6002
#define MAX_LISTEN_QUEUE			10
#define PACKET_SIZE					100
#define INPUT_FILE					"input.txt"
#define OUTPUT_FILE					"output.txt"
#define PDR							50 // packet drop rate (in percentage)
#define PKT_TIMEOUT					2 // retransmission time (in seconds)
#define WINDOW_SIZE					5
#define DELAY_MIN					0 // retransmission time (in milliseconds)
#define DELAY_MAX					2 // retransmission time (in milliseconds)
#define SYNC_MSG					"sync"
#define SYNC_LEN					strlen(SYNC_MSG)

enum State {
	unsent,
	unack,
	ack
};

typedef struct _PACKET_ {
	int size;
	int seqno;
	bool is_last;
	bool is_ack;
	int channel_id;
	char payload[PACKET_SIZE];
} PACKET;

#define MAX_PACKET_SIZE				sizeof(PACKET)

PACKET create_new_packet (int size, int seqno, bool is_last, bool is_ack, int cid, char *payload);
void print_packet (PACKET pkt, char *append);
bool should_drop ();
double rand_range (double min, double max);
void mdelay (double time_ms);

#endif
