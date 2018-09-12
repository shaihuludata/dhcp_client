#define UDP_HEADER_SIZE 8

int send_raw_message(int data_len, char * data, char source_ip[32], char destination_ip[32]);

unsigned short csum(unsigned short *ptr,int nbytes);

struct pseudo_header
{
    u_int32_t source_address;
    u_int32_t dest_address;
    u_int8_t placeholder;
    u_int8_t protocol;
    u_int16_t udp_length;
} ;
