/* DreamShell ##version##

   main.c - Ping
   Copyright (C) 2009 Lawrence Sebald
   Copyright (C) 2009-2019 SWAT

*/
#include "ds.h"
#include <netdb.h>

#define DATA_SIZE 56

int main(int argc, char *argv[]) {

    if(argc < 2) {
		ds_printf("Usage: %s host\n", argv[0]);
        return CMD_NO_ARG;
    }

    uint8 ip[4];
    int i, scan_ip[4] = { 0, 0, 0 ,0 };
    uint8 data[DATA_SIZE];
    char *name = argv[1];
    struct hostent *hent;

    i = sscanf(name, "%d.%d.%d.%d", &scan_ip[0], &scan_ip[1], &scan_ip[2], &scan_ip[3]);

    if (i < 4) {
        hent = gethostbyname((const char *)name);

        if(hent) {
            name = hent->h_addr;
            ds_printf("DS_OK: %s\n", hent->h_name);
        } else {
            ds_printf("DS_ERROR: Can't lookup host %s\n", name);
            return CMD_ERROR;
        }

        i = sscanf(name, "%d.%d.%d.%d", &scan_ip[0], &scan_ip[1], &scan_ip[2], &scan_ip[3]);

        if (i < 4) {
            ds_printf("DS_ERROR: Bad format: %d %s\n", i, name);
            return CMD_ERROR;
        }
    }

    for(i = 0; i < 4; ++i) {
        ip[i] = (uint8)(scan_ip[i] & 0xff);
    }

    /* Fill in the data for the ping packet... this is pretty simple and doesn't
       really have any real meaning... */
    for(i = 0; i < DATA_SIZE; ++i) {
        data[i] = (uint8)i;
    }

    ds_printf("DS_PROCESS: Connecting to %d.%d.%d.%d\n", ip[0], ip[1], ip[2], ip[3]);

    /* Send out 4 pings, waiting 250ms between attempts. */
    for(i = 0; i < 4; ++i) {
        if (net_icmp_send_echo(net_default_dev, ip, 0, i, data, DATA_SIZE) < 0) {
            ds_printf("DS_ERROR: send echo error %d\n", errno);
            return CMD_ERROR;
        }
        thd_sleep(250);
    }

    ds_printf("DS_OK: Done.\n");
    return CMD_OK;
}
