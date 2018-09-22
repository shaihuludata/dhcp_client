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
#include <time.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <stropts.h> //ioctl
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <sys/random.h>
#include <sys/stat.h>
#include <sys/syslog.h>
#include <sys/times.h>
#include <net/if.h>

#include "dhcp.h"
#include "dhcp_cli.h"
#include "lease.h"
#include "options.h"
#include "udp_raw.h"
#include "l2_raw.h"
#include "stupid_header_file.h"

#define CPS 100
#define INTERFACE "enp3s6"
#define LEASE_DIR "./"

#define DBG1 1  // NAK after state_init_reboot
#define DBG10 10  // reduce T1

int debug = 10;

void usage() {
	fprintf(stdout, "usage: dhcp_cli [-ncsi]\n");
	fprintf(stdout, "[-n, --no-daemonize]\n");
	fprintf(stdout, "[-c, --client]\n");
	fprintf(stdout, "[-s, --server]\n");
	fprintf(stdout, "[-i, --interface] [interface]\n");
}

int options_validation(int argc, char ** argv) {
	return 1;
}

int main(int argc, char ** argv) {
	int daemonize = 1;
	char * s_ip = malloc(IP_MAX_STR_SIZE);
	char * d_ip = malloc(IP_MAX_STR_SIZE);
	char * interface_name = malloc(10);
	int interface_set = 0;
	int s_ip_set = 0;
	int d_ip_set = 0;

	if (options_validation(argc, argv)) {
		for (int i = 1; i < argc; ++i) {
			if ((strcmp(argv[i], "-n") == 0) ||
					(strcmp(argv[i], "--no-daemonize") == 0)) {
				daemonize = 0;
			}
			if ((strcmp(argv[i], "-s") == 0) ||
					(strcmp(argv[i], "--server") == 0)) {
				i++;
				d_ip = argv[i];
				d_ip_set = 1;
			}
			if ((strcmp(argv[i], "-i") == 0) ||
					(strcmp(argv[i], "--interface") == 0)) {
				i++;
				interface_name = argv[i];
				interface_set = 1;
			}
			if ((strcmp(argv[i], "-c") == 0) ||
					(strcmp(argv[i], "--client_addr") == 0)) {
				i++;
				s_ip = argv[i];
				s_ip_set = 1;
			}
		}
	} else {
		usage();
		exit(0);
	}

	if (!interface_set) {
		interface_name = INTERFACE;
	}

	int socket_fds[3];
	init_socket(socket_fds);
	/*
	 * 0 - for configuration and ioctl
	 * 1 - for udp_socket to prevent ICMP port unreachable,
	 * 2 - agh burzum-ishi krimpatul
	 */
	int sfd = socket_fds[2];

    int seed = time(NULL);
    srand(seed);

    struct tms b_buf, e_buf;
    int start = times(&b_buf);
    int stop = times(&e_buf);
    int timer = 0;

    lease * lease = malloc(sizeof(*lease));
    int T1 = 0;
    int T2 = 0;
    void * option_value = malloc(100);
    int timeout_wait_before_discover = 0;
    int xid = 0;

    unsigned int m_len = 0;
	char * data = malloc(PDU_DHCP_MAX);
	char * frame = malloc(MAX_ETH_F);

	//заmmap'ить файл конфигурации, а потом распарсить его
	struct stat sb;
	char lease_filename[20];
	snprintf(lease_filename, sizeof(lease_filename),
			"%s%s%s", LEASE_DIR, interface_name, ".lease");

	FILE * fp = NULL;
	//int fs;
	//char * config_buf;
	if (stat(lease_filename, &sb) < 0) {
		perror("Creating lease file");
		fp = fopen(lease_filename, "w");
		fclose(fp);
		new_lease(lease, lease_filename);
	}
	else {
		fprintf(stdout, "Using existent lease file\n");
		get_lease(lease, lease_filename);
	}

	if (!s_ip_set) {
		//get_current_ip(interface_name, s_ip);
		struct in_addr addr;
		addr.s_addr = lease->client_ip;
		memcpy(s_ip, inet_ntoa(addr), IP_MAX_STR_SIZE);
	}
	if (!d_ip_set) {
		//get_current_ip(interface_name, d_ip);
		struct in_addr addr;
		addr.s_addr = lease->server_ip;
		memcpy(d_ip, inet_ntoa(addr), IP_MAX_STR_SIZE);
	}

	char * s_mac = malloc(HLEN);
	get_current_mac(interface_name, s_mac);

	char d_mac[HLEN] = MAC_BCAST;
	char type_received = 0;

    int state = STATE_INIT_REBOOT;

    int state_init_reboot() {
		xid = rand();
		memset(data, 0, PDU_DHCP_MAX);
		if (debug == DBG1)
			m_len = compose_request(xid, s_mac, data, IP_NULL, d_ip);
		else
			m_len = compose_request(xid, s_mac, data, s_ip, d_ip);
		if (send_l2_raw_message(sfd, m_len, data, IP_NULL, IP_BROAD,
				interface_name, s_mac, d_mac) < 0) {
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
    			m_len = recv_l2_raw_message(sfd, data, frame, match_xid,
    					xid, TIMEOUT_REQUESTING_SEC);
    		}
    		get_dhcp_o_value(m_len, data, OPT53, option_value);
			type_received = (*(char *) option_value);

			switch (type_received) {
				case TYPE_DHCPACK:
					printf("\t <--- ACK received\n");
					get_dhcp_o_value(m_len, data, OPT51, option_value);
					lease->lease_time = ntohl((*(uint32_t *)option_value));
					get_dhcp_o_value(m_len, data, OPT58, option_value);
					if (debug == DBG10)
						T1 = 5;
					else
						T1 = ntohl((*(uint32_t *)option_value));

					get_dhcp_o_value(m_len, data, OPT59, option_value);
					T2 = ntohl((*(uint32_t *)option_value));
					lease->renew_time = T1;
					lease->rebind_time = T2;
					get_dhcp_o_value(m_len, data, OPT1, &lease->mask_ip);
					get_dhcp_o_value(m_len, data, OPT3, &lease->router_ip);
					get_dhcp_o_value(m_len, data, OPT6, &lease->dns);

					start = times(&b_buf); timer = 0;
					return STATE_BOUND;
					break;
				case TYPE_DHCPNAK:
					printf("\t <--- NAK received!\n");
					timeout_wait_before_discover = rand() % 1 + 10;
					memset(lease, 0, sizeof(*lease));
					return STATE_INIT;
					break;
				default:
					printf("type received %d\n", type_received);
					break;
			}
			stop = times(&e_buf);
			timer = (stop - start)/CPS;
    	} while (timer < TIMEOUT_REQUESTING_SEC);
    	printf("Request timeout\n");
    	return STATE_INIT;
    }

    int state_init() {
		do {
			stop = times(&e_buf);
			timer = (stop - start)/CPS;
			sleep(1);
		} while (timer > timeout_wait_before_discover);
		xid = rand();

		start = times(&b_buf); timer = 0;
		m_len = compose_discover(xid, s_mac, data, s_ip, IP_NULL);
		if (send_l2_raw_message(sfd, m_len, data, IP_NULL, IP_BROAD,
				interface_name, s_mac, d_mac) < 0) {
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
				m_len = recv_l2_raw_message(sfd, data, frame, match_xid, xid,
						TIMEOUT_SELECTING_SEC);
			}

			get_dhcp_o_value(m_len, data, OPT53, option_value);
			type_received = (*(char *) option_value);
			switch (type_received) {
				case TYPE_DHCPOFFER:
					get_dhcp_o_value(m_len, data, OPT58, option_value);
					if (debug == DBG10)
						T1 = 5;
					else
						T1 = ntohl((*(uint32_t *)option_value));
					get_dhcp_o_value(m_len, data, OPT59, option_value);
					T2 = ntohl((*(uint32_t *)option_value));
					lease->renew_time = T1;
					lease->rebind_time = T2;
					get_dhcp_o_value(m_len, data, OPT51, option_value);
					lease->lease_time = (*(uint *) option_value);
					get_dhcp_o_value(m_len, data, OPT54, option_value);
					lease->server_ip = (*(uint *) option_value);
					lease->client_ip = inet_addr(s_ip);
					new_lease(lease, lease_filename);

					struct in_addr addr;
					addr.s_addr = lease->server_ip;
					memcpy(d_ip, inet_ntoa(addr), IP_MAX_STR_SIZE);
					printf("\t <--- offer received!\n");
					xid = rand();
					m_len = compose_request(xid, s_mac, data, s_ip, d_ip);
					if (send_l2_raw_message(sfd, m_len, data, IP_NULL, IP_BROAD,
							interface_name, s_mac, d_mac) < 0) {
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
			get_dhcp_o_value(m_len, data, OPT53, option_value);
			type_received = (*(char *) option_value);
			switch (type_received) {
				case TYPE_DHCPACK:
					get_dhcp_o_value(m_len, data, OPT58, option_value);
					if (debug == DBG10)
						T1 = 5;
					else
						T1 = ntohl((*(uint32_t *)option_value));
					get_dhcp_o_value(m_len, data, OPT59, option_value);
					T2 = ntohl((*(uint32_t *)option_value));
					lease->renew_time = T1;
					lease->rebind_time = T2;
					get_dhcp_o_value(m_len, data, OPT51, option_value);
					lease->lease_time = (*(uint *) option_value);
					printf("\t <--- ACK received\n");
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
			get_current_ip(interface_name, s_ip);
			if (inet_addr(s_ip) != lease->client_ip) {
				printf("current address doesn't match\n");
				//add addr
			}
			stop = times(&e_buf);
			timer = (stop - start)/CPS;
			//DHCPOFFER, DHCPACK, DHCPNAK/Discard
			sleep(1);
			if (debug == DBG10) printf("T1 - timer = %d\n", (T1 - timer));
		}
		printf("Bound timeout\n");

		//send dhcp_request to leasing_server
		xid = rand();
		m_len = compose_request(xid, s_mac, data, s_ip, d_ip);
		if (send_l2_raw_message(sfd, m_len, data, IP_NULL, IP_BROAD,
				interface_name, s_mac, d_mac) < 0) {
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
    		get_dhcp_o_value(m_len, data, OPT53, option_value);
    		type_received = (*(char *) option_value);

			switch (type_received) {
				case TYPE_DHCPACK:
					get_dhcp_o_value(m_len, data, OPT58, option_value);
					if (debug == DBG10)
						T1 = 5;
					else
						T1 = ntohl((*(uint32_t *)option_value));
					get_dhcp_o_value(m_len, data, OPT59, option_value);
					T2 = ntohl((*(uint32_t *)option_value));
					lease->renew_time = T1;
					lease->rebind_time = T2;
					get_dhcp_o_value(m_len, data, OPT51, option_value);
					lease->lease_time = (*(uint *) option_value);
					printf("\t <--- ACK received\n");
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
			timer = (stop - start)/CPS;
    	} while (timer < T2);
    	//broadcast DHCPREQUEST
		xid = rand();
		m_len = compose_request(xid, s_mac, data, s_ip, d_ip);
		if (send_l2_raw_message(sfd, m_len, data, IP_NULL, IP_BROAD,
				interface_name, s_mac, d_mac) < 0)
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
			get_dhcp_o_value(m_len, data, OPT53, option_value);
			type_received = (*(char *) option_value);

			switch (type_received) {
				case TYPE_DHCPACK:
					get_dhcp_o_value(m_len, data, OPT51, option_value);
					lease->lease_time = *((uint *) option_value);
					get_dhcp_o_value(m_len, data, OPT58, option_value);
					if (debug == DBG10)
						T1 = 5;
					else
						T1 = ntohl((*(uint32_t *)option_value));
					get_dhcp_o_value(m_len, data, OPT59, option_value);
					T2 = ntohl((*(uint32_t *)option_value));
					lease->renew_time = T1;
					lease->rebind_time = T2;
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
			timer = (stop - start)/CPS;
    	} while (timer < lease->lease_time);
		//broadcast DHCPREQUEST
		return STATE_REBINDING;
    }

    if (daemonize) {
    	printf("will daemonize\n");
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
            	//CLI --- DHCP Discover --> SRV
            	state = state_init();
				break;
            case STATE_SELECTING:
            	//CLI <-- DHCP Offer    --- SRV
            	state = state_selecting();
				break;
            case STATE_REQUESTING:
            	//CLI --- DHCP Request  --> SRV
            	//CLI <-- DHCP Ack      --- SRV
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
        memset(data, 0, PDU_DHCP_MAX);
        memset(frame, 0, MAX_ETH_F);
    }
    free(data);
    free(lease);
    return 1;
}
