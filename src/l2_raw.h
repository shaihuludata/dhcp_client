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

int send_l2_raw_message(int data_len, char * data, char source_ip[32], char destination_ip[32], char int_name[10]);

#endif /* L2_RAW_H_ */
