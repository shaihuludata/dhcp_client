/*
 ============================================================================
 Name        : dhcp_cli.c
 Author      : MDS
 Version     : 0.1
 Description : DHCP client on raw UDP sockets
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <time.h>
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
#include <sys/times.h>
#include <net/if.h>

#include "dhcp.h"
#include "options.h"
#include "udp_raw.h"
#include "l2_raw.h"

#define CPS 100

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

    struct tms b_buf, e_buf;
    int start = times(&b_buf);
    int stop = times(&e_buf);
    int timer = 0;

    int T1 = 0;
    int T2 = 0;
    int Tlease = 0;
    int timeout_wait_before_discover = 0;
    int xid = 0;

    unsigned int m_len = 0;
	char * data = malloc(PDU_DHCP_MAX);
	char * frame = malloc(MAX_ETH_F);
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
	get_current_mac(interface_name, s_mac);

	char * s_ip = malloc(15);
	get_current_ip(interface_name, s_ip);

	char d_mac_broadcast[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
	char type_received = 0;

    int state = STATE_INIT_REBOOT;

    int state_init_reboot() {
		xid = rand();
		memset(data, 0, PDU_DHCP_MAX);
		m_len = compose_request(xid, s_mac, data, s_ip, "192.168.0.0");
		if (send_l2_raw_message(sfd, m_len, data, IP_NULL, IP_BROAD, interface_name, s_mac, d_mac_broadcast) < 0) {
			perror("Failed to send message");
			return STATE_INIT_REBOOT;
    	} else {
			perror("Request sent");
			start = times(&b_buf); timer = 0;
			return STATE_REBOOTING;
		}
    }

    int state_rebooting() {
    	do {
    		m_len = 0;
    		while (m_len == 0) {
    			m_len = recv_l2_raw_message(sfd, data, frame, match_xid, xid, TIMEOUT_REQUESTING_SEC);
    		}
			type_received = dispatch_ack(m_len, data);

			switch (type_received) {
				case TYPE_DHCPACK:
					//record lease, set T1, set T2
					printf("\t <--- ACK received\n");
					start = times(&b_buf); timer = 0;
					return STATE_BOUND;
					break;
				case TYPE_DHCPNAK:
					printf("\t <--- NAK received!\n");
					timeout_wait_before_discover = rand() % 1 + 10;
					//restart??
					return STATE_INIT;
					break;
				default:
					printf("type received %d\n", type_received);
					break;
			}
			stop = times(&e_buf);
			timer = (stop - start)/CPS;  // CLOCKS_PER_SEC;
    	} while (timer < TIMEOUT_REQUESTING_SEC);  // && state == STATE_REBOOTING);
    	printf("Request timeout\n");
    	return STATE_INIT;
    }

    int state_init() {
		do {
			stop = times(&e_buf);
			timer = (stop - start)/CPS; //CLOCKS_PER_SEC;
			sleep(1);
		} while (timer > timeout_wait_before_discover);
		xid = rand();

		start = times(&b_buf); timer = 0;
		m_len = compose_discover(xid, s_mac, data, s_ip, "0.0.0.0");
		if (send_l2_raw_message(sfd, m_len, data, IP_NULL, IP_BROAD, interface_name, s_mac, d_mac_broadcast) < 0) {
			perror("Failed to send message");
			return STATE_INIT;
		} else {
			perror("Discover sent");
			return STATE_SELECTING;
		}
    }

    int state_selecting() {
    	m_len = 0;
		do {
			while(m_len == 0) {
				m_len = recv_l2_raw_message(sfd, data, frame, match_xid, xid, TIMEOUT_SELECTING_SEC);
			}

			type_received = dispatch_ack(m_len, data);
			switch (type_received) {
				case TYPE_DHCPOFFER:
					//record lease, set T1, set T2
					printf("\t <--- offer received!\n");
					xid = rand();
					m_len = compose_request(xid, s_mac, data, s_ip, "192.168.0.1");
					if (send_l2_raw_message(sfd, m_len, data, IP_NULL, IP_BROAD, interface_name, s_mac, d_mac_broadcast) < 0) {
						perror("Failed to send request");
						return STATE_SELECTING;
					} else {
						perror("Request sent");
						start = times(&b_buf); timer = 0;
						return STATE_REQUESTING;
					}
					break;
				default:
					printf("type_received %d\n", type_received);
					sleep(10);
			}
			stop = times(&e_buf);
			timer = (stop - start)/CPS; //CLOCKS_PER_SEC;
		//} while ((timer < TIMEOUT_SELECTING_SEC) && (state == STATE_SELECTING));
		} while (timer < TIMEOUT_SELECTING_SEC);
		printf("Request timeout\n");
		start = times(&b_buf); timer = 0;
		timeout_wait_before_discover = rand() % 1 + 10;
		return STATE_INIT;
		}

    int state_requesting() {
		do {
			m_len = 0;
			while(m_len == 0) {
				m_len = recv_l2_raw_message(sfd, data, frame, match_xid, xid, 1);
			}
			type_received = dispatch_ack(m_len, data);
			switch (type_received) {
				case TYPE_DHCPACK:
					// record lease, set T1, set T2
					T1 = 10; T2 = 20; Tlease = 30;
					printf("\t <--- ACK received\n");  // timers to set %d, %d\n",T1, T2);
					start = times(&b_buf); timer = 0;
					if (1) {
						start = times(&b_buf); timer = 0;
						return STATE_BOUND;
					} else {
						// send DHCP_decline
						return STATE_INIT;
					}
					break;
				case TYPE_DHCPNAK:
					printf("\t <--- NAK received!\n");
					return STATE_INIT;
					// send dhcp_decline
					break;
				case TYPE_DHCPOFFER:
					// DHCPoffer / discard
					break;
				default:
					printf("type received %d\n", type_received);
					sleep(10);
			}
			stop = times(&e_buf);
			timer = (stop - start)/CPS;  // CLOCKS_PER_SEC;
		} while (timer < TIMEOUT_REQUESTING_SEC);
		return STATE_INIT;
		// Send DHCPDECLINE
    }

    int state_bound() {
		//check if
		memset(data, 0, PDU_DHCP_MAX);

		while (timer < T1) {
			stop = times(&e_buf);
			timer = (stop - start)/CPS;  // CLOCKS_PER_SEC;
			//DHCPOFFER, DHCPACK, DHCPNAK/Discard
		}
		//send dhcp_request to leasing_server
		printf("Bound timeout\n");

		xid = rand();
		m_len = compose_request(xid, s_mac, data, s_ip, "192.168.0.1");
		if (send_l2_raw_message(sfd, m_len, data, IP_NULL, IP_BROAD, interface_name, s_mac, d_mac_broadcast) < 0) {
			perror("Failed to send message");
			return STATE_BOUND;
		} else {
			perror("Request sent");
			start = times(&b_buf); timer = 0;
			return STATE_RENEWING;
		}
    }

    int state_renewing() {
    	m_len = 0;
    	memset(data, 0, PDU_DHCP_MAX);
    	do {
    		while(m_len == 0) {
    			m_len = recv_l2_raw_message(sfd, data, frame, match_xid, xid, 1);
    		}
    		type_received = dispatch_ack(m_len, data);

			switch (type_received) {
				case TYPE_DHCPACK:
					printf("\t <--- ACK received\n");  // timers to set %d, %d\n",T1, T2);
					start = times(&b_buf); timer = 0;
					return STATE_BOUND;
					break;
				case TYPE_DHCPNAK:
					printf("\t <--- NACK received\n");
					return STATE_INIT;
					//halt network???
					break;
				default:
					printf("type received %d\n", type_received);
					sleep(1);
					break;
			}
			stop = times(&e_buf);
			timer = (stop - start)/CPS; //CLOCKS_PER_SEC;
    	} while (timer < T2);
    	//broadcast DHCPREQUEST
		xid = rand();
		m_len = compose_request(xid, s_mac, data, s_ip, "192.168.0.1");
		if (send_l2_raw_message(sfd, m_len, data, IP_NULL, IP_BROAD, interface_name, s_mac, d_mac_broadcast) < 0)
			perror("Failed to send message");
		else {
			perror("Request sent");
			start = times(&b_buf); timer = 0;
		}
		return STATE_REBINDING;
    }

    int state_rebinding() {
    	memset(data, 0, PDU_DHCP_MAX);
    	m_len = 0;
    	do {
			while(m_len == 0) {
				m_len = recv_l2_raw_message(sfd, data, frame, match_xid, xid, 1);
			}
			type_received = dispatch_ack(m_len, data);

			switch (type_received) {
				case TYPE_DHCPACK:
					//record lease, set T1, set T2
					return STATE_BOUND;
					start = times(&b_buf); timer = 0;
					break;
				case TYPE_DHCPNAK:
					return STATE_INIT;
					//lease expired
					//halt network???
					break;
				default:
					printf("\t <--- type received %d\n", type_received);
					break;
			}
			stop = times(&e_buf);
			timer = (stop - start)/CPS; //CLOCKS_PER_SEC;
    	} while (timer < Tlease);
		//broadcast DHCPREQUEST
		return STATE_REBINDING;
    }

    while (1) {
		printf("State: %d\n", state);
        switch (state) {
			case STATE_INIT_REBOOT:
				state = state_init_reboot();
				break;
			case STATE_REBOOTING:
				state = state_rebooting();
				break;
            case STATE_INIT:
//            	//CLI --- DHCP Discover --> SRV
            	state = state_init();
				break;
            case STATE_SELECTING:
//            	//CLI <-- DHCP Offer    --- SRV
            	state = state_selecting();
				break;
            case STATE_REQUESTING:
//            	//CLI --- DHCP Request  --> SRV
//            	//CLI <-- DHCP Ack      --- SRV
            	state = state_requesting();
                break;
            case STATE_BOUND:
            	state = state_bound();
                break;
            case STATE_RENEWING:
            	state = state_renewing();
                break;
            case STATE_REBINDING:
				state = state_rebinding();
                break;
			default:
				printf("State not implemented\n");
				sleep(10);
        }
//        sleep(1);
        memset(data, 0, PDU_DHCP_MAX);
        memset(frame, 0, MAX_ETH_F);
    }
    free(data);
    return 1;
}
