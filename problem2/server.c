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
	srand(time(0)); // initialize random num generation

	int listenfd; // listening socket
	int c0_connfd, c1_connfd; // channel 0, 1 connection sockets
	struct sockaddr_in serv_addr;

	// listening socket
	listenfd = socket(AF_INET, SOCK_STREAM, 0);
	if (listenfd < 0) {
		perror("ERROR in server listening socket creation");
		return 1;
	}

	// Setting server addr
	memset(&serv_addr, '0', sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(SERVER_PORT);
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);

	// bindi listening socket and server addr
	if (bind(listenfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
		perror("Error in binding server address");
		return 1;
	}

	// start listening for connections
	if (listen(listenfd, MAX_LISTEN_QUEUE) < 0) {
		perror("Error in starting to listen for connections");
		return 1;
	}

	// Accept channel 0
	c0_connfd = accept(listenfd, (struct sockaddr *) NULL, NULL);
	if (c0_connfd < 0) {
		perror("Error in accepting channel 0 connection");
		return 1;
	}

	// Accept channel 1
	c1_connfd = accept(listenfd, (struct sockaddr *) NULL, NULL);
	if (c1_connfd < 0) {
		perror("Error in accepting channel 1 connection");
		return 1;
	}

	// output file
	FILE *fp = fopen(OUTPUT_FILE, "w");

	// pollfd struct channel 0
	struct pollfd c0_pollfd = (struct pollfd) {
		.fd = c0_connfd,
		.events = POLLIN
	};

	// pollfd struct channel 1
	struct pollfd c1_pollfd = (struct pollfd) {
		.fd = c1_connfd,
		.events = POLLIN
	};

	bool c0_closed = false, c1_closed = true;
	while (!c0_closed || !c1_closed) {
		struct pollfd c_pollfds[2] = {c0_pollfd, c1_pollfd};

		int nready = poll(c_pollfds, 2, -1);
		if (nready <= 0) continue; // unexpected return of poll

		if (c_pollfds[0].revents == POLLIN) {
			// channel 0 fd became readable

			PACKET rcv;
			int n = read(c0_connfd, &rcv, MAX_PACKET_SIZE+1);
			if (n != 0) { 
				// socket not closed unexpectedly

				print_packet(rcv, "RCVD");

				// write packet's data to file
				// at appropriate offset
				/*
				 *fseek(fp, rcv.seqno, SEEK_SET);
				 *fwrite(rcv.payload, 1, rcv.size, fp);
				 */

				// send ACK
				PACKET pkt = create_new_packet(4, rcv.seqno, rcv.is_last, true, rcv.channel_id, "ACK");
				write(c0_connfd, &pkt, MAX_PACKET_SIZE);
				print_packet(pkt, "SENT");
			}
			else {
				c0_closed = true;
			}
		}

		if (c_pollfds[1].revents == POLLIN) {
			// channel 1 fd became readable

			PACKET rcv;
			int n = read(c1_connfd, &rcv, MAX_PACKET_SIZE+1);
			if (n != 0) {
				// socket not closed unexpectedly

				print_packet(rcv, "RCVD");

				// write packet's data to file
				// at appropriate offset
				fseek(fp, rcv.seqno, SEEK_SET);
				fwrite(rcv.payload, 1, rcv.size, fp);

				// send ACK
				PACKET pkt = create_new_packet(4, rcv.seqno, rcv.is_last, true, rcv.channel_id, "ACK");
				write(c1_connfd, &pkt, MAX_PACKET_SIZE);
				print_packet(pkt, "SENT");
			} 
			else {
				c1_closed = true;
			}
		}
	}

	fclose(fp);
	close(c0_connfd);
	close(c1_connfd);
	close(listenfd);
}
