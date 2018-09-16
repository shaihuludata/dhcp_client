#include <arpa/inet.h> //inet_addr
#include <string.h>
#include <stdio.h>

#include "l2_raw.h"
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

unsigned int compose_request(int xid, char * source_mac, void * buf, char req_ip[4], char req_srv[15])
{
	dhcp_ * data = (dhcp_ *) buf;

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
	options_Request * options = (options_Request *)  (buf+sizeof(*data));
	options->cookie = MAGIC_COOKIE_DHCP;
	options->option_53.code = 53;
	options->option_53.len = 1;
	options->option_53.data = TYPE_DHCPREQUEST;
	options->option_50.code = 50;
	options->option_50.len = 4;
	options->option_50.data = inet_addr(req_ip); // TODO: исправить
	options->option_54.code = 54;
	options->option_54.len = 4;
	options->option_54.data = inet_addr(req_srv); // TODO: исправить
	options->option_55.code = 55;
	options->option_55.len = 4;
	options->option_55.data = 0x2a060301; // 1 - subnet, 3 - router, 6 - dns, 42 - ntps
	options->option_255 = 0xFF;
	// 255
	size_t total_size = sizeof(*data) + sizeof(*options);
	return total_size;
}

char dispatch_ack(message * M) {
	char ret = 0;
	struct opt {char T; char L; char *V;};
	struct opt * opts[10];
	//dhcp_ * data = (dhcp_ *) M->data;
	char * options = M->data + sizeof(dhcp_) + sizeof(int);  // int - magic cookie
	int num_of_opts = 0;
	int opt_size = M->len - (sizeof(dhcp_) + sizeof(int));
	printf("sdjfnjkfnjka %d\n", (int)sizeof(void));
	for (int i=0; i < opt_size; ) {
		struct opt opt;
		opt.T = options[i];
		opts[num_of_opts] = &opt;
		if (opt.T == 0xFF) break;
		opt.L = options[i+1];
		opt.V = &options[i+2];
		i += opt.L + sizeof(opt.T);
		if ((opt.T == 53) && (opt.L == 1)) ret = *opt.V;
	}
	for (int i; i<10; i++) printf("%p\n", opts[i]);
	return ret; //TODO исправить
}
