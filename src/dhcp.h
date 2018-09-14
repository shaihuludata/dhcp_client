#define DHCP_PORT_SRV 67
#define DHCP_PORT_CLI 68

#define PDU_DHCP_MAX 576

#define DHCPDISCOVER 1
#define DHCPOFFER    2
#define DHCPREQUEST  3
#define DHCPDECLINE  4
#define DHCPACK      5
#define DHCPNAK      6
#define DHCPRELEASE  7
#define DHCPINFORM   8

#define TYPE_DHCPDISCOVER	1
#define TYPE_DHCPOFFER		2
#define TYPE_DHCPREQUEST	3
#define TYPE_DHCPDECLINE	4
#define TYPE_DHCPACK		5
#define TYPE_DHCPNAK		6
#define TYPE_DHCPRELEASE	7
#define TYPE_DHCPINFORM		8

#define OP_DISCOVER 1
#define OP_REQUEST 1

#define MAGIC_COOKIE_DHCP	1666417251

typedef struct
{
	unsigned int len;
	char * data;
} message;

typedef struct
{
    char code;
    char len;
    int data;
} option;

typedef struct
{
    char op;
    char htype;
    char hlen;
    char hops;
    int xid;
    short secs;
    short flags;
    int ciaddr;
    int yiaddr;
    int siaddr;
    int giaddr;
    char chaddr[16];
    char sname[64];
    char file[128];
} dhcp_;

//int common_overhead(char op, dhcp_ * data);

unsigned int compose_discover(int xid);
unsigned int compose_request(int xid, char * source_mac, void * buf);

//dhcp_ compose_discover(int a);
//dhcp_ * dispatch_discover(void * arg);
