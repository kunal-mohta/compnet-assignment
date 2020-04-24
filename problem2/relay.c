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
	int saddr_size = sizeof(serv_addr);

	// Socket creation for communication with server
	server_fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (server_fd < 0) {
		perror("ERROR in relay-server socket creation");
		return 1;
	}

	// Setting server address
	memset(&serv_addr, '0', sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(SERVER_PORT);
	serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

	// Inform server about relay addr
	sendto(server_fd, SYNC_MSG, SYNC_LEN*sizeof(char), 0, (struct sockaddr *) &serv_addr, saddr_size);


	/** RELAY AS "SERVER" FOR CLIENT **/
	int client_fd; // client socket
	struct sockaddr_in relay_serv_addr;

	// Socket creation for communication with client 
	client_fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (client_fd < 0) {
		perror("ERROR in relay-client socket creation");
		return 1;
	}

	// Setting relay server addr
	memset(&relay_serv_addr, '0', sizeof(relay_serv_addr));
	relay_serv_addr.sin_family = AF_INET;
	relay_serv_addr.sin_port = htons((strcmp(argv[1], "even") == 0) ? EVEN_RELAY_PORT : ODD_RELAY_PORT );
	relay_serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);

	// bindi listening socket and server addr
	if (bind(client_fd, (struct sockaddr *) &relay_serv_addr, sizeof(relay_serv_addr)) < 0) {
		perror("Error in binding server address");
		return 1;
	}

	// get client addr
	struct sockaddr_in client_addr;
	int caddr_size = sizeof(client_addr);
	char client_sync[SYNC_LEN];
	if (recvfrom(client_fd, client_sync, SYNC_LEN*sizeof(char), 0, (struct sockaddr *) &client_addr, &caddr_size) == -1) {
		perror("Error in syncing with client");
		return 1;
	}

	/** NEED TO LISTEN FOR MSGS FROM BOTH SERVER & CLIENT **/

	bool client_closed = false, server_closed = false;
	// exit when either one of client or server socket is closed
	while (!client_closed && !server_closed) {

		PACKET rcv;
		int ret;

		memset(&rcv, 0, sizeof(rcv));
		caddr_size = sizeof(client_addr);
		ret = recvfrom(client_fd, &rcv, MAX_PACKET_SIZE, MSG_DONTWAIT, (struct sockaddr *) &client_addr, &caddr_size);
		if (ret != -1) {
			// msg received from client

			if (ret != 0) { 
				// socket not closed

				/*if (!should_drop()) {*/
					// packet not dropped

					print_packet(rcv, "RCVD from client");

					/*mdelay(rand_range(DELAY_MIN, DELAY_MAX));*/

					PACKET pkt = create_new_packet(rcv.size, rcv.seqno, rcv.is_last, rcv.is_ack, rcv.channel_id, rcv.payload);
					sendto(server_fd, &pkt, MAX_PACKET_SIZE, 0, (struct sockaddr *) &serv_addr, saddr_size);
					print_packet(pkt, "SENT to server");
				/*
				 *}
				 *else {
				 *    printf("client packet dropped\n");
				 *}
				 */
			}
			else {
				client_closed = true;
			}
		}

		memset(&rcv, 0, sizeof(rcv));
		saddr_size = sizeof(serv_addr);
		ret = recvfrom(server_fd, &rcv, MAX_PACKET_SIZE, MSG_DONTWAIT, (struct sockaddr *) &serv_addr, &saddr_size);
		if (ret != -1) {
			// msg received from server 

			if (ret != 0) {
				// socket not closed 

				// packets from server are not dropped
				print_packet(rcv, "RCVD from server");

				mdelay(rand_range(DELAY_MIN, DELAY_MAX));

				PACKET pkt = create_new_packet(rcv.size, rcv.seqno, rcv.is_last, rcv.is_ack, rcv.channel_id, rcv.payload);
				sendto(client_fd, &pkt, MAX_PACKET_SIZE, 0, (struct sockaddr *) &client_addr, caddr_size);
				print_packet(pkt, "SENT to client");
			} 
			else {
				server_closed = true;
			}
		}
	}
	// to inform server, client to stop
	if (!server_closed) {
		sendto(server_fd, "", 0, 0, (struct sockaddr *) &serv_addr, sizeof(serv_addr));
	}
	if (!client_closed) {
		sendto(client_fd, "", 0, 0, (struct sockaddr *) &client_addr, sizeof(client_addr));
	}

	close(client_fd);
	close(server_fd);
	return 0;
}
