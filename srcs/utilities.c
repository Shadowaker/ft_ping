#include "ft_ping.h"

static t_stats *s_stats;

void stats_init(t_stats *stats) {
	s_stats = stats;
}

void sigint_handler(int sig) {
	(void)sig; // signal() must use the int parameter
	struct timeval now;
	gettimeofday(&now, NULL);
	long elapsed_ms = (now.tv_sec - s_stats->start.tv_sec) * 1000
		+ (now.tv_usec - s_stats->start.tv_usec) / 1000;
	double loss = s_stats->sent
		? 100.0 * (s_stats->sent - s_stats->received) / s_stats->sent
		: 0.0;
	printf("\n--- %s ping statistics ---\n", s_stats->host);
	printf("%d packets transmitted, %d received, %.0f%% packet loss, time %ldms\n",
		s_stats->sent, s_stats->received, loss, elapsed_ms);

	if (s_stats->received > 0) {
		double avg = s_stats->rtt_sum / s_stats->received;
		double stddev = sqrt(s_stats->rtt_sum_sq / s_stats->received - avg * avg);
		printf("rtt min/avg/max/stddev = %.3f/%.3f/%.3f/%.3f ms\n",
			s_stats->rtt_min, avg, s_stats->rtt_max, stddev);
	}
	exit(EXIT_SUCCESS);
}

// Calculate checksum
unsigned short checksum(void *b, int len) {
    unsigned short *buf = b;
    unsigned int sum = 0;
    for (; len > 1; len -= 2)
        sum += *buf++;
    if (len == 1)
        sum += *(unsigned char*)buf;
    sum = (sum >> 16) + (sum & 0xffff);
    sum += (sum >> 16);
    return ~sum;
}