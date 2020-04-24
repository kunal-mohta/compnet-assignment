#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>
#include <poll.h>
#include <time.h>

#include "common.h"

int main () {
	PACKET pkt_buf[WINDOW_SIZE];
	enum State pkt_status[WINDOW_SIZE];
	for (int i = 0; i < WINDOW_SIZE; i++) {
		pkt_status[i] = unack;
	}
	int window_start = 0, window_end = WINDOW_SIZE-1;

	int serv_fd;
	int r1_fd, r2_fd; // relay 1,2 connection sockets
	struct sockaddr_in serv_addr;

	// listening socket
	serv_fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (serv_fd < 0) {
		perror("ERROR in server listening socket creation");
		return 1;
	}

	// Setting server addr
	memset(&serv_addr, '0', sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(SERVER_PORT);
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);

	// bind socket and server addr
	if (bind(serv_fd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
		perror("Error in binding server address");
		return 1;
	}

	// Get relay 1 addr
	struct sockaddr_in relay1_addr;
	int r1addr_size = sizeof(relay1_addr);
	char r1_sync[SYNC_LEN+1];
	if (recvfrom(serv_fd, r1_sync, SYNC_LEN*sizeof(char), 0, (struct sockaddr *) &relay1_addr, &r1addr_size) == -1) {
		perror("Error in syncing with relay 1");
		return 1;
	}
	r1_sync[SYNC_LEN] = 0;
	printf("r1 sync: %s\n", r1_sync);

	// Get relay 2 addr
	struct sockaddr_in relay2_addr;
	int r2addr_size = sizeof(relay2_addr);
	char r2_sync[SYNC_LEN+1];
	if (recvfrom(serv_fd, r2_sync, SYNC_LEN*sizeof(char), 0, (struct sockaddr *) &relay2_addr, &r2addr_size) == -1) {
		perror("Error in syncing with relay 2");
		return 1;
	}
	r2_sync[SYNC_LEN] = 0;
	printf("r2 sync: %s\n", r2_sync);

	// output file
	FILE *fp = fopen(OUTPUT_FILE, "w");

	bool r1_closed = false, r2_closed = true;
	while (!r1_closed || !r2_closed) {

		PACKET rcv;
		int ret;

		memset(&rcv, 0, sizeof(rcv));
		r1addr_size = sizeof(relay1_addr);
		ret = recvfrom(serv_fd, &rcv, MAX_PACKET_SIZE, MSG_DONTWAIT, (struct sockaddr *) &relay1_addr, &r1addr_size);
		if (ret != -1) {
			// msg received from relay 1 

			if (ret != 0) { 
				// socket not closed

				print_packet(rcv, "RCVD from relay 1");

				// according to SR protocol
				// as given in Kurose-Ross textbook
				if (rcv.seqno >= window_start - WINDOW_SIZE && rcv.seqno <= window_end) {
					// send ACK
					PACKET pkt = create_new_packet(4, rcv.seqno, rcv.is_last, true, rcv.channel_id, "ACK");
					sendto(serv_fd, &pkt, MAX_PACKET_SIZE, 0, (struct sockaddr *) &relay1_addr, r1addr_size);
					print_packet(pkt, "SENT to relay 1");

					pkt_status[rcv.seqno % WINDOW_SIZE] = ack;
					pkt_buf[rcv.seqno % WINDOW_SIZE] = rcv;

					if (rcv.seqno == window_start) {
						int ind = window_start % WINDOW_SIZE;
						while (pkt_status[ind] == ack) {
							fwrite(pkt_buf[ind].payload, 1, pkt_buf[ind].size, fp);
							pkt_status[ind] = unack;
							window_start++;
							window_end++;
							ind = window_start % WINDOW_SIZE;
						}
					}
				}
			}
			else {
				r1_closed = true;
			}
		}

		memset(&rcv, 0, sizeof(rcv));
		r2addr_size = sizeof(relay2_addr);
		ret = recvfrom(serv_fd, &rcv, MAX_PACKET_SIZE, MSG_DONTWAIT, (struct sockaddr *) &relay2_addr, &r2addr_size);
		if (ret != -1) {
			// msg received from relay 2 

			if (ret != 0) {
				// socket not closed unexpectedly

				print_packet(rcv, "RCVD from relay 2");

				// according to SR protocol
				// as given in Kurose-Ross textbook
				if (rcv.seqno >= window_start - WINDOW_SIZE && rcv.seqno <= window_end) {
					// send ACK
					PACKET pkt = create_new_packet(4, rcv.seqno, rcv.is_last, true, rcv.channel_id, "ACK");
					sendto(serv_fd, &pkt, MAX_PACKET_SIZE, 0, (struct sockaddr *) &relay2_addr, r2addr_size);
					print_packet(pkt, "SENT to relay 2");

					pkt_status[rcv.seqno % WINDOW_SIZE] = ack;
					pkt_buf[rcv.seqno % WINDOW_SIZE] = rcv;

					if (rcv.seqno == window_start) {
						int ind = window_start % WINDOW_SIZE;
						while (pkt_status[ind] == ack) {
							fwrite(pkt_buf[ind].payload, 1, pkt_buf[ind].size, fp);
							pkt_status[ind] = unack;
							window_start++;
							window_end++;
							ind = window_start % WINDOW_SIZE;
						}
					}
				}
			} 
			else {
				r2_closed = true;
			}
		}
	}

	fclose(fp);
	close(r1_fd);
	close(r2_fd);
	close(serv_fd);
}
