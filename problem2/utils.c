#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#include "common.h"

PACKET create_new_packet (int size, int seqno, bool is_last, bool is_ack, char *payload) {
	PACKET pkt;
	pkt = (PACKET) {
		.size = size,
		.seqno = seqno,
		.is_last = is_last,
		.is_ack = is_ack,
	};
	memset(pkt.payload, 0, PACKET_SIZE);
	memcpy(pkt.payload, payload, PACKET_SIZE);
	return pkt;
}

char *generate_timestamp () {
	struct timespec t1;
	if (clock_gettime(CLOCK_REALTIME, &t1) == -1) {
		perror("Error generating timestamp");
		exit(1);
	}
	struct tm *t2 = localtime(&t1.tv_sec); 
	char *ret = malloc(16*sizeof(char));
	memset(ret, 0, 16*sizeof(char));
	sprintf(ret, "%02d:%02d:%02d.%06ld", t2->tm_hour, t2->tm_min, t2->tm_sec, t1.tv_nsec/1000);
	return ret;
}

void print_packet (PACKET pkt, char *node_name, char *event, char *src, char *dst) {
	char *time_stamp = generate_timestamp();
	if (pkt.is_ack) {
		printf("\n%15s %10s %25s %10s %10d %15s %15s\n", node_name, event, time_stamp, "ACK", pkt.seqno, src, dst);
	}
	else {
		printf("\n%15s %10s %25s %10s %10d %15s %15s\n", node_name, event, time_stamp, "DATA", pkt.seqno, src, dst);
	}
	free(time_stamp);
}

// decides whether to drop packet
bool should_drop () {
	double threshold = ((double)PDR)/100;
	double norm_rand = ((double)rand())/RAND_MAX;

	return norm_rand < threshold;
}

// gives random value in a range
double rand_range (double min, double max) {
	double scaled_rand = (((double)rand())/RAND_MAX)*(max-min);
	return min + scaled_rand;
}

// suspends program execution for specified time
// takes input in milliseconds, has precision of nanoseconds
void mdelay (double time_ms) {
	struct timespec delay_time;
	delay_time.tv_sec = time_ms/1000;
	delay_time.tv_nsec = (time_ms - (delay_time.tv_sec*1000)) * 1000000;
	/*printf("here %lf, sec %ld, nsec %ld\n", time_ms, delay_time.tv_sec, delay_time.tv_nsec);*/
	int x = nanosleep(&delay_time, &delay_time);
	if (x == -1) {
		perror("nanosleep");
	}
}
