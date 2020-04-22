#include <stdio.h>
#include <string.h>

#include "common.h"

PACKET create_new_packet (int size, int seqno, bool is_last, bool is_ack, int cid, char *payload) {
	PACKET pkt;
	pkt = (PACKET) {
		.size = size,
		.seqno = seqno,
		.is_last = is_last,
		.is_ack = is_ack,
		.channel_id = cid
	};
	strcpy(pkt.payload, payload);
	return pkt;
}

void print_packet (PACKET pkt, char *append) {
	if (pkt.is_ack) {
		printf("\n%s ACK: for PKT with Seq. no. %d from channel %d\n", append, pkt.seqno, pkt.channel_id);
	}
	else {
		printf("\n%s PKT: Seq. no. %d of size %d bytes from channel %d\n", append, pkt.seqno, pkt.size, pkt.channel_id);
	}
	/*printf("msg: %s\n", pkt.payload);*/
}
