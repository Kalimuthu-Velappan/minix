/*
ping.c
*/

#define DEBUG	1

#include <sys/types.h>
#include <errno.h>
#include <signal.h>
#include <net/gen/netdb.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <net/gen/oneCsum.h>
#include <fcntl.h>
#include <net/gen/in.h>
#include <net/gen/inet.h>
#include <net/gen/ip_hdr.h>
#include <net/gen/icmp_hdr.h>
#include <net/gen/ip_io.h>

#define WRITE_SIZE 30
char buffer[16*1024];

int main(int argc, char *argv[]);

#if DEBUG
#define where() fprintf(stderr, "%s %d:", __FILE__, __LINE__);
#endif

#if __STDC__
#define PROTO(x,y) x y
#else
#define PROTO(x,y) X ()
#endif

PROTO (int main, (int argc, char *argv[]) );
static PROTO (void sig_hand, (int signal) );

main(argc, argv)
int argc;
char *argv[];
{
	int fd, i;
	int result;
	nwio_ipopt_t ipopt;
	ip_hdr_t *ip_hdr;
	icmp_hdr_t *icmp_hdr;
	ipaddr_t dst_addr;
	struct hostent *hostent;
	int length;
	char *dev_name = "/dev/ip";
	char *host_name;

	if (argc<2 || (argc > 3 && strcmp(argv[1], "-I")) || argc>5)
	{
		fprintf(stderr, "Usage: %s [-I /dev/ip<n>]  hostname [length]\n", argv[0]);
		exit(1);
	}
	if (argc > 3) {
		host_name = argv[3];
		dev_name = argv[2];
	} else {
		host_name = argv[1];
	}
	hostent= gethostbyname(host_name);
	if (!hostent)
	{
		dst_addr= inet_addr(host_name);
		if (dst_addr == -1)
		{
			fprintf(stderr, "%s: unknown host (%s)\n",
				argv[0], host_name);
			exit(1);
		}
	}
	else
		dst_addr= *(ipaddr_t *)(hostent->h_addr);
	if (argc == 3 || argc == 5)
	{
		length= strtol (argv[argc - 1], (char **)0, 0);
		if (length< sizeof(icmp_hdr_t) + IP_MIN_HDR_SIZE)
		{
			fprintf(stderr, "%s: length too small (%s)\n",
				argv[0], argv[argc - 1]);
			exit(1);
		}
	}
	else
		length= WRITE_SIZE;

	fd= open (dev_name, O_RDWR);
	if (fd<0)
		perror("open"), exit(1);

	ipopt.nwio_flags= NWIO_COPY | NWIO_PROTOSPEC;
	ipopt.nwio_proto= 1;

	result= ioctl (fd, NWIOSIPOPT, &ipopt);
	if (result<0)
		perror("ioctl (NWIOSIPOPT)"), exit(1);

	result= ioctl (fd, NWIOGIPOPT, &ipopt);
	if (result<0)
		perror("ioctl (NWIOGIPOPT)"), exit(1);

	for (i= 0; i< 20; i++)
	{
		ip_hdr= (ip_hdr_t *)buffer;
		ip_hdr->ih_dst= dst_addr;

		icmp_hdr= (icmp_hdr_t *)(buffer+20);
		icmp_hdr->ih_type= 8;
		icmp_hdr->ih_code= 0;
		icmp_hdr->ih_chksum= 0;
		icmp_hdr->ih_chksum= ~oneC_sum(0, (u16_t *)icmp_hdr,
			WRITE_SIZE-20);
		result= write(fd, buffer, length);
		if (result<0)
		{
			perror("write");
			exit(1);
		}
		if (result != length)
		{
			where();
			fprintf(stderr, "result= %d\n", result);
			exit(1);
		}

		alarm(0);
		signal (SIGALRM, sig_hand);
		alarm(1);

		result= read(fd, buffer, sizeof(buffer));
		if (result>= 0 || errno != EINTR)
			break;
	}
	if (i >= 20)
	{
		printf("no answer from %s\n", host_name);
		exit(1);
	}
	if (result<0)
	{
		perror ("read");
		exit(1);
	}
	printf("%s is alive\n", host_name);
	exit(0);
}

static void sig_hand(signal)
int signal;
{
}
