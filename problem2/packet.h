#ifndef PACKET_H
#define PACKET_H


#define PACKET_SIZE					100
#define PDR							50 // packet drop rate (in percentage)
#define PKT_TIMEOUT					2 // retransmission time (in seconds - use decimal for ms)
#define DELAY_MIN					0 // lower bound of delay added by relay (in milliseconds)
#define DELAY_MAX					2 // upper bound of delay added by relay (in milliseconds)
#define WINDOW_SIZE					5 // also becomes the size of buffer for server
#define SEQ_NOS						15 // should be >= 2*WINDOW_SIZE


typedef struct _PACKET_ {
	int size;
	int seqno;
	bool is_last;
	bool is_ack;
	char payload[PACKET_SIZE];
} PACKET;

#define MAX_PACKET_SIZE				sizeof(PACKET)

#endif
