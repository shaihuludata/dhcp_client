#define TIMER_TICS_IN_SECOND

#define STATE_INIT_REBOOT	0
#define STATE_REBOOTING		1
#define STATE_INIT			2
#define STATE_SELECTING		3
#define STATE_REQUESTING	4
#define STATE_BOUND			5
#define STATE_REBINDING		6
#define STATE_RENEWING		7

#define HTYPE 1
#define HLEN 6
#define PLEN 4

#define TIMEOUT_DISCOVER_SEC 3
#define TIMEOUT_REQUESTING_SEC 3
#define TIMEOUT_BOUND_SEC 3

#define IP_NULL "0.0.0.0"
#define IP_BROAD "255.255.255.255"

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

#pragma pack(push, 1)
typedef struct
{
    char code;
    char len;
    int data;
} option_50;

typedef struct
{
    char code;
    char len;
    unsigned int data;
} option_51;

typedef struct
{
    char code;
    char len;
    char data;
} option_53;

typedef struct
{
    char code;
    char len;
    int data;
} option_54;

typedef struct
{
    char code;
    char len;
    int data;
} option_55;

typedef struct
{
	int cookie;
    option_53 option_53;
    option_50 option_50;
    option_54 option_54;
    option_55 option_55;
    char option_255;
} options_Request;
#pragma pack(pop)
