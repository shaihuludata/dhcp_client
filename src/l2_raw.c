#include <stdio.h> //printf
#include <stdlib.h>
#include <string.h> //memset
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <linux/if_ether.h>
#include <linux/if_packet.h>
#include <netinet/ip.h>
#include <netinet/udp.h>   //Provides declarations for udp header
#include <netinet/in.h>
#include <net/ethernet.h>
#include <net/route.h>
#include <net/if.h>

#include "options.h"
#include "l2_raw.h"
#include "dhcp.h"

//unsigned short csum(unsigned short *ptr,int nbytes)
//{
//    register long sum;
//    unsigned short oddbyte;
//    register short answer;
//
//    sum=0;
//    while(nbytes>1) {
//        sum+=*ptr++;
//        nbytes-=2;
//    }
//    if(nbytes==1) {
//        oddbyte=0;
//        *((u_char*)&oddbyte)=*(u_char*)ptr;
//        sum+=oddbyte;
//    }
//
//    sum = (sum>>16)+(sum & 0xffff);
//    sum = sum + (sum>>16);
//    answer=(short)~sum;
//
//    return(answer);
//}

//unsigned short udp_csum(unsigned short *buf, int nwords)
//{       //
//        unsigned long sum;
//        for(sum=0; nwords>0; nwords--)
//                sum += *buf++;
//        sum = (sum >> 16) + (sum &0xffff);
//        sum += (sum >> 16);
//        return (unsigned short)(~sum);
//}

unsigned short csum(uint16_t *addr, short len)
{
	int sum = 0;
	uint16_t *w = addr;
	uint16_t answer = 0;

	while (len > 1) {
		sum += *w++;
		len -= sizeof (uint16_t);
	}

	if (len == 1) {
		*(uint8_t *) (&answer) = *(uint8_t *) w;
		sum += answer;
	}

	sum = (sum >> 16) + (sum & 0xFFFF);
	sum += (sum >> 16);
	answer = ~sum;
	return (answer);
}

//unsigned short udp_csum(struct iphdr * iph, struct udphdr * udph, short datagram_size)
//{
//	short psize = sizeof(struct pseudo_header) + datagram_size;
//	char * pseudogram = malloc(psize);
//	struct pseudo_header * psh = (struct pseudo_header *) pseudogram;
//	psh->source_address = ntohl(iph->saddr);
//	psh->dest_address = ntohl(iph->daddr);
//	psh->placeholder = 0;
//	psh->protocol = IPPROTO_UDP;
//	psh->udp_length = ntohs(udph->len);
//
//	memcpy(pseudogram + sizeof(*psh), udph, datagram_size);
//
//	int checksum = 0;
//	short * hptr = (short * ) pseudogram;
//
//	while (psize > 1) {
//		checksum += *(hptr++);
//		psize -= sizeof(psize);
//	}
//	if (psize & 1) {
//		checksum += *((uint8_t *)hptr++);
//	}
//
//	while (checksum >> 16)
//		checksum = (checksum & 0xFFFF) + (checksum >> 16);
//
//	return ((short)~checksum);
//}

unsigned short udp_csum(const void * buff, size_t len, in_addr_t src_addr, in_addr_t dest_addr)
{
        const uint16_t *buf=buff;
        uint16_t *ip_src=(void *)&src_addr, *ip_dst=(void *)&dest_addr;
        uint32_t sum;
        size_t length=len;

        // Calculate the sum                                            //
        sum = 0;
        while (len > 1)
        {
                sum += *buf++;
                if (sum & 0x80000000)
                        sum = (sum & 0xFFFF) + (sum >> 16);
                len -= 2;
        }

        if ( len & 1 )
                // Add the padding if the packet lenght is odd          //
                sum += *((uint8_t *)buf);

        // Add the pseudo-header                                        //
        sum += *(ip_src++);
        sum += *ip_src;

        sum += *(ip_dst++);
        sum += *ip_dst;

        sum += htons(IPPROTO_UDP);
        sum += htons(length);

        // Add the carries                                              //
        while (sum >> 16)
                sum = (sum & 0xFFFF) + (sum >> 16);

        // Return the one's complement of sum                           //
        return ( (uint16_t)(~sum)  );
}

int init_socket (int * sfds)
{
	int sfd = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
	if (sfd == -1)
	{
		//socket creation failed, may be because of non-root privileges
		perror("Failed to create raw socket");
		exit(0);
	}

	/* create a UDP socket to prevent ICMP port unreachable */
	struct sockaddr_in udp_addr;
	int udp_rcv_sfd = socket(PF_INET, SOCK_DGRAM, 0);
	if (udp_rcv_sfd < 0)
	{
		perror("Failed to create udp_rcv_sfd");
		exit(1);
	}

	memset(&udp_addr, 0, sizeof(udp_addr));
	udp_addr.sin_family = PF_INET;
	udp_addr.sin_addr.s_addr = htons(INADDR_ANY);
	udp_addr.sin_port = DHCP_PORT_CLI;
	if (bind(udp_rcv_sfd, (struct sockaddr *)&udp_addr, sizeof(udp_addr)) == -1)
	{
		perror("udp_rcv_sfd: bind error");
		exit(2);
	}

	int send_sfd = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
	if (send_sfd < 0)
	{
		perror("Failed to create send_sfd socket.");
		exit(3);
	}
	else
	{
//		struct timeval timeout;
//		timeout.tv_sec = DISCOVER_TIMEOUT_SEC;
//		timeout.tv_usec = 0;
//		setsockopt(send_sfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

		int optval = 1;
		//int opt = setsockopt(send_sfd, SOL_SOCKET, SO_BROADCAST, (char *) &(optval), sizeof(int));
		setsockopt(send_sfd, SOL_SOCKET, SO_BROADCAST, (char *) &(optval), sizeof(int));
		//printf("%d\n", opt);
	}

	//int socks[3] = {sfd, udp_rcv_sfd, send_sfd};
	sfds[0] = sfd; sfds[1] = udp_rcv_sfd; sfds[2] = send_sfd;

	//sfds = (int * [3]){sfd, udp_rcv_sfd, send_sfd};
	return 0;
}

