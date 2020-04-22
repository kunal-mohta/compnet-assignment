#ifndef COMMON_H
#define COMMON_H

#include <stdbool.h>

#define SERVER_PORT					5001
#define MAX_LISTEN_QUEUE			10
#define PACKET_SIZE					100
#define INPUT_FILE					"input.txt"
#define OUTPUT_FILE					"output.txt"

typedef struct _PACKET_ {
	int size;
	int seqno;
	bool is_last;
	bool is_ack;
	int channel_id;
	char payload[PACKET_SIZE];
} PACKET;

#define MAX_PACKET_SIZE				sizeof(PACKET) + (PACKET_SIZE*sizeof(char))

PACKET create_new_packet (int size, int seqno, bool is_last, bool is_ack, int cid, char *payload);
void print_packet (PACKET pkt, char *append);

#endif
