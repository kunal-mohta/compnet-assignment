#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <time.h>

#include "common.h"

void send_packet_to_relay (PACKET pkt, int relay1_fd, int relay2_fd) {

}

int main () {
	int relay1_fd, relay2_fd; // channel 0, 1 sockets
	int bytes_sent = 0; // file offset till which already sent
	int curr_seqno = 0;
	struct sockaddr_in relay1_addr, relay2_addr;

	// Socket creation for relay 1 
	relay1_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (relay1_fd < 0) {
		perror("ERROR in channel 0 socket creation");
		return 1;
	}

	// Socket creation for relay 2
	relay2_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (relay2_fd < 0) {
		perror("ERROR in channel 1 socket creation");
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
	if (connect(relay1_fd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
		perror("Error in relay 1 connect()");
		return 1;
	}

	// Establish connection channel 1
	if (connect(relay2_fd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
		perror("Error in relay 2 connect()");
		return 1;
	}

	FILE *fp = fopen(INPUT_FILE, "r");

	// First packet send, via relay 1 
	char r1_msg[PACKET_SIZE+1];
	int nread = fread(r1_msg, 1, PACKET_SIZE, fp);
	r1_msg[nread] = 0;

	PACKET r1_pkt = create_new_packet(strlen(r1_msg)*sizeof(char), curr_seqno, nread != PACKET_SIZE, false, 0, r1_msg); // if nread != PACKET_SIZE, then packet is last
	bytes_sent += nread;
	curr_seqno++;

	send_packet_to_relay(r1)
	write(relay1_fd, &r1_pkt, MAX_PACKET_SIZE+1);
	print_packet(r1_pkt, "SENT");

	// start timer for channel 0 pkt
	clock_t c0_start_time = clock();

	// making both sockets non-blocking
	int flags = fcntl(relay1_fd, F_GETFL, 0);
	fcntl(relay1_fd, F_SETFL, flags | O_NONBLOCK);
	flags = fcntl(relay2_fd, F_GETFL, 0);
	fcntl(relay2_fd, F_SETFL, flags | O_NONBLOCK);

	/*bool r1_stop = false, r2_stop = false;*/
	bool r1_stop = false, r2_stop = true;
	while (!r1_stop || !r2_stop) {
		PACKET rcv;

		if (!r1_stop) {
			// try reading from channel 0
			// only if eof not seen yet

			memset(&rcv, '0', sizeof(rcv));
			nread = read(relay1_fd, &rcv, MAX_PACKET_SIZE+1);
			if (nread == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
				clock_t curr_time = clock();

				double diff = (double)(curr_time - c0_start_time)/CLOCKS_PER_SEC;

				if (diff > PKT_TIMEOUT) {
					// retransmission
					
					write(relay1_fd, &r1_pkt, MAX_PACKET_SIZE+1);
					print_packet(r1_pkt, "SENT");

					// reset channel 0 timer
					c0_start_time = clock();
				}
			}
			else {
				print_packet(rcv, "RCVD");

				if (rcv.seqno != r1_pkt.seqno) {
					// this should not occur in the give protocol
					printf("Channel 0: ACK received for incorrect packet\n");
				}

				if (feof(fp) != 0) r1_stop = true;
				else {
					// create new packet and send
					char r1_msg[PACKET_SIZE+1];
					nread = fread(r1_msg, 1, PACKET_SIZE, fp);
					r1_msg[nread] = 0;
					r1_pkt = create_new_packet(strlen(r1_msg)*sizeof(char), bytes_sent, nread != PACKET_SIZE, false, 0, r1_msg);
					bytes_sent += nread;

					write(relay1_fd, &r1_pkt, MAX_PACKET_SIZE+1);
					print_packet(r1_pkt, "SENT");

					// reset channel 0 timer
					c0_start_time = clock();
				}
			}
		}
		

		if (!r2_stop) {
			// try reading from channel 1
			// only if eof not seen yet

			memset(&rcv, '0', sizeof(rcv));
			nread = read(relay2_fd, &rcv, MAX_PACKET_SIZE+1);
			if (nread == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
				clock_t curr_time = clock();

				double diff = (double)(curr_time - c1_start_time)/CLOCKS_PER_SEC;

				if (diff > PKT_TIMEOUT) {
					// retransmission
					
					write(relay2_fd, &r2_pkt, MAX_PACKET_SIZE+1);
					print_packet(r2_pkt, "SENT");

					// reset channel 1 timer
					c1_start_time = clock();
				}
			}
			else {
				print_packet(rcv, "RCVD");

				if (rcv.seqno != r2_pkt.seqno) {
					// this should not occur in the give protocol
					printf("Channel 1: ACK received for incorrect packet\n");
				}

				if (feof(fp) != 0) r2_stop = true;
				else {
					// create new packet and send
					char r2_msg[PACKET_SIZE+1];
					nread = fread(r2_msg, 1, PACKET_SIZE, fp);
					r2_msg[nread] = 0;
					r2_pkt = create_new_packet(strlen(r2_msg)*sizeof(char), bytes_sent, nread != PACKET_SIZE, false, 1, r2_msg);
					bytes_sent += nread;

					write(relay2_fd, &r2_pkt, MAX_PACKET_SIZE+1);
					print_packet(r2_pkt, "SENT");

					// reset channel 1 timer
					c1_start_time = clock();
				}
			}
		}
		
	}

	fclose(fp);
	close(relay1_fd);
	close(relay2_fd);
}
