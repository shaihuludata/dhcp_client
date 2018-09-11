#include <stdio.h> //printf
#include <string.h> //memset
#include <sys/socket.h>
#include <stdlib.h>
#include <errno.h>
#include <netinet/udp.h>   //Provides declarations for udp header
//#include<netinet/ip.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "options.h"
#include "udp_raw.h"

int send_raw_udp_message(int data_len, char * data, char source_ip[32], char destination_ip[32])
{
	//data = (char *) &data[data_len];
	int ret;
	int sfd = socket(AF_INET, SOCK_RAW, IPPROTO_UDP);
	if (sfd == -1)
	{
		//socket creation failed, may be because of non-root privileges
		perror("Failed to create raw socket");
		exit(1);
	}
	else
	{
		//int setsockopt(int sockfd, int level, int optname, const void *optval, socklen_t optlen);
		int optval = 1;
		int opt = setsockopt(sfd, SOL_SOCKET, SO_BROADCAST, (char *) &(optval), sizeof(int));
		printf("%d\n", opt);
	}

	int d_size = sizeof(data)+8;
	char datagram[d_size]; //, *pseudogram;
	memset (datagram, 0, d_size);
	struct udphdr * udph = (struct udphdr *) (datagram); // + sizeof (struct ip));
	strcpy(datagram+8, data); //, (size_t) sizeof(data));

	struct sockaddr_in sin;
	//struct pseudo_header psh;

	sin.sin_family = AF_INET;
	sin.sin_port = htons(PORT_CLI);
	sin.sin_addr.s_addr = inet_addr (destination_ip);

	udph->source = htons (PORT_CLI);
	udph->dest = htons (PORT_SRV);
	udph->len = htons(sizeof(datagram)); //8 - udp header size
	udph->check = 0; //leave checksum 0 now, can be filled later by pseudo header
	ret = sendto(sfd, datagram, sizeof(datagram),  0, (struct sockaddr *) &sin, sizeof (sin));
	return ret;
}
