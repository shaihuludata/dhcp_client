#define TIMER_TICS_IN_SECOND

#define PORT_SRV 67
#define PORT_CLI 68

#define STATE_INIT  0
#define STATE_SELECTING 1
#define STATE_REQUESTING 2
#define STATE_BOUND 3
#define STATE_REBINDING 4
#define STATE_RENEWING 5

#define HTYPE 1
#define HLEN 6

#define DHCPDISCOVER 1
#define DHCPOFFER    2
#define DHCPREQUEST  3
#define DHCPDECLINE  4
#define DHCPACK      5
#define DHCPNAK      6
#define DHCPRELEASE  7
#define DHCPINFORM   8

#define OP_DISCOVER 1


//typedef struct
//{
//    void * option_53;
//    void * option_50;
//} OH_Discover;
//
//typedef struct
//{
//    void * option_53;
//    void * option_1;
//    void * option_3;
//    void * option_51;
//    void * option_54;
//} OH_Offer;
//
//typedef struct
//{
//    void * option_53;
//    void * option_50;
//    void * option_54;
//} OH_Request;
//
//typedef struct
//{
//    void * option_53;
//    void * option_1;
//    void * option_3;
//    void * option_51;
//    void * option_54;
//} OH_Ack;
//
//
//
//
//
//
//
//
///*
// *  Code   Len        Subnet Mask
// * +-----+-----+-----+-----+-----+-----+
// * |  1  |  4  |  m1 |  m2 |  m3 |  m4 |
// * +-----+-----+-----+-----+-----+-----+
//*/
//typedef struct
//{
//    char code = 1;
//    char len = 4;
//    int data;
//} option_1;
//
//typedef struct
//{
//    char code = 3;
//    char len;
//    int data;
//} option_3;
//
//typedef struct
//{
//    char code = 50;
//    char len = 4;
//    int data;
//} option_50;
//
//typedef struct
//{
//    char code = 51;
//    char len = 4;
//    unsigned int data;
//} option_51;
//
//
//typedef struct
//{
//    char code = 53;
//    char len = 1;
//    int data;
//} option_53;
//
//typedef struct
//{
//    char code = 54;
//    char len = 4;
//    int data;
//} option_54;
