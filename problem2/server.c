#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>
#include <poll.h>
#include <time.h>

#include "common.h"
#include "packet.h"

// Packet window buffer related initialization
char *pkt_buf[WINDOW_SIZE];
enum State pkt_status[WINDOW_SIZE];
int window_start = 0, window_end = WINDOW_SIZE-1;
int window_start_ind = 0;

bool is_seqno_in_window (int seqno) {
	if (window_start < window_end && seqno <= window_end && seqno >= window_start)
		return true;
	else if (window_start >= window_end && seqno >= window_start || seqno <= window_end)
		return true;
	else return false;
}

int seqno_diff (int x, int y) {
	if (x <= y)
		return y-x;
	else {
		return SEQ_NOS-x+y;
	}
}

bool insert_in_buf (PACKET p) {
	int seqno = p.seqno;
	if (is_seqno_in_window(seqno)) {
		int diff = seqno_diff(window_start, seqno);
		int i = window_start_ind;
		while (diff) {
			i = (i + 1) % WINDOW_SIZE;
			diff--;
		}
		pkt_status[i] = ack;
		char *payload = (char *) malloc((p.size+1)*sizeof(char));
		memcpy(payload, p.payload, (p.size)*sizeof(char));
		payload[p.size] = 0;
		/*strcpy(payload, p.payload);*/
		pkt_buf[i] = payload;
		return true;
	}
	else return false;
}

int main () {
	if (SEQ_NOS < 2*WINDOW_SIZE) {
		printf("SEQ_NOS is set less than 2*WINDOW_SIZE\nCannot procede\n");
		return 1;
	}

	// Initializing buffers
	for (int i = 0; i < WINDOW_SIZE; i++) {
		pkt_status[i] = unack;
		pkt_buf[i] = NULL;
	}

	int serv_fd;
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

	// Get relay 2 addr
	struct sockaddr_in relay2_addr;
	int r2addr_size = sizeof(relay2_addr);
	char r2_sync[SYNC_LEN+1];
	if (recvfrom(serv_fd, r2_sync, SYNC_LEN*sizeof(char), 0, (struct sockaddr *) &relay2_addr, &r2addr_size) == -1) {
		perror("Error in syncing with relay 2");
		return 1;
	}
	r2_sync[SYNC_LEN] = 0;

	// output file
	FILE *fp = fopen(OUTPUT_FILE, "w");
	printf("\n%-10s %-10s %-20s %-13s %-10s %-10s %-10s\n", "Node name", "Event", "Timestamp", "Packet type", "Seq. no.", "Source", "Dest");

	bool r1_closed = false, r2_closed = true;
	while (!r1_closed || !r2_closed) {

		PACKET rcv;
		int ret;

		memset(&rcv, 0, sizeof(rcv));

		struct sockaddr_in relay_addr;
		int raddr_size = sizeof(relay_addr);
		raddr_size = sizeof(relay_addr);
		ret = recvfrom(serv_fd, &rcv, MAX_PACKET_SIZE, MSG_DONTWAIT, (struct sockaddr *) &relay_addr, &raddr_size);

		if (ret != -1) {
			// msg received from relay 
			
			// finding out which relay it is from
			int relay_num = 0;
			char relay_name[10];
			if (relay_addr.sin_addr.s_addr == relay1_addr.sin_addr.s_addr
					&& relay_addr.sin_port == relay1_addr.sin_port) {
				relay_num = 1;
				strcpy(relay_name, "RELAY1");
			}
			else if (relay_addr.sin_addr.s_addr == relay2_addr.sin_addr.s_addr
					&& relay_addr.sin_port == relay2_addr.sin_port) {
				relay_num = 2;
				strcpy(relay_name, "RELAY2");
			}

			if (ret != 0) { 
				// socket not closed

				print_packet(rcv, "SERVER", "R", relay_name, "SERVER");

				// according to SR protocol
				if (is_seqno_in_window(rcv.seqno)) {
					// send ACK
					PACKET pkt = create_new_packet(4, rcv.seqno, rcv.is_last, true, "ACK");
					print_packet(pkt, "SERVER", "S", "SERVER", relay_name);
					sendto(serv_fd, &pkt, MAX_PACKET_SIZE, 0, (struct sockaddr *) &relay_addr, raddr_size);

					if (!insert_in_buf(rcv)) {
						/*printf("Buffer full\n");*/
					}

					if (rcv.seqno == window_start) {
						int ind = window_start_ind;
						while (pkt_status[ind] == ack) {
							fwrite(pkt_buf[ind], 1, strlen(pkt_buf[ind]), fp);
							pkt_status[ind] = unack;
							free(pkt_buf[ind]);
							pkt_buf[ind] = NULL;
							window_start = (window_start + 1) % SEQ_NOS;
							window_end = (window_end + 1) % SEQ_NOS;
							window_start_ind = (window_start_ind + 1) % WINDOW_SIZE;
							ind = window_start_ind; 
						}
					}
				}
			}
			else {
				r1_closed = true;
			}
		}
	}

	fclose(fp);
	close(serv_fd);
}
