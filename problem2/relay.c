#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>
#include <poll.h>
#include <time.h>

#include "common.h"

int main (int argc, char *argv[]) {
	if (argc != 2 || (strcmp(argv[1], "odd") != 0 && strcmp(argv[1], "even"))) {
		printf("Incorrect format of running relay program.\n");
		printf("Expected:- ./relay odd OR ./relay even\n");
		return 1;
	}

	srand(time(0)); // initialize random num generation


	/** RELAY AS "CLIENT" FOR SERVER **/

	int server_fd; // server socket
	struct sockaddr_in serv_addr;

	// Socket creation for channel 0
	server_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (server_fd < 0) {
		perror("ERROR in relay client socket creation (for connection with server)");
		return 1;
	}

	// Setting server address
	memset(&serv_addr, '0', sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(SERVER_PORT);
	serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

	// Establish connection main server 
	if (connect(server_fd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
		perror("Error in relay connect() with server");
		return 1;
	}


	/** RELAY AS "SERVER" FOR CLIENT **/

	int listenfd; // listening socket
	int client_fd; // connection socket
	struct sockaddr_in relay_serv_addr;

	// listening socket
	listenfd = socket(AF_INET, SOCK_STREAM, 0);
	if (listenfd < 0) {
		perror("ERROR in relay server listening socket creation");
		return 1;
	}

	// Setting relay server addr
	memset(&relay_serv_addr, '0', sizeof(relay_serv_addr));
	relay_serv_addr.sin_family = AF_INET;
	relay_serv_addr.sin_port = htons((strcmp(argv[1], "even") == 0) ? EVEN_RELAY_PORT : ODD_RELAY_PORT );
	relay_serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);

	// bindi listening socket and server addr
	if (bind(listenfd, (struct sockaddr *) &relay_serv_addr, sizeof(relay_serv_addr)) < 0) {
		perror("Error in binding server address");
		return 1;
	}

	// start listening for connections
	if (listen(listenfd, MAX_LISTEN_QUEUE) < 0) {
		perror("Error in starting to listen for connections");
		return 1;
	}

	// Accept client connection
	client_fd = accept(listenfd, (struct sockaddr *) NULL, NULL);
	if (client_fd < 0) {
		perror("Error in accepting client connection");
		return 1;
	}


	/** NEED TO LISTEN FOR MSGS FROM BOTH SERVER & CLIENT **/

	// pollfd struct channel 0
	struct pollfd client_pollfd = (struct pollfd) {
		.fd = client_fd,
		.events = POLLIN
	};

	// pollfd struct channel 1
	struct pollfd server_pollfd = (struct pollfd) {
		.fd = server_fd,
		.events = POLLIN
	};

	bool client_closed = false, server_closed = false;
	// exit when either one of client or server socket is closed
	while (!client_closed && !server_closed) {
		struct pollfd c_pollfds[2] = {client_pollfd, server_pollfd};

		int nready = poll(c_pollfds, 2, -1);
		if (nready <= 0) continue; // unexpected return of poll

		if (c_pollfds[0].revents == POLLIN) {
			// client fd became readable

			PACKET rcv;
			int n = read(client_fd, &rcv, MAX_PACKET_SIZE);
			if (n != 0) { 
				// socket not closed

				if (!should_drop()) {
					// packet not dropped

					print_packet(rcv, "RCVD from client");

					mdelay(rand_range(DELAY_MIN, DELAY_MAX));

					PACKET pkt = create_new_packet(rcv.size, rcv.seqno, rcv.is_last, rcv.is_ack, rcv.channel_id, rcv.payload);
					write(server_fd, &pkt, MAX_PACKET_SIZE);
					print_packet(pkt, "SENT to server");
				}
				else {
					printf("client packet dropped\n");
				}
			}
			else {
				client_closed = true;
			}
		}

		if (c_pollfds[1].revents == POLLIN) {
			// server fd became readable

			PACKET rcv;
			int n = read(server_fd, &rcv, MAX_PACKET_SIZE);
			if (n != 0) {
				// socket not closed unexpectedly

				// packets from server are not dropped
				print_packet(rcv, "RCVD from server");

				mdelay(rand_range(DELAY_MIN, DELAY_MAX));

				PACKET pkt = create_new_packet(rcv.size, rcv.seqno, rcv.is_last, rcv.is_ack, rcv.channel_id, rcv.payload);
				write(client_fd, &pkt, MAX_PACKET_SIZE);
				print_packet(pkt, "SENT to client");
			} 
			else {
				server_closed = true;
			}
		}
	}

	close(client_fd);
	close(server_fd);
	close(listenfd);
	return 0;
}
