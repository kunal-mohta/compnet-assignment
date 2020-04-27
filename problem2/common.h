#ifndef COMMON_H
#define COMMON_H

#include <stdbool.h>

#include "packet.h"

#define INPUT_FILE					"input.txt"
#define OUTPUT_FILE					"output.txt"
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


// state of packets in window
enum State {
	unsent, // not used in server, only for client
	unack, // for server, this means invalid
	ack // for server, this means valid
};

PACKET create_new_packet (int size, int seqno, bool is_last, bool is_ack, char *payload);
void print_packet (PACKET pkt, char *node_name, char *event, char *src, char *dst);
bool should_drop ();
double rand_range (double min, double max);
void mdelay (double time_ms);

#endif
