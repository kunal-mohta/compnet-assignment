#ifndef PACKET_H
#define PACKET_H

#include "common.h"

#define PACKET_SIZE					100
#define PDR							10 // packet drop rate (in percentage)
#define PKT_TIMEOUT					2 // retransmission time (in seconds - use decimal for ms)
#define BUF_SIZE					5 // in terms of packets that can fit

typedef struct _PACKET_ {
	int size;
	int seqno;
	bool is_last;
	bool is_ack;
	int channel_id;
	char payload[PACKET_SIZE];
} PACKET;

#define MAX_PACKET_SIZE				sizeof(PACKET)


#endif
