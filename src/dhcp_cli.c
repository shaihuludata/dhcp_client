/*
 ============================================================================
 Name        : dhcp_cli.c
 Author      : MDS
 Version     : 0.1
 Description : DHCP client on raw UDP sockets
 ============================================================================
 */

#include <stdio.h>
#include <sys/socket.h>
#include <time.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/random.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <syslog.h>
#include "dhcp.c"
#include "options.h"

typedef struct
{
	char set; //if set == 1, structure exists
	dhcp_ * data;
} message;

int send_message(message M);

int main(int argc, char * argv[])
{
//    int sfd = socket(AF_INET, SOCK_RAW, IPPROTO_UDP);
    int state = STATE_INIT;
    int seed = time(NULL);
    srand(seed);
    int T1 = rand() % 10 + 1;
//    int T2 = 0;
    int timer = 0;

    struct tms b_buf, e_buf;
    int start = times(&b_buf);
    int stop;
    message M;

//    setlogmask (LOG_UPTO (LOG_NOTICE));
//    openlog ("exampleprog", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);
//    syslog (LOG_NOTICE, "Program started by User %d", getuid ());
//    syslog (LOG_INFO, "A tree falls in a forest");
//    closelog ();

    /*
     * CLI --- DHCP Discover --> SRV
     * CLI <-- DHCP Offer    --- SRV
     * CLI --- DHCP Request  --> SRV
     * CLI <-- DHCP Ack      --- SRV
    */

    while (1)
    {
        switch (state)
        {
            case STATE_INIT:
                M.set = 0;
                M.data = compose_discover(1);

                stop = times(&e_buf);
                timer = (stop - start)/100;
                if (timer >= T1)
                {
                    if (send_message(M) == 1)
                    {
                        M.set = 1;
                        state = STATE_SELECTING;
                    }
                    else
                    {
                    	perror("Failed to send message");
                    }
                }
                break;
            case STATE_SELECTING:
                break;
            case STATE_REQUESTING:
                break;
            case STATE_BOUND:
                break;
            case STATE_REBINDING:
                break;
            case STATE_RENEWING:
                break;
        }
        sleep(1);
    }
    return 1;
}

int send_message(message M)
{
    return 1;
    return 0;
}