int send_l2_raw_message(unsigned int data_len, char * payload, char source_ip[32], char destination_ip[32],
		char interface_name[10], char source_mac[6], char destination_mac[6])
{
	int socket_fds[3];
	init_socket(socket_fds);
	int send_sfd = socket_fds[2];
	char data[data_len];
	memset (&data, 0, data_len);
	memcpy(data, payload, data_len);

	char frame[1500];
	struct ethhdr * ethh = (struct ethhdr *) &frame[0];
	struct iphdr * iph = (struct iphdr *) &frame[L2_HDR_SIZE];
	struct udphdr * udph = (struct udphdr *) &frame[L2_HDR_SIZE+L3_HDR_SIZE];

	int f_size = sizeof(ethh) + sizeof(iph) + sizeof(udph) + sizeof(data);

	memset (frame, 0, f_size);
	memcpy(&frame[sizeof(*ethh) + sizeof(*udph) + sizeof(*iph)], data, sizeof(data));

//	struct sockaddr_in * sin = malloc(sizeof(struct sockaddr_in));
//	memset(sin, 0, sizeof(*sin));
//	sin->sin_family = PF_PACKET;
//	sin->sin_port = htons(DHCP_PORT_CLI);
//	//sin.sin_addr.s_addr = inet_addr (destination_ip);
//	//inet_aton (destination_ip, &sin->sin_addr);
//	struct in_addr socket_addr;
//	inet_aton (destination_ip, &socket_addr);
//	memcpy(&sin->sin_addr, &socket_addr, 4);

	struct sockaddr_ll * sin = malloc(sizeof(struct sockaddr_ll));
	memset(sin, 0, sizeof(*sin));
	if ((sin->sll_ifindex = if_nametoindex(interface_name)) == 0) {
			perror("Failed to resolve the index of interface.\n");
			exit(5);
		}
	//sin->sll_family = PF_PACKET;

	//sin.sin_addr.s_addr = inet_addr (destination_ip);
	//inet_aton (destination_ip, &sin->sin_addr);

//	struct if_nameindex *interfaces = if_nameindex();
//	struct if_nameindex *interface;

//	ethh->h_dest = htons(*destination_mac);
	memcpy((char *)ethh->h_dest, destination_mac, 6);
	memcpy((char *)ethh->h_source, source_mac, 6);
	//memcpy((char *)ethh->h_source, ifopt.ifr_hwaddr.sa_data, 6);
	ethh->h_proto = htons(0x0800);

	short int packet_size = sizeof (*iph) + sizeof (*udph) + data_len;
	iph->ihl = 5;
	iph->version = 4;
	iph->tos = 0;
	iph->tot_len = htons(packet_size);
	iph->id = htons (54321); //Id of this packet
	iph->frag_off = 0;
	iph->ttl = 250;
	iph->protocol = IPPROTO_UDP;
	iph->check = 0;      //Set to 0 before calculating checksum
	//iph->saddr = inet_addr (source_ip);    //Spoof the source ip address
	struct in_addr saddr;
	inet_aton (source_ip, &saddr);    //Spoof the source ip address
	iph->saddr = saddr.s_addr;
	struct in_addr daddr;
	inet_aton (destination_ip, &daddr);
	iph->daddr = daddr.s_addr; //sin->sin_addr.s_addr;

	char * packet = &frame[sizeof(*ethh)];
	iph->check = csum ((unsigned short *) packet, sizeof(*iph));

	short int datagram_size = sizeof(*udph) + data_len;
	udph->source = htons (DHCP_PORT_CLI);
	udph->dest = htons (DHCP_PORT_SRV);
	udph->len = htons(datagram_size); //8 - udp header size
	udph->check = 0;

	char * datagram = &frame[sizeof(*ethh) + sizeof(*iph)];
	//udph->check = htons(udp_csum(iph, udph, datagram_size));  // (unsigned short*) packet , datagram_size));
	udph->check = udp_csum(datagram, datagram_size, saddr.s_addr, daddr.s_addr);

	int ret;
	size_t total_len = sizeof (*ethh) + sizeof (*iph) + sizeof (*udph) + sizeof(data);
	ret = sendto(send_sfd, &frame[0], total_len,  0, (struct sockaddr *) sin, sizeof (*sin));
	printf("%d\n", ret);

	for (int i=0; i<sizeof(socket_fds)/sizeof(socket_fds[0]); i++)
		close(socket_fds[i]);
	return ret;
}
