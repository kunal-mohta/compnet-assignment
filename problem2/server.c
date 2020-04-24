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
	int listenfd; // listening socket
	int r1_connfd, r2_connfd; // relay 1,2 connection sockets
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

	// bind listening socket and server addr
	if (bind(listenfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
		perror("Error in binding server address");
		return 1;
	}

	// start listening for connections
	if (listen(listenfd, MAX_LISTEN_QUEUE) < 0) {
		perror("Error in starting to listen for connections");
		return 1;
	}

	// Accept relay 1 
	r1_connfd = accept(listenfd, (struct sockaddr *) NULL, NULL);
	if (r1_connfd < 0) {
		perror("Error in accepting relay 1 connection");
		return 1;
	}

	// Accept relay 2 
	r2_connfd = accept(listenfd, (struct sockaddr *) NULL, NULL);
	if (r2_connfd < 0) {
		perror("Error in accepting relay 2 connection");
		return 1;
	}

	// output file
	FILE *fp = fopen(OUTPUT_FILE, "w");

	// pollfd struct channel 0
	struct pollfd r1_pollfd = (struct pollfd) {
		.fd = r1_connfd,
		.events = POLLIN
	};

	// pollfd struct channel 1
	struct pollfd r2_pollfd = (struct pollfd) {
		.fd = r2_connfd,
		.events = POLLIN
	};

	bool r1_closed = false, r2_closed = true;
	while (!r1_closed || !r2_closed) {
		struct pollfd r_pollfds[2] = {r1_pollfd, r2_pollfd};

		int nready = poll(r_pollfds, 2, -1);
		if (nready <= 0) continue; // unexpected return of poll

		if (r_pollfds[0].revents == POLLIN) {
			// channel 0 fd became readable

			PACKET rcv;
			int n = read(r1_connfd, &rcv, MAX_PACKET_SIZE);
			if (n != 0) { 
				// socket not closed unexpectedly

				print_packet(rcv, "RCVD from relay 1");

				// write packet's data to file
				// at appropriate offset
				/*
				 *fseek(fp, rcv.seqno, SEEK_SET);
				 *fwrite(rcv.payload, 1, rcv.size, fp);
				 */

				// send ACK
				PACKET pkt = create_new_packet(4, rcv.seqno, rcv.is_last, true, rcv.channel_id, "ACK");
				write(r1_connfd, &pkt, MAX_PACKET_SIZE);
				print_packet(pkt, "SENT to relay 1");
			}
			else {
				r1_closed = true;
			}
		}

		if (r_pollfds[1].revents == POLLIN) {
			// channel 1 fd became readable

			PACKET rcv;
			int n = read(r2_connfd, &rcv, MAX_PACKET_SIZE);
			if (n != 0) {
				// socket not closed unexpectedly

				print_packet(rcv, "RCVD from relay 2");

				// write packet's data to file
				// at appropriate offset
				fseek(fp, rcv.seqno, SEEK_SET);
				fwrite(rcv.payload, 1, rcv.size, fp);

				// send ACK
				PACKET pkt = create_new_packet(4, rcv.seqno, rcv.is_last, true, rcv.channel_id, "ACK");
				write(r2_connfd, &pkt, MAX_PACKET_SIZE);
				print_packet(pkt, "SENT to relay 2");
			} 
			else {
				r2_closed = true;
			}
		}
	}

	fclose(fp);
	close(r1_connfd);
	close(r2_connfd);
	close(listenfd);
}
