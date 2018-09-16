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

int main(int argc, char * argv[]) {
	int socket_fds[3];
	init_socket(socket_fds);
	/*
	 * 0 - to configuration and ioctl
	 * 1 - to udp_socket to be binded and prevent ICMP port unreachable,
	 * 2 - agh burzum-ishi krimpatul
	 */
	int fd = socket_fds[0];
	int sfd = socket_fds[2];

    int seed = time(NULL);
    srand(seed);
    int T1 = rand() % 1 + 1;
    int T2 = 0;
    int timer = 0;
    int xid = 0;

    unsigned int m_len = 0;
	char * data;
	message * M;
	char * interface_name = "enp3s6"; //get from argv?
	//char leasing_server_ip[16]; //get from config

	//int fd = socket(AF_INET, SOCK_DGRAM, 0);
	struct ifreq ifopt;
	memset(&ifopt, 0, sizeof(ifopt));
	strcpy(ifopt.ifr_name, interface_name);

	char * s_mac = malloc(HLEN);
	if (ioctl(fd, SIOCGIFHWADDR, &ifopt) == -1) {
		perror("Failed to get hwaddr");//, interface_name);
		exit(4);
	}
	strncpy(s_mac, ifopt.ifr_hwaddr.sa_data, HLEN);

	char * s_ip = malloc(PLEN);
	if (ioctl(fd, SIOCGIFADDR, &ifopt) == -1) {
		perror("Failed to get ipaddr");//, interface_name);
		exit(4);
	}
	struct sockaddr_in* ipaddr = (struct sockaddr_in*)&ifopt.ifr_addr;
	//strncpy(s_ip, inet_ntoa(ipaddr->sin_addr), 4);
	s_ip = inet_ntoa(ipaddr->sin_addr);
	printf("%s\n", s_ip);

	close(fd);

	char d_mac_broadcast[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
	char type_received = 0;
    clock_t start = clock();
    clock_t stop;

    int state = STATE_INIT_REBOOT;
    while (1) {
		printf("State: %d\n", state);
        switch (state) {
			case STATE_INIT_REBOOT:
				xid = rand();
				data = malloc(PDU_DHCP_MAX);
				memset(data, 0, PDU_DHCP_MAX);
				m_len = compose_request(xid, s_mac, data, s_ip, "192.168.0.1");
				if (send_l2_raw_message(sfd, m_len, data, IP_NULL, IP_BROAD, interface_name, s_mac, d_mac_broadcast) < 0)
					perror("Failed to send message");
				else {
					perror("Request sent");
					state = STATE_REBOOTING;
				}
				free(data);
				break;
			case STATE_REBOOTING:
				//int len = recv(listen_raw_fd, buf, max_len, 0);
				M = malloc(sizeof(message));
				memset(M, 0, sizeof(message));
				M->data = malloc(PDU_DHCP_MAX);
				recv_l2_raw_message(sfd, M);
				type_received = dispatch_ack(M);
				free(M->data);
				free(M);
				switch (type_received) {
					case TYPE_DHCPACK:
						//record lease, set T1, set T2
						printf("Successss!\n");
						state = STATE_BOUND;
						break;
					case TYPE_DHCPNAK:
						printf("NAK received!\n");
						state = STATE_INIT;
						//restart??
						break;
					default:
						if (timer > T2) {
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
                if (timer >= T1) {
					if (send_l2_raw_message(sfd, sizeof(data), (char *)&data, IP_NULL, IP_BROAD,
							interface_name, s_mac, d_mac_broadcast) < 0)
						perror("Failed to send message");
					else {
						perror("Discover sent");
						state = STATE_SELECTING;
					}
				}
                break;
            case STATE_SELECTING:
            	//CLI <-- DHCP Offer    --- SRV
            	//prepare buffer for offers
            	if (1>0) {
            		state = STATE_REQUESTING;
				}
                break;
            case STATE_REQUESTING:
            	//CLI --- DHCP Request  --> SRV
            	//CLI <-- DHCP Ack      --- SRV
            	switch (type_received) {
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
