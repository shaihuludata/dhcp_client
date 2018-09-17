#include <arpa/inet.h> //inet_addr
#include <string.h>
#include <stdio.h>

#include "l2_raw.h"
#include "options.h"
#include "dhcp.h"

//int common_overhead(char op, dhcp_ * data) {
//	return 1;
//}

unsigned int compose_discover(int xid, char * source_mac, void * buf, char req_ip[4], char req_srv[15])
{
	dhcp_ * data = (dhcp_ *) buf;

	data->op = OP_DISCOVER;
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
	//char data->sname[64];
	memset(data->sname, 0, sizeof(data->sname));
	//char data->sname[128];
	memset(data->file, 0, sizeof(data->file));

	options_Discover * options = (options_Discover *)  (buf+sizeof(*data));
	options->cookie = MAGIC_COOKIE_DHCP;
	options->option_53.code = 53;
	options->option_53.len = 1;
	options->option_53.data = TYPE_DHCPDISCOVER;

	options->option_61.code = 61;
	options->option_61.len = 7;
	options->option_61.data[0] = 0x01;
	strncpy(options->option_61.data+1, source_mac, HLEN);

	options->option_50.code = 50;
	options->option_50.len = 4;
	options->option_50.data = inet_addr(req_ip);

	options->option_55.code = 55;
	options->option_55.len = 4;
	options->option_55.data = 0x2a060301; // 1 - subnet, 3 - router, 6 - dns, 42 - ntps
	options->option_255 = 0xFF;

	size_t total_size = sizeof(*data) + sizeof(*options);
    return total_size;
}

//int dispatch_offer(void * arg)
//{
//	dhcp_ data = common_overhead(arg);
//	return data;
//}

unsigned int compose_request(int xid, char * source_mac, void * buf, char req_ip[4], char req_srv[15]) {
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
	options->option_50.data = inet_addr(req_ip);
	options->option_54.code = 54;
	options->option_54.len = 4;
	options->option_54.data = inet_addr(req_srv);
	options->option_55.code = 55;
	options->option_55.len = 4;
	options->option_55.data = 0x2a060301; // 1 - subnet, 3 - router, 6 - dns, 42 - ntps
	options->option_255 = 0xFF;
	// 255
	size_t total_size = sizeof(*data) + sizeof(*options);
	return total_size;
}

int match_xid (char * data, int xid) {
	dhcp_ * head = (dhcp_ *) data;
	if (head->xid != xid) return 0;
	//printf("xid %x\n", xid);//ntohs() != xid
	return 1;
}

char dispatch_ack(int len, char * data) {
	char ret = 0;
	//dhcp_ * data = (dhcp_ *) M->data;
	char * options = data + sizeof(dhcp_) + sizeof(int);  // int - magic cookie
	int num_of_opts = 0;

	struct opt {char T; char L; char *V;};
	struct opt * opts[10];
	int opt_size = len - (sizeof(dhcp_) + sizeof(int));
	for (int i=0; i < opt_size; ) {
		struct opt opt;
		opt.T = options[i];
		opts[num_of_opts] = &opt;
		if (opt.T == 255) break;
		opt.L = options[i+1];
		opt.V = &options[i+2];
		i += opt.L + sizeof(opt.T);
		if ((opt.T == 53) && (opt.L == 1)) ret = *opt.V;
	}
	for (int i; i<10; i++) printf("%p ", opts[i]);
	return ret; //TODO исправить
}

//char dispatch_offer(message * M, int xid) {
