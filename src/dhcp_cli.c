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
//	int fd = socket_fds[0];
	int sfd = socket_fds[2];

    int seed = time(NULL);
    srand(seed);
    int T1 = 0;
    int T2 = 0;
    int Tlease = 0;
    int timer = 0;
    int timeout_wait_before_discover = 0;
    int xid = 0;

    unsigned int m_len = 0;
	char * data;
	message * M;
	M = malloc(sizeof(message));
	char * interface_name = "enp3s6"; //get from argv?
	//char leasing_server_ip[16]; //get from config

	//заmmap'ить файл, а потом распарсить его при помощи strchr.
//	struct stat sb;
//	int fs = open(filename, O_RDONLY);
//	if(fd < 0 ) exit(-1); // ошибка
//	if(stat(fd, &sb) < 0) exit(-2); // ошибка
//	char *buf = mmap(NULL, sb.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
//	if(buf == MAP_FAILED) exit(-3); // ошибка

	char * s_mac = malloc(HLEN);
//	struct ifreq ifopt;
//	memset(&ifopt, 0, sizeof(ifopt));
//	strcpy(ifopt.ifr_name, interface_name);
//	if (ioctl(fd, SIOCGIFHWADDR, &ifopt) == -1) {
//		perror("Failed to get hwaddr");//, interface_name);
//		exit(4);
//	}
	//strncpy(s_mac, ifopt.ifr_hwaddr.sa_data, HLEN);
	get_current_mac(interface_name, s_mac);

	char * s_ip = malloc(15);
