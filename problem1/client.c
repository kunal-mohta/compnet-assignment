#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <time.h>

#include "common.h"

int main () {
	int c0_sockfd, c1_sockfd; // channel 0, 1 sockets
	int bytes_sent = 0; // file offset till which already sent
	struct sockaddr_in serv_addr;

	// Socket creation for channel 0
	c0_sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (c0_sockfd < 0) {
		perror("ERROR in channel 0 socket creation");
		return 1;
	}

	// Socket creation for channel 1
	c1_sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (c1_sockfd < 0) {
		perror("ERROR in channel 1 socket creation");
		return 1;
	}

	// Setting server address
	memset(&serv_addr, '0', sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(SERVER_PORT);
	serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

	// Establish connection channel 0
	if (connect(c0_sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
		perror("Error in channel 0 connect()");
		return 1;
	}

	// Establish connection channel 1
	if (connect(c1_sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
		perror("Error in channel 1 connect()");
		return 1;
	}

	FILE *fp = fopen(INPUT_FILE, "r");


	// First packet send, on channel 0
	char c0_msg[PACKET_SIZE+1];
	int nread = fread(c0_msg, 1, PACKET_SIZE, fp);
	c0_msg[nread] = 0;

	PACKET c0_pkt = create_new_packet(strlen(c0_msg)*sizeof(char), bytes_sent, nread != PACKET_SIZE, false, 0, c0_msg); // if nread != PACKET_SIZE, then packet is last
	bytes_sent += nread;

	write(c0_sockfd, &c0_pkt, MAX_PACKET_SIZE+1);
	print_packet(c0_pkt, "SENT");

	/*if (nread != PACKET_SIZE) return 0; // first packet was the last packet*/

	// start timer for channel 0 pkt
	clock_t c0_start_time = clock();


	// Second packet send, on channel 1
	char c1_msg[PACKET_SIZE+1];
	nread = fread(c1_msg, 1, PACKET_SIZE, fp);
	c1_msg[nread] = 0;

	PACKET c1_pkt = create_new_packet(strlen(c1_msg)*sizeof(char), bytes_sent, nread != PACKET_SIZE, false, 1, c1_msg); // if nread != PACKET_SIZE, then packet is last
	bytes_sent += nread;

	write(c1_sockfd, &c1_pkt, MAX_PACKET_SIZE+1);
	print_packet(c1_pkt, "SENT");

	/*if (nread != PACKET_SIZE) return 0; // first packet was the last packet*/

	// start timer for channel 1 pkt
	clock_t c1_start_time = clock();


	// making both sockets non-blocking
	int flags = fcntl(c0_sockfd, F_GETFL, 0);
	fcntl(c0_sockfd, F_SETFL, flags | O_NONBLOCK);
	flags = fcntl(c1_sockfd, F_GETFL, 0);
	fcntl(c1_sockfd, F_SETFL, flags | O_NONBLOCK);

	bool c0_stop = false, c1_stop = false;
	while (!c0_stop || !c1_stop) {
		PACKET rcv;

		if (!c0_stop) {
			// try reading from channel 0
			// only if eof not seen yet

			memset(&rcv, '0', sizeof(rcv));
			nread = read(c0_sockfd, &rcv, MAX_PACKET_SIZE+1);
			if (nread == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
				clock_t curr_time = clock();

				double diff = (double)(curr_time - c0_start_time)/CLOCKS_PER_SEC;

				if (diff > PKT_TIMEOUT) {
					// retransmission
					
					write(c0_sockfd, &c0_pkt, MAX_PACKET_SIZE+1);
					print_packet(c0_pkt, "SENT");

					// reset channel 0 timer
					c0_start_time = clock();
				}
			}
			else {
				print_packet(rcv, "RCVD");

				if (rcv.seqno != c0_pkt.seqno) {
					// this should not occur in the give protocol
					printf("Channel 0: ACK received for incorrect packet\n");
				}

				if (feof(fp) != 0) c0_stop = true;
				else {
					// create new packet and send
					char c0_msg[PACKET_SIZE+1];
					nread = fread(c0_msg, 1, PACKET_SIZE, fp);
					c0_msg[nread] = 0;
					c0_pkt = create_new_packet(strlen(c0_msg)*sizeof(char), bytes_sent, nread != PACKET_SIZE, false, 0, c0_msg);
					bytes_sent += nread;

					write(c0_sockfd, &c0_pkt, MAX_PACKET_SIZE+1);
					print_packet(c0_pkt, "SENT");

					// reset channel 0 timer
					c0_start_time = clock();
				}
			}
		}
		

		if (!c1_stop) {
			// try reading from channel 1
			// only if eof not seen yet

			memset(&rcv, '0', sizeof(rcv));
			nread = read(c1_sockfd, &rcv, MAX_PACKET_SIZE+1);
			if (nread == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
				clock_t curr_time = clock();

				double diff = (double)(curr_time - c1_start_time)/CLOCKS_PER_SEC;

				if (diff > PKT_TIMEOUT) {
					// retransmission
					
					write(c1_sockfd, &c1_pkt, MAX_PACKET_SIZE+1);
					print_packet(c1_pkt, "SENT");

					// reset channel 1 timer
					c1_start_time = clock();
				}
			}
			else {
				print_packet(rcv, "RCVD");

				if (rcv.seqno != c1_pkt.seqno) {
					// this should not occur in the give protocol
					printf("Channel 1: ACK received for incorrect packet\n");
				}

				if (feof(fp) != 0) c1_stop = true;
				else {
					// create new packet and send
					char c1_msg[PACKET_SIZE+1];
					nread = fread(c1_msg, 1, PACKET_SIZE, fp);
					c1_msg[nread] = 0;
					c1_pkt = create_new_packet(strlen(c1_msg)*sizeof(char), bytes_sent, nread != PACKET_SIZE, false, 1, c1_msg);
					bytes_sent += nread;

					write(c1_sockfd, &c1_pkt, MAX_PACKET_SIZE+1);
					print_packet(c1_pkt, "SENT");

					// reset channel 1 timer
					c1_start_time = clock();
				}
			}
		}
		
	}

	fclose(fp);
	close(c0_sockfd);
	close(c1_sockfd);
}
