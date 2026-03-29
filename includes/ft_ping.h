#ifndef FT_PING_H
# define FT_PING_H

# include <stdio.h>
# include <stdlib.h>
# include <string.h>
# include <unistd.h>
# include <sys/types.h>
# include <sys/socket.h>
# include <sys/time.h>
# include <netinet/ip.h>
# include <netinet/ip_icmp.h> // https://manpages.opensuse.org/Leap-15.6/man-pages-posix/netinet_in.h.0p.en.html
# include <arpa/inet.h>
# include <netdb.h>
# include <errno.h>
# include <signal.h>
# include <math.h>

# define PACKET_SIZE 64
# define TIMEOUT_SEC 2
# define BASE_STR "%zd bytes from %s: icmp_seq=%d ttl=%d time=%.3f ms"
# define NL	"\n"

typedef struct s_flags {
	int     verbose;
	int     count;
	int		timestamp;
	int		wait;
	int		deadline;
}				t_flags;

typedef struct s_stats {
	int             sent;
	int             received;
	double          rtt_min;
	double          rtt_max;
	double          rtt_sum;
	double          rtt_sum_sq;
	const char      *host;
	struct timeval  start;
}				t_stats;

unsigned short  checksum(void *b, int len);
void            stats_init(t_stats *stats);
void            sigint_handler(int sig);

#endif //FT_PING_H
