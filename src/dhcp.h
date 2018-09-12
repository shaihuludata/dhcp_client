#define DHCP_PORT_SRV 67
#define DHCP_PORT_CLI 68

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

int common_overhead(char op, dhcp_ * data);

dhcp_ compose_discover(int a);

//dhcp_ compose_discover(int a);
//dhcp_ * dispatch_discover(void * arg);
