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
#include <sys/syslog.h>
#include "dhcp.h"
#include "options.h"
#include "udp_raw.h"

#define CPS 40

int main(int argc, char * argv[])
{
    int state = STATE_INIT;
    int seed = time(NULL);
    srand(seed);
    int T1 = rand() % 10 + 1;
//    int T2 = 0;
    int timer = 0;
	dhcp_ data;

    clock_t start = clock();
    clock_t stop;

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
            	data = compose_discover(1);
                stop =  clock();
                timer = (stop - start)/CPS; //CLOCKS_PER_SEC;
                timer = 10;
                if (timer >= T1)
                {
					if (send_raw_udp_message(sizeof(data), (char *)&data, "0.0.0.0", "255.255.255.255") < 0)
						perror("Failed to send message");
					else
					{
						perror("Message sent");
						state = STATE_SELECTING;
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
