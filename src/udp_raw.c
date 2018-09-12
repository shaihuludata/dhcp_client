#include <stdio.h> //printf
#include <string.h> //memset
#include <sys/socket.h>
#include <stdlib.h>
#include <errno.h>
#include <netinet/udp.h>   //Provides declarations for udp header
#include <netinet/ip.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <linux/if_packet.h>
#include <net/ethernet.h>

#include "options.h"
#include "udp_raw.h"

int send_raw_message(int data_len, char * data, char source_ip[32], char destination_ip[32])
{
	int ret;
	int sfd = socket(AF_INET, SOCK_RAW, IPPROTO_RAW);
	//int sfd = socket(AF_INET, SOCK_RAW, 0);
	//int sfd = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL));

	if (sfd == -1)
	{
		//socket creation failed, may be because of non-root privileges
		perror("Failed to create raw socket");
		exit(1);
	}
	else
	{
		int optval = 1;
		int opt = setsockopt(sfd, SOL_SOCKET, SO_BROADCAST, (char *) &(optval), sizeof(int));
		printf("%d\n", opt);

//		char *opt;
//		opt = "eth0";
//		setsockopt(sfd, SOL_SOCKET, SO_BINDTODEVICE, opt, 4);
	}

	char *pseudogram;
	char datagram[4096];
	struct iphdr * iph = (struct iphdr *) &datagram[0];
	struct udphdr * udph = (struct udphdr *) &datagram[20];
	int d_size = sizeof(data) + sizeof(udph) + sizeof(iph);

	memset (datagram, 0, d_size);
	strncpy(&datagram[sizeof(*udph) + sizeof(*iph)], data, sizeof(data)-1);

	struct sockaddr_in sin;
	struct pseudo_header psh;

	int psize = sizeof(struct pseudo_header) + sizeof(struct udphdr) + strlen(data);
	pseudogram = malloc(psize);
	memcpy(pseudogram, (char*) &psh, sizeof (struct pseudo_header));
	memcpy(pseudogram + sizeof(struct pseudo_header), udph, sizeof(struct udphdr) + strlen(data));

	sin.sin_family = AF_INET;
	sin.sin_port = htons(PORT_CLI);
	//sin.sin_addr.s_addr = inet_addr (destination_ip);
	inet_aton (destination_ip, &sin.sin_addr);

	iph->ihl = 5;
	iph->version = 4;
	iph->tos = 0;
	iph->tot_len = sizeof (*iph) + sizeof (*udph) + strlen(data);
	iph->id = htons (54321); //Id of this packet
	iph->frag_off = 0;
	iph->ttl = 1;
	iph->protocol = IPPROTO_UDP;
	iph->check = 0;      //Set to 0 before calculating checksum
	//iph->saddr = inet_addr (source_ip);    //Spoof the source ip address
	struct in_addr saddr;
	inet_aton (source_ip, &saddr);    //Spoof the source ip address
	iph->saddr = saddr.s_addr;
	iph->daddr = sin.sin_addr.s_addr;
	iph->check = csum ((unsigned short *) datagram, iph->tot_len);

	udph->source = htons (PORT_CLI);
	udph->dest = htons (PORT_SRV);
	udph->len = htons(8+strlen(data)); //8 - udp header size
	udph->check = 0;

	psh.source_address = inet_addr (source_ip);
	psh.dest_address = sin.sin_addr.s_addr;
	psh.placeholder = 0;
	psh.protocol = IPPROTO_UDP;
	psh.udp_length = htons(sizeof(struct udphdr) + strlen(data) );

	pseudogram = malloc(psize);

	memcpy(pseudogram , (char*) &psh , sizeof (struct pseudo_header));
	memcpy(pseudogram + sizeof(struct pseudo_header) , udph , sizeof(struct udphdr) + strlen(data));

	udph->check = htons(csum( (unsigned short*) pseudogram , psize));

	ret = sendto(sfd, datagram, iph->tot_len,  0, (struct sockaddr *) &sin, sizeof (sin));
	printf("%d\n", ret);
	close(sfd);
	return ret;
}

unsigned short csum(unsigned short *ptr,int nbytes)
{
    register long sum;
    unsigned short oddbyte;
    register short answer;

    sum=0;
    while(nbytes>1)
    {
        sum+=*ptr++;
        nbytes-=2;
    }
    if(nbytes==1)
    {
        oddbyte=0;
        *((u_char*)&oddbyte)=*(u_char*)ptr;
        sum+=oddbyte;
    }

    sum = (sum>>16)+(sum & 0xffff);
    sum = sum + (sum>>16);
    answer=(short)~sum;

    return(answer);
}
