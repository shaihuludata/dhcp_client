/*
 * l2_raw.h
 *
 *  Created on: 12 сент. 2018 г.
 *      Author: sw
 */

#ifndef L2_RAW_H_
#define L2_RAW_H_

#define L2_HDR_SIZE 14
#define L3_HDR_SIZE 20
#define L4_HDR_SIZE 8

struct pseudo_header
{
    u_int32_t source_address;
    u_int32_t dest_address;
    u_int8_t placeholder;
    u_int8_t protocol;
    u_int16_t udp_length;
};

int send_l2_raw_message(unsigned int data_len, char * payload, char source_ip[32], char destination_ip[32],
		char interface_name[10], char source_mac[6], char destination_mac[6]);

#endif /* L2_RAW_H_ */
