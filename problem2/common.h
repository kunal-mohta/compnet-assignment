#ifndef COMMON_H
#define COMMON_H

#include <stdbool.h>

#define DELAY_MIN					0 // lower bound of delay added by relay (in milliseconds)
#define DELAY_MAX					2 // upper bound of delay added by relay (in milliseconds)
#define PACKET_SIZE					100
#define PDR							50 // packet drop rate (in percentage)
#define PKT_TIMEOUT					2 // retransmission time (in seconds - use decimal for ms)
#define INPUT_FILE					"input.txt"
#define OUTPUT_FILE					"output.txt"
#define WINDOW_SIZE					5 // also becomes the size of buffer for server

#define SERVER_PORT					5001
#define RELAY1_CLI_PORT				6001 // for relay as client socket
#define RELAY1_SERV_PORT			6002 // for relay as server socket
#define RELAY2_CLI_PORT				6003 // for relay as client socket
#define RELAY2_SERV_PORT			6004 // for relay as server socket
#define SERV_ADDR					"127.0.0.1"
#define RELAY1_ADDR					"127.0.0.2"
#define RELAY2_ADDR					"127.0.0.3"
#define MAX_LISTEN_QUEUE			10
#define SYNC_MSG					"sync"
#define SYNC_LEN					strlen(SYNC_MSG)

typedef struct _PACKET_ {
	int size;
	int seqno;
	bool is_last;
	bool is_ack;
	char payload[PACKET_SIZE];
} PACKET;

#define MAX_PACKET_SIZE				sizeof(PACKET)


// state of packets in window
enum State {
	unsent, // not used in server, only for client
	unack,
	ack
};

PACKET create_new_packet (int size, int seqno, bool is_last, bool is_ack, char *payload);
void print_packet (PACKET pkt, char *node_name, char *event, char *src, char *dst);
bool should_drop ();
double rand_range (double min, double max);
void mdelay (double time_ms);

#endif
