#include "../includes/ft_ping.h"

// Help function
void usage(const char *progname) {
	printf("Usage: %s [-v] [-?] [-c count] [-s size] [-W timeout] [-w deadline] <destination>\n", progname);
	printf("Options:\n");
	printf("  -v             Verbose output\n");
	printf("  -c count       Stop after sending count packets\n");
	printf("  -s size        Set ICMP data size in bytes (default: %d).\n", DEFAULT_DATA_SIZE);
	printf("  -D             Print timestamp (unix time + microseconds as in gettimeofday) before each line.\n");
	printf("  -W timeout     Set time to wait for a response, in seconds (default: %d).\n", TIMEOUT_SEC);
	printf("  -w deadline    Exit after deadline seconds regardless of packets sent.\n");
	printf("  -?             Show this help message\n");
	exit(EXIT_SUCCESS);
}

int send_loop(int *sockfd, struct sockaddr_in addr, t_flags *flags, t_stats *stats) {
	size_t packet_size = ICMP_MINLEN + flags->size;
	char *packet = malloc(packet_size);
	if (!packet) {
		perror("malloc");
		close(*sockfd);
		exit(EXIT_FAILURE);
	}
	char recvbuf[IP_MAXPACKET];
	int seq = 0;
	struct timeval start, end;

	while (1) {

		memset(packet, 0, packet_size);
		((struct icmp *)packet)->icmp_type = ICMP_ECHO;
		((struct icmp *)packet)->icmp_code = 0;
		((struct icmp *)packet)->icmp_id = getpid() & 0xFFFF;
		((struct icmp *)packet)->icmp_seq = seq++;
		for (size_t i = ICMP_MINLEN; i < packet_size; i++)
			packet[i] = '0' + (i % 10);
		((struct icmp *)packet)->icmp_cksum = checksum(packet, packet_size);

		stats->sent++;
		gettimeofday(&start, NULL);
		if (sendto(*sockfd, packet, packet_size, 0, (struct sockaddr *)&addr, sizeof(addr)) <= 0) {
			perror("sendto");
			close(*sockfd);
			exit(EXIT_FAILURE);
		}

		// Set timeout
		struct timeval tv;
		tv.tv_sec = flags->wait ? flags->wait : TIMEOUT_SEC;
		tv.tv_usec = 0;

		fd_set readfds;
		FD_ZERO(&readfds);
		FD_SET(*sockfd, &readfds);

		unsigned int got_reply = 0; // considering moving to bool type
		while (!got_reply) {
			int ret = select(*sockfd + 1, &readfds, NULL, NULL, &tv);

			if (ret == -1) {
				perror("select");
				close(*sockfd);
				exit(EXIT_FAILURE);
			}

			if (ret == 0) {
				break;
			}

			struct sockaddr_in from;
			socklen_t addrlen = sizeof(from);
			ssize_t recv_bytes = recvfrom(*sockfd, recvbuf, sizeof(recvbuf), 0, (struct sockaddr *)&from, &addrlen);
			gettimeofday(&end, NULL);

			if (recv_bytes <= 0) {
				perror("recvfrom");
				close(*sockfd);
				exit(EXIT_FAILURE);
			}

			struct ip *ip_hdr = (struct ip *)recvbuf;
			int ip_hdr_len = ip_hdr->ip_hl * 4;
			struct icmp *icmp_reply = (struct icmp *)(recvbuf + ip_hdr_len);

			if (icmp_reply->icmp_type != ICMP_ECHOREPLY
				|| icmp_reply->icmp_id != (getpid() & 0xFFFF)
				|| from.sin_addr.s_addr != addr.sin_addr.s_addr)
				continue;

			got_reply = 1;
			double rtt = (end.tv_sec - start.tv_sec) * 1000.0
						+ (end.tv_usec - start.tv_usec) / 1000.0;
			stats->received++;

			if (stats->rtt_min < 0 || rtt < stats->rtt_min) stats->rtt_min = rtt;
			if (rtt > stats->rtt_max) stats->rtt_max = rtt;

			stats->rtt_sum += rtt;
			stats->rtt_sum_sq += rtt * rtt;
			ssize_t icmp_bytes = recv_bytes - ip_hdr_len;

			if (flags->timestamp)
				printf("[%ld.%06ld] ", end.tv_sec, (long)end.tv_usec);
			if (flags->verbose)
				printf("%zd bytes from %s: icmp_seq=%d ttl=%d time=%.3f ms (type=%d code=%d)\n",
					icmp_bytes, inet_ntoa(from.sin_addr),
					icmp_reply->icmp_seq, ip_hdr->ip_ttl, rtt,
					icmp_reply->icmp_type, icmp_reply->icmp_code);
			else
				printf("%zd bytes from %s: icmp_seq=%d ttl=%d time=%.3f ms\n",
					icmp_bytes, inet_ntoa(from.sin_addr),
					icmp_reply->icmp_seq, ip_hdr->ip_ttl, rtt);
		}
		sleep(1);

		// KILL IT
		if (flags->count > 0 && seq >= flags->count) {
			free(packet);
			sigint_handler(0);
		}
	}

	free(packet);
	return 0;
}

int main(int argc, char *argv[]) {
	int sockfd;
	struct sockaddr_in addr;
	t_flags flags;
	t_stats stats = {0, 0, -1.0, 0.0, 0.0, 0.0, NULL, {0}};

	int opt;

	memset(&flags, 0, sizeof(flags));
	flags.size = DEFAULT_DATA_SIZE;
	while ((opt = getopt(argc, argv, "v?c:s:DW:w:")) != -1) {
		switch (opt) {
			case 'v':
				flags.verbose = 1;
				break;
			case 'c':
				flags.count = atoi(optarg);
				break;
			case 's':
				flags.size = atoi(optarg);
				break;
			case 'D':
				flags.timestamp = 1;
				break;
			case 'W':
				flags.wait = atoi(optarg);
				break;
			case 'w':
				flags.deadline = atoi(optarg);
				break;
			case '?':
				if (optopt == 0 || optopt == '?')
					usage(argv[0]);
				fprintf(stderr, "%s: invalid option -- '%c'\n", argv[0], optopt);
				exit(EXIT_FAILURE);
			default:
				usage(argv[0]);
		}
	}

	if (optind >= argc) {
		fprintf(stderr, "Destination address required.\n");
		usage(argv[0]);
	}

	const char *dest = argv[optind];

	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	if (inet_pton(AF_INET, dest, &addr.sin_addr) <= 0) {
		struct hostent *host = gethostbyname(dest);
		if (!host) {
			perror("gethostbyname");
			exit(EXIT_FAILURE);
		}
		addr.sin_addr = *(struct in_addr *)host->h_addr;
	}

	sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
	if (sockfd < 0) {
		perror("socket");
		exit(EXIT_FAILURE);
	}

	stats.host = dest;
	stats_init(&stats);
	signal(SIGINT, sigint_handler);
	signal(SIGALRM, sigint_handler);
	if (flags.deadline > 0)
		alarm(flags.deadline);
	printf("PING %s (%s): %d data bytes\n", dest, inet_ntoa(addr.sin_addr), flags.size);
	gettimeofday(&stats.start, NULL);
	send_loop(&sockfd, addr, &flags, &stats);

	close(sockfd);
	return 0;
}