#ifndef COMMON_H
#define COMMON_H

#include <stdbool.h>

#include "packet.h"

#define INPUT_FILE					"input.txt"
#define OUTPUT_FILE					"output.txt"
#define SERVER_PORT					5001
#define SERV_ADDR					"127.0.0.1"
#define MAX_LISTEN_QUEUE			10 // for listen()

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