//	if (ioctl(fd, SIOCGIFADDR, &ifopt) == -1) {
//		perror("Failed to get ipaddr");//, interface_name);
//		exit(4);
//	}
//	struct sockaddr_in* ipaddr = (struct sockaddr_in*)&ifopt.ifr_addr;
//	s_ip = inet_ntoa(ipaddr->sin_addr);
//
//	close(fd);
	get_current_ip(interface_name, s_ip);

	char d_mac_broadcast[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
	char type_received = 0;
    clock_t start, stop;

    int state = STATE_INIT_REBOOT;
    while (1) {
		printf("State: %d\n", state);
        switch (state) {
			case STATE_INIT_REBOOT:
				xid = rand();
				data = malloc(PDU_DHCP_MAX);
				m_len = compose_request(xid, s_mac, data, s_ip, "192.168.0.0");
				if (send_l2_raw_message(sfd, m_len, data, IP_NULL, IP_BROAD, interface_name, s_mac, d_mac_broadcast) < 0)
					perror("Failed to send message");
				else {
					perror("Request sent");
					state = STATE_REBOOTING;
					start = clock();
				}
				free(data);
				sleep(1);
				break;
			case STATE_REBOOTING:
				M->data = malloc(PDU_DHCP_MAX);
				recv_l2_raw_message(sfd, M, match_xid, xid);
				type_received = dispatch_ack(M->len, M->data);
				free(M->data);
				switch (type_received) {
					case TYPE_DHCPACK:
						//record lease, set T1, set T2
						printf("ACK received\n");
						state = STATE_BOUND;
						start = clock();
						break;
					case TYPE_DHCPNAK:
						printf("NAK received!\n");
						timeout_wait_before_discover = rand() % 1 + 10;
						state = STATE_INIT;
						//restart??
						break;
					default:
						printf("Request timeout\n");
						stop =  clock();
						timer = (stop - start)/CPS; //CLOCKS_PER_SEC;
						if (timer > TIMEOUT_REQUESTING_SEC) {
							state = STATE_INIT;
						}
						break;
				}
				type_received = 0;
				sleep(1);
				break;
            case STATE_INIT:
            	//CLI --- DHCP Discover --> SRV
            	stop =  clock();
            	timer = (stop - start)/CPS; //CLOCKS_PER_SEC;
            	if (timer >= timeout_wait_before_discover) {
                	xid = rand();
					data = malloc(PDU_DHCP_MAX);
					m_len = compose_discover(xid, s_mac, data, s_ip, "0.0.0.0");
					start = clock();
					if (send_l2_raw_message(sfd, m_len, data, IP_NULL, IP_BROAD, interface_name, s_mac, d_mac_broadcast) < 0)
						perror("Failed to send message");
					else {
						perror("Discover sent");
						state = STATE_SELECTING;
					free(data);
					}
				} else sleep(1);
                break;
            case STATE_SELECTING:
				//CLI <-- DHCP Offer    --- SRV
				M->data = malloc(PDU_DHCP_MAX);
				recv_l2_raw_message(sfd, M, match_xid, xid);
				if (M->len > 0) {
					type_received = dispatch_ack(M->len, M->data);
					free(M->data);
					printf("type_received %d\n", type_received);
					switch (type_received) {
						case TYPE_DHCPOFFER:
							//record lease, set T1, set T2
							printf("offer received!\n");
							printf("sending_request!\n");
							xid = rand();
							data = malloc(PDU_DHCP_MAX);
							m_len = compose_request(xid, s_mac, data, s_ip, "192.168.0.1");
							if (send_l2_raw_message(sfd, m_len, data, IP_NULL, IP_BROAD, interface_name, s_mac, d_mac_broadcast) < 0) {
								perror("Failed to send request");
								exit(0);
							} else {
								perror("Request sent");
								state = STATE_REQUESTING;
								start = clock();
							}
							free(data);
							break;
						default:
							stop =  clock();
							timer = (stop - start)/CPS; //CLOCKS_PER_SEC;
							if (timer > TIMEOUT_SELECTING_SEC) {
								printf("Request timeout\n");
								timeout_wait_before_discover = rand() % 1 + 10;
								state = STATE_INIT;
							}
							break;
					}
				} else {
					free(M->data);
					stop =  clock();
					timer = (stop - start)/CPS; //CLOCKS_PER_SEC;
					if (timer > TIMEOUT_SELECTING_SEC) {
						printf("Request timeout\n");
						timeout_wait_before_discover = rand() % 1 + 10;
						state = STATE_INIT;
					}
				}
				sleep(1);
				break;
            case STATE_REQUESTING:
            	//CLI --- DHCP Request  --> SRV
            	//CLI <-- DHCP Ack      --- SRV
				M->data = malloc(PDU_DHCP_MAX);
				recv_l2_raw_message(sfd, M, match_xid, xid);
				type_received = dispatch_ack(M->len, M->data);
				free(M->data);
            	switch (type_received) {
            		case TYPE_DHCPACK:
						//record lease, set T1, set T2
            			T1 = 10; T2 = 20; Tlease = 30;
            			printf("ACK received\n");  // timers to set %d, %d\n",T1, T2);
            			start = clock();
            			if (1) {
            				state = STATE_BOUND;
            				start = clock();
            			} else {
            				state = STATE_INIT;
            				//send DHCP_decline
            			}
						break;
            		case TYPE_DHCPNAK:
            			printf("NAK received!\n");
            			state = STATE_INIT;
						//send dhcp_decline
            			break;
            		case TYPE_DHCPOFFER:
            			//DHCPoffer / discard
            			break;
            		default:
            			stop =  clock();
            			timer = (stop - start)/CPS; //CLOCKS_PER_SEC;
            			if (timer > TIMEOUT_REQUESTING_SEC) {
            				//Send DHCPDECLINE
            				state = STATE_INIT;
            			}
            			break;
				}
            	sleep(1);
                break;
            case STATE_BOUND:
            	//check if
            	stop =  clock();
            	timer = (stop - start)/CPS; //CLOCKS_PER_SEC;
            	if (timer > T1) {
            		printf("Bound timeout\n");
            		state = STATE_RENEWING;
            		//send dhcp_request to leasing_server
            	}
            	//DHCPOFFER, DHCPACK, DHCPNAK/Discard
            	sleep(1);
                break;
            case STATE_REBINDING:
				M->data = malloc(PDU_DHCP_MAX);
				recv_l2_raw_message(sfd, M, match_xid, xid);
				type_received = dispatch_ack(M->len, M->data);
				free(M->data);

				switch (type_received) {
					case TYPE_DHCPACK:
						//record lease, set T1, set T2
						state = STATE_BOUND;
						start = clock();
						break;
					case TYPE_DHCPNAK:
						state = STATE_INIT;
						//lease expired
						//halt network???
						break;
					default:
						stop =  clock();
						timer = (stop - start)/CPS; //CLOCKS_PER_SEC;
						if (timer > Tlease) {
							//broadcast DHCPREQUEST
							state = STATE_REBINDING;
						}
						break;
				}
				sleep(1);
                break;
            case STATE_RENEWING:
            	xid = rand();
				data = malloc(PDU_DHCP_MAX);
				m_len = compose_request(xid, s_mac, data, s_ip, "192.168.0.1");
				if (send_l2_raw_message(sfd, m_len, data, IP_NULL, IP_BROAD, interface_name, s_mac, d_mac_broadcast) < 0)
					perror("Failed to send message");
				else {
					perror("Request sent");
					start = clock();
				}
				free(data);

				M->data = malloc(PDU_DHCP_MAX);
				recv_l2_raw_message(sfd, M, match_xid, xid);
				type_received = dispatch_ack(M->len, M->data);
				free(M->data);

				switch (type_received) {
					case TYPE_DHCPACK:
						//record lease, set T1, set T2
						state = STATE_BOUND;
						start = clock();
						break;
					case TYPE_DHCPNAK:
						state = STATE_INIT;
						//halt network???
						break;
					default:
						stop =  clock();
						timer = (stop - start)/CPS; //CLOCKS_PER_SEC;
						if (timer > T2) {
							//broadcast DHCPREQUEST
							xid = rand();
							data = malloc(PDU_DHCP_MAX);
							m_len = compose_request(xid, s_mac, data, s_ip, "192.168.0.1");
							if (send_l2_raw_message(sfd, m_len, data, IP_NULL, IP_BROAD, interface_name, s_mac, d_mac_broadcast) < 0)
								perror("Failed to send message");
							else {
								perror("Request sent");
								start = clock();
							}
							free(data);

							state = STATE_REBINDING;
						}
						break;
				}
				sleep(1);
                break;
        }
    }
    free(M);
    return 1;
}
