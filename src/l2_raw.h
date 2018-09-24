/*
 * l2_raw.h
 *
 *  Created on: 12 сент. 2018 г.
 *      Author: sw
 */

#ifndef L2_RAW_H_
#define L2_RAW_H_

#define HTYPE 1
#define HLEN 6
#define PLEN 4

#define MAC_BCAST {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
#define IP_MAX_STR_SIZE 15
#define IP_NULL "0.0.0.0"
#define IP_BROAD "255.255.255.255"

#define MAX_ETH_F	1514
#define L2_HDR_SIZE 14
#define L3_HDR_SIZE 20
#define L4_HDR_SIZE 8

typedef struct {
	int len;
	void * data;
} message;

struct pseudo_header {
    u_int32_t source_address;
    u_int32_t dest_address;
    u_int8_t placeholder;
    u_int8_t protocol;
    u_int16_t udp_length;
};

int get_current_mac(char * interface_name, char * s_mac);
int get_current_ip(char * interface_name, char * s_ip);
int set_current_ip(char * interface_name, char * s_ip, char * mask);

int init_socket (int * sfds);
//int deinit_socket(int sfds[3]);
int send_l2_raw_message(int sfd, unsigned int data_len, char * payload, char source_ip[32], char destination_ip[32],
		char interface_name[10], char source_mac[6], char destination_mac[6]);

//void * recv_l2_raw_message(int fd, message * M);
int recv_l2_raw_message(int fd, char * data, char * frame, int (*matcher)(char *, int), int match_arg, int timeout);

#endif /* L2_RAW_H_ */
