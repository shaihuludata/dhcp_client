/*
 ============================================================================
 Name        : dhcp_cli.c
 Author      : MDS
 Version     : 0.1
 Description : DHCP client on raw UDP sockets
 ============================================================================
 */

#include <stdio.h>
#include <sys/socket.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stropts.h> //ioctl
#include <sys/ioctl.h>
#include <sys/random.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/syslog.h>
#include <net/if.h>
#include "dhcp.h"
#include "options.h"
#include "udp_raw.h"
#include "l2_raw.h"

#define CPS 40

int main(int argc, char * argv[])
{
    int state = STATE_INIT_REBOOT;
    int seed = time(NULL);
    srand(seed);
    int T1 = rand() % 1 + 1;
    int T2 = 0;
    int timer = 0;
    int xid = 0;

	//message M;
    unsigned int m_len = 0;
	char * data;  // [PDU_DHCP_MAX];
	char * interface_name = "enp3s6";
	//char leasing_server_ip[16]; //get from argv?

	int fd = socket(AF_INET, SOCK_DGRAM, 0);
	struct ifreq ifopt;
	memset(&ifopt, 0, sizeof(ifopt));
	strcpy(ifopt.ifr_name, interface_name);
	if (ioctl(fd, SIOCGIFHWADDR, &ifopt) == -1)
	{
		perror("Failed to get hwaddr");//, interface_name);
		exit(4);
	}
	char * s_mac = malloc(HLEN);
	strncpy(s_mac, ifopt.ifr_hwaddr.sa_data, HLEN);

	if (ioctl(fd, SIOCGIFADDR, &ifopt) == -1)
	{
		perror("Failed to get ipaddr");//, interface_name);
		exit(4);
	}
	char * s_ip = malloc(PLEN);
	struct sockaddr_in* ipaddr = (struct sockaddr_in*)&ifopt.ifr_addr;
	strcpy(s_ip, inet_ntoa(ipaddr->sin_addr));//, 4);

	close(fd);

	char d_mac_broadcast[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
	int type_received = 0;
    clock_t start = clock();
    clock_t stop;

    while (1)
    {
		printf("State: %d\n", state);
        switch (state)
        {
			case STATE_INIT_REBOOT:
				xid = rand();
				data = malloc(PDU_DHCP_MAX);
				memset(data, 0, PDU_DHCP_MAX);
				m_len = compose_request(xid, s_mac, data, s_ip, "192.168.0.1");
				printf("%d\n", m_len);
				//memcpy(data, M.data, M.len);
				if (send_l2_raw_message(m_len, data, IP_NULL, IP_BROAD,
						interface_name, s_mac, d_mac_broadcast) < 0)
					perror("Failed to send message");
				else
				{
					perror("Request sent");
					state = STATE_REBOOTING;
				}
				break;
			case STATE_REBOOTING:
				switch (type_received)
				{
					case TYPE_DHCPACK:
						//record lease, set T1, set T2
						state = STATE_BOUND;
						break;
					case TYPE_DHCPNAK:
						state = STATE_INIT;
						//restart??
						break;
					default:
						if (timer > T2)
						{
							//broadcast DHCPREQUEST
							state = STATE_REBINDING;
						}
						break;
				}
				break;
            case STATE_INIT:
            	//CLI --- DHCP Discover --> SRV
            	//m_len = compose_discover(1);
            	//data = M.data
                stop =  clock();
                timer = (stop - start)/CPS; //CLOCKS_PER_SEC;
                timer = 10;
                if (timer >= T1)
                {
					if (send_l2_raw_message(sizeof(data), (char *)&data, IP_NULL, IP_BROAD,
							interface_name, s_mac, d_mac_broadcast) < 0)
						perror("Failed to send message");
					else
					{
						perror("Discover sent");
						state = STATE_SELECTING;
					}
				}
                break;
            case STATE_SELECTING:
            	//CLI <-- DHCP Offer    --- SRV
            	//prepare buffer for offers
            	if (1>0)
				{
            		state = STATE_REQUESTING;
				}
                break;
            case STATE_REQUESTING:
            	//CLI --- DHCP Request  --> SRV
            	//CLI <-- DHCP Ack      --- SRV
            	switch (type_received)
            	{
            		case TYPE_DHCPACK:
						//record lease, set T1, set T2
						state = STATE_BOUND;
						break;
            		case TYPE_DHCPNAK:
            			state = STATE_INIT;
						//send dhcp_decline
            			break;
            		case TYPE_DHCPOFFER:
            			//DHCPoffer / discard
            			break;
            		default:
            			if (timer > TIMEOUT_REQUESTING_SEC)
            			{
            				//Send DHCPDECLINE
            				state = STATE_INIT;
            			}
            			break;
				}
                break;
            case STATE_BOUND:
            	if (timer > TIMEOUT_BOUND_SEC)
            	{
            		state = STATE_RENEWING;
            		//send dhcp_request to leasing_server
            	}
            	//DHCPOFFER, DHCPACK, DHCPNAK/Discard
                break;
            case STATE_REBINDING:
				switch (type_received)
				{
					case TYPE_DHCPACK:
						//record lease, set T1, set T2
						state = STATE_BOUND;
						break;
					case TYPE_DHCPNAK:
						state = STATE_INIT;
						//lease expired
						//halt network???
						break;
					default:
						if (timer > T2)
						{
							//broadcast DHCPREQUEST
							state = STATE_REBINDING;
						}
						break;
				}
                break;
            case STATE_RENEWING:
				switch (type_received)
				{
					case TYPE_DHCPACK:
						//record lease, set T1, set T2
						state = STATE_BOUND;
						break;
					case TYPE_DHCPNAK:
						state = STATE_INIT;
						//halt network???
						break;
					default:
						if (timer > T2)
						{
							//broadcast DHCPREQUEST
							state = STATE_REBINDING;
						}
						break;
				}
                break;
        }
        sleep(1);
    }
    return 1;
}
