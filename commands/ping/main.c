/* DreamShell ##version##

   main.c - Ping
   Copyright (C) 2009 Lawrence Sebald
   Copyright (C) 2009-2014 SWAT

*/
            
#include "ds.h"


#define DATA_SIZE 56

/*
static void icmp_ds_echo_cb(const uint8 *ip, uint16 seq, uint64 delta_us,
                                 uint8 ttl, const uint8* data, int data_sz) {
    ds_printf(" %d bytes from %d.%d.%d.%d: icmp_seq=%d ttl=%d time=%.3f ms\n",
           data_sz, ip[0], ip[1], ip[2], ip[3], seq, ttl,
           delta_us / 1000.0);
}*/


int main(int argc, char *argv[]) { 
     
    if(argc < 2) {
		ds_printf("Usage: %s ip_addr\n", argv[0]);
        return CMD_NO_ARG; 
    } 

	//net_icmp_echo_cb = icmp_ds_echo_cb;

	/* The address to ping... */
    uint8 addr[4] = { 192, 168, 0, 1 };
    uint8 data[DATA_SIZE];
    int i;

	sscanf(argv[1], "%c.%c.%c.%c", &addr[0], &addr[1], &addr[2], &addr[3]);

    /* Fill in the data for the ping packet... this is pretty simple and doesn't
       really have any real meaning... */
    for(i = 0; i < DATA_SIZE; ++i) {
        data[i] = (uint8)i;
    }

    /* Send out 4 pings, waiting 250ms between attempts. */
    for(i = 0; i < 4; ++i) {
        net_icmp_send_echo(net_default_dev, addr, 0, i, data, DATA_SIZE);
        thd_sleep(250);
    }

	//net_icmp_echo_cb = icmp_default_echo_cb;

    return CMD_OK; 
}


