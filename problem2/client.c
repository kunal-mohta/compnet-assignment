#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <time.h>

#include "common.h"

enum State {
	unsent,
	unack,
	ack
};

void send_packet_to_relay (PACKET pkt, int relay1_fd, int relay2_fd) {
	if (pkt.seqno % 2 == 0) {
		write(relay1_fd, &pkt, MAX_PACKET_SIZE);
		print_packet(pkt, "SENT to relay 1");
	}
	else {
		write(relay2_fd, &pkt, MAX_PACKET_SIZE);
		print_packet(pkt, "SENT to relay 2");
	}
}

int main () {
	enum State pkt_status[WINDOW_SIZE];
	clock_t timers[WINDOW_SIZE];
	int timer_base = 0;
	for (int i = 0; i < WINDOW_SIZE; i++) {
		timers[i] = -1;
		pkt_status[i] = unsent;
	}

	PACKET pkt_buf[WINDOW_SIZE];


	int relay1_fd, relay2_fd; // relay 1 (even), 2 (odd) sockets
	int bytes_sent = 0; // file offset till which already sent
	int curr_seqno = 0;
	int window_start = 0, window_end = WINDOW_SIZE-1;
	struct sockaddr_in relay1_addr, relay2_addr;

	// Socket creation for relay 1 
	relay1_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (relay1_fd < 0) {
		perror("ERROR in relay 1 socket creation");
		return 1;
	}

	// Socket creation for relay 2
	relay2_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (relay2_fd < 0) {
		perror("ERROR in relay 2 socket creation");
		return 1;
	}

	// Setting relay 1 address
	memset(&relay1_addr, '0', sizeof(relay1_addr));
	relay1_addr.sin_family = AF_INET;
	relay1_addr.sin_port = htons(EVEN_RELAY_PORT);
	relay1_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

	// Setting relay 2 address
	memset(&relay2_addr, '0', sizeof(relay2_addr));
	relay2_addr.sin_family = AF_INET;
	relay2_addr.sin_port = htons(ODD_RELAY_PORT);
	relay2_addr.sin_addr.s_addr = inet_addr("127.0.0.1");


	// Establish connection relay 1 
	if (connect(relay1_fd, (struct sockaddr *) &relay1_addr, sizeof(relay1_addr)) < 0) {
		perror("Error in relay 1 connect()");
		return 1;
	}

	// Establish connection channel 1
	if (connect(relay2_fd, (struct sockaddr *) &relay2_addr, sizeof(relay2_addr)) < 0) {
		perror("Error in relay 2 connect()");
		return 1;
	}

	FILE *fp = fopen(INPUT_FILE, "r");

	// making both sockets non-blocking
	int flags = fcntl(relay1_fd, F_GETFL, 0);
	fcntl(relay1_fd, F_SETFL, flags | O_NONBLOCK);
	flags = fcntl(relay2_fd, F_GETFL, 0);
	fcntl(relay2_fd, F_SETFL, flags | O_NONBLOCK);

	bool r1_stop = false, r2_stop = false, file_end = false;
	while (!r1_stop || !r2_stop) {
		if (file_end && pkt_status[window_start % WINDOW_SIZE] == unsent) break;

		// Send as many packets as possible to fill the window
		while (!file_end && curr_seqno <= window_end) {
			if (feof(fp) != 0) {
				file_end = true;
				break;
			}
			/*char msg[PACKET_SIZE+1];*/
			char msg[PACKET_SIZE];
			memset(msg, 0, PACKET_SIZE);
			int nread = fread(msg, 1, PACKET_SIZE, fp);
			/*msg[nread] = 0;*/

			PACKET pkt = create_new_packet(strlen(msg)*sizeof(char), curr_seqno, nread != PACKET_SIZE, false, 0, msg); // if nread != PACKET_SIZE, then packet is last

			send_packet_to_relay(pkt, relay1_fd, relay2_fd);

			pkt_buf[curr_seqno % WINDOW_SIZE] = pkt;
			pkt_status[curr_seqno % WINDOW_SIZE] = unack;

			// start timer for pkt
			clock_t start_time = clock();
			timers[curr_seqno % WINDOW_SIZE] = start_time;

			bytes_sent += nread;
			curr_seqno++;
		}

		// Check for timeouts and get min time left among all packets
		/*double min_time = -1;*/
		for (int i = 0; i < WINDOW_SIZE; i++) {
			if (timers[i] != -1 && pkt_status[i] == unack) {
				clock_t curr_time = clock();
				double diff = (double)(curr_time - timers[i])/CLOCKS_PER_SEC;

				if (diff > PKT_TIMEOUT) {
					// retransmission
					
					send_packet_to_relay(pkt_buf[i], relay1_fd, relay2_fd);

					// reset timer
					timers[i] = clock();

					// recalculate diff
					/*diff = (double)(curr_time - timers[i])/CLOCKS_PER_SEC;*/
				}
				/*
				 *double rem_time = PKT_TIMEOUT - diff;
				 *if (min_time == -1 || rem_time < min_time) min_time = rem_time;
				 */
			}
		}

		PACKET rcv;
		int nread;
		/*if (!r1_stop) {*/
			// try reading from relay 1 
			// only if eof not seen yet

			memset(&rcv, '0', sizeof(rcv));
			nread = read(relay1_fd, &rcv, MAX_PACKET_SIZE);
			if (nread != -1) {
				print_packet(rcv, "RCVD from relay 1");

				if (rcv.seqno >= window_start) {
					pkt_status[rcv.seqno % WINDOW_SIZE] = ack;
					if (rcv.seqno == window_start) {
						int ind = window_start % WINDOW_SIZE;
						while (pkt_status[ind] == ack) {
							pkt_status[ind] = unsent;
							window_start++;
							window_end++;
							ind = window_start % WINDOW_SIZE;
						}
					}
				}
				else {
					printf("Duplicate ACK\n");
				}
				
			}
		/*}*/
		

		/*if (!r2_stop) {*/
			// try reading from channel 1
			// only if eof not seen yet

			memset(&rcv, '0', sizeof(rcv));
			nread = read(relay2_fd, &rcv, MAX_PACKET_SIZE);
			if (nread != -1) {
				print_packet(rcv, "RCVD from relay 2");

				if (rcv.seqno >= window_start) {
					pkt_status[rcv.seqno % WINDOW_SIZE] = ack;
					if (rcv.seqno == window_start) {
						int ind = window_start % WINDOW_SIZE;
						while (pkt_status[ind] == ack) {
							pkt_status[ind] = unsent;
							window_start++;
							window_end++;
							ind = window_start % WINDOW_SIZE;
						}
					}
				}
				else {
					printf("Duplicate ACK\n");
				}
			}
		/*}*/
		
	}

	fclose(fp);
	close(relay1_fd);
	close(relay2_fd);
}
