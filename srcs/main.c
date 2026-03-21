#include "ft_ping.h"

// Help function
void usage(const char *progname) {
    printf("Usage: %s [-v] [-?] <destination>\n", progname);
    printf("Options:\n");
    printf("  -v   Verbose output\n");
    printf("  -?   Show this help message\n");
    exit(EXIT_SUCCESS);
}

int send_loop(int *sockfd, struct sockaddr_in addr, const char *dest, t_flags *flags) {
	struct icmp packet;
	char recvbuf[IP_MAXPACKET];
	int seq = 0;

	while (1) {

		memset(&packet, 0, sizeof(packet));
    	packet.icmp_type = ICMP_ECHO;
    	packet.icmp_code = 0;
    	packet.icmp_id = getpid() & 0xFFFF;
    	packet.icmp_seq = seq++;
    	packet.icmp_cksum = checksum(&packet, sizeof(packet));

		if (sendto(*sockfd, &packet, sizeof(packet), 0, (struct sockaddr *)&addr, sizeof(addr)) <= 0) {
        	perror("sendto");
        	close(*sockfd);
        	exit(EXIT_FAILURE);
    	}

    	if (flags->verbose)
        	printf("Sent ICMP Echo Request to %s\n", dest);

	    // Set timeout
	    struct timeval tv;
	    tv.tv_sec = TIMEOUT_SEC;
	    tv.tv_usec = 0;

	    fd_set readfds;
	    FD_ZERO(&readfds);
	    FD_SET(*sockfd, &readfds);

	    int ret = select(*sockfd + 1, &readfds, NULL, NULL, &tv);
	    if (ret == -1) {
	        perror("select");
	        close(*sockfd);
	        exit(EXIT_FAILURE);
	    }
	    else if (ret == 0) {
	        printf("Request timeout for icmp_seq %d\n", seq - 1);
	        sleep(1);
	        continue;
	    }
		socklen_t addrlen = sizeof(addr);
		ssize_t recv_bytes = recvfrom(*sockfd, recvbuf, sizeof(recvbuf), 0, (struct sockaddr *)&addr, &addrlen);
		if (recv_bytes <= 0) {
			perror("recvfrom");
			close(*sockfd);
			exit(EXIT_FAILURE);
		}
		struct ip *ip_hdr = (struct ip *)recvbuf;
		int ip_hdr_len = ip_hdr->ip_hl * 4;
		struct icmp *icmp_reply = (struct icmp *)(recvbuf + ip_hdr_len);
		if (flags->verbose)
			printf("Received ICMP type=%d code=%d from %s\n",
				icmp_reply->icmp_type, icmp_reply->icmp_code, inet_ntoa(addr.sin_addr));
		else
			printf("Reply from %s: icmp_seq=%d\n", inet_ntoa(addr.sin_addr), icmp_reply->icmp_seq);
		sleep(1);
	}

	return 0;
}

int main(int argc, char *argv[]) {
    int sockfd;
    struct sockaddr_in addr;
	
	t_flags flags;

    int opt;

	memset(&flags, 0, sizeof(flags));
    while ((opt = getopt(argc, argv, "v?")) != -1) {
        switch (opt) {
            case 'v':
                flags.verbose = 1;
                break;
            case '?':
                usage(argv[0]);
                break;
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

	send_loop(&sockfd, addr, dest, &flags);

    close(sockfd);
    return 0;
}
