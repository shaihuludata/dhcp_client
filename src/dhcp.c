#include <arpa/inet.h> //inet_addr
#include <string.h>

#include "options.h"
#include "dhcp.h"

//int common_overhead(char op, dhcp_ * data) {
//	return 1;
//}

//size_t compose_discover(int xid) {
//	dhcp_ data;
//	data.op = OP_DISCOVER;
//	data.htype = HTYPE;
//	data.hlen = HLEN;
//	data.hops = 0;
//	data.xid = xid;
//	data.secs = 0;
//	data.flags = 0;
//	data.ciaddr = inet_addr("0.0.0.0");
//	data.yiaddr = inet_addr("0.0.0.0");
//	data.siaddr = inet_addr("0.0.0.0");
//	data.giaddr = inet_addr("0.0.0.0");
////	data.chaddr[16];
////	data.sname[128];
//
////	option option_53;
////	option_53.code = 53;
////	option_53.len = 1;
////	option_53.data = DHCPDISCOVER;
////	data.options[0] = option_53;
//
////	struct in_addr addr;
////	option option_50;
////	option_50.code = 50;
////	option_50.len = 4;
////	option_50.data = inet_aton("0.0.0.0", &addr);
////	data.options[1] = option_50;
//	message M = {.len = sizeof(data), .data = (char *)&data};
//    return M;
//}

//int dispatch_offer(void * arg)
//{
//	dhcp_ data = common_overhead(arg);
//	return data;
//}
//
unsigned int compose_request(int xid, char * source_mac, void * buf)
{
	dhcp_ * data = (dhcp_ *) buf;
	//memset (&data, 0, sizeof(data));
	data->op = OP_REQUEST;
	data->htype = HTYPE;
	data->hlen = HLEN;
	data->hops = 0;
	data->xid = xid;
	data->secs = 0;
	data->flags = 0;
	data->ciaddr = inet_addr("0.0.0.0");
	data->yiaddr = inet_addr("0.0.0.0");
	data->siaddr = inet_addr("0.0.0.0");
	data->giaddr = inet_addr("0.0.0.0");

	char source_mac_w_padding[16];
	char padding[10] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
	strncpy(source_mac_w_padding, (char * )source_mac, HLEN);
	strncpy(source_mac_w_padding + HLEN, padding, sizeof(padding) - 1);
	strncpy(data->chaddr, source_mac_w_padding, sizeof(source_mac_w_padding) - 1);
	//char data->sname[64];
	memset(data->sname, 0, sizeof(data->sname));
	//char data->sname[128];
	memset(data->file, 0, sizeof(data->file));
	//236

	//memset(data->options, 0, sizeof(options_Request));
	// = {.code = 53, .len = 1, .data = TYPE_DHCPREQUEST};
	//3+6+6+4 = 19
	options_Request * options = (options_Request *)  (buf+sizeof(*data));
	//option53= {.code = 53, .len = 1, .data = TYPE_DHCPREQUEST};
	options->cookie = MAGIC_COOKIE_DHCP;
	options->option_53.code = 53;
	options->option_53.len = 1;
	options->option_53.data = TYPE_DHCPREQUEST;
	options->option_50.code = 50;
	options->option_50.len = 4;
	options->option_50.data = inet_addr("0.0.0.0"); // TODO: исправить
	options->option_54.code = 54;
	options->option_54.len = 4;
	options->option_54.data = inet_addr("0.0.0.0"); // TODO: исправить
	options->option_255 = 0xFF;

	size_t total_size = sizeof(*data) + sizeof(*options);
	return total_size; // 255
}
//
//dhcp_ * dispatch_ack(void * arg)
//{
//	dhcp_ data = common_overhead(arg);
//	return data;
//}
