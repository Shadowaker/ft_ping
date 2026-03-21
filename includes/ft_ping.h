#ifndef FT_PING_H
# define FT_PING_H

# include <stdio.h>
# include <stdlib.h>
# include <string.h>
# include <unistd.h>
# include <sys/types.h>
# include <sys/socket.h>
# include <sys/time.h>
# include <netinet/ip_icmp.h> // https://manpages.opensuse.org/Leap-15.6/man-pages-posix/netinet_in.h.0p.en.html
# include <arpa/inet.h>
# include <netdb.h>
# include <errno.h>

# define PACKET_SIZE 64
# define TIMEOUT_SEC 2

typedef struct s_flags {
	int     verbose;
}				t_flags;


unsigned short checksum(void *b, int len);

#endif //FT_PING_H
