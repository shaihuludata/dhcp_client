#include "options.h"

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
    char sname[128];
    option * options[];
} dhcp_;

//dhcp_ * compose_discover(void * arg);
//dhcp_ * dispatch_discover(void * arg);

int common_overhead(char op, dhcp_ * data)
{
	data->op = op;
    data->htype = HTYPE;
    data->hlen = HLEN;
	return 1;
}

dhcp_ compose_discover(int a)
{
	dhcp_ data;
	common_overhead(OP_DISCOVER, &data);

//	option option_53;
//	option_53.code = 53;
//	option_53.len = 1;
//	option_53.data = DHCPDISCOVER;
//	data.options[0] = option_53;

//	struct in_addr addr;
//	option option_50;
//	option_50.code = 50;
//	option_50.len = 4;
//	option_50.data = inet_aton("0.0.0.0", &addr);
//	data.options[1] = option_50;

    return data;
}

//int dispatch_offer(void * arg)
//{
//	dhcp_ data = common_overhead(arg);
//	return data;
//}
//
//dhcp_ * compose_request(void * arg)
//{
//	dhcp_ data = common_overhead(arg);
//	return data;
//}
//
//dhcp_ * dispatch_ack(void * arg)
//{
//	dhcp_ data = common_overhead(arg);
//	return data;
//}
