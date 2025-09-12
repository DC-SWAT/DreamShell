/* DreamShell ##version##

   main.c - NTP
   Copyright (C) 2023 Eric Fradella
   Copyright (C) 2023 SWAT

*/
#include "ds.h"
#include <netdb.h>
#include <arch/rtc.h>

#define NTP_PORT    "123"
#define NTP_SERVER  "us.pool.ntp.org"
#define NTP_DELTA   2208988800ULL

/* Structure for 48-byte NTP packet */
typedef struct ntp_packet {
    uint8_t leap_ver_mode;    /* First 2 bits are leap indicator, next
                                 three bits NTP version, last 3 bits mode */
    uint8_t stratum;
    uint8_t poll_interval;
    uint8_t precision;
    uint32_t root_delay;
    uint32_t root_dispersion;
    uint32_t ref_id;
    uint32_t ref_time_s;
    uint32_t ref_time_f;
    uint32_t orig_time_s;
    uint32_t orig_time_f;
    uint32_t rcv_time_s;
    uint32_t rcv_time_f;
    uint32_t trns_time_s;
    uint32_t trns_time_f;
} ntp_packet_t;

int main(int argc, char **argv) {
    ntp_packet_t packet;
    int sockfd;
    struct addrinfo *ai;
    time_t ntp_time, dc_time;
    int sync = 0;

    if(argc < 2) {
        ds_printf("Usage: %s options args...\n"
            "Options: \n"
            " -s, --sync  -Sync system time\n\n"
            "Example: %s -s\n", argv[0]);
        return CMD_NO_ARG; 
    }

	struct cfg_option options[] = {
		{"sync", 's', NULL, CFG_BOOL, (void *) &sync, 0},
		CFG_END_OF_LIST
	};

  	CMD_DEFAULT_ARGS_PARSER(options);

    if(!sync) {
        return CMD_NO_ARG;
    }

    ds_printf("DS_PROCESS: Connecting...\n");

    /* Create NTP packet and clear it */
    memset(&packet, 0, sizeof(ntp_packet_t));

    /* Leave leap indicator blank, set version number to 4,and
       set client mode to 3. 0x23 = 00 100 011 = 0, 4, 3 */
    packet.leap_ver_mode = 0x23;

    /* Create a new UDP socket */
    if((sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
        ds_printf("DS_ERROR: Opening socket failed.\n");
        return CMD_ERROR;
    }

    /* Retrieve IP address for our specified hostname */
    if(getaddrinfo(NTP_SERVER, NTP_PORT, NULL, &ai)) {
        ds_printf("DS_ERROR: Resolving host failed.\n");
        return CMD_ERROR;
    }

    if(connect(sockfd, ai->ai_addr, ai->ai_addrlen) < 0) {
        ds_printf("DS_ERROR: Connecting to server failed.\n");
        return CMD_ERROR;
    }

    freeaddrinfo(ai);

    /* Send the NTP packet we constructed */
    if(write(sockfd, &packet, sizeof(ntp_packet_t)) < 0) {
        ds_printf("DS_ERROR: Writing to socket failed.\n");
        return CMD_ERROR;
    }

    /* Receive the packet back from the server,
       now filled out with the current time */
    if(read(sockfd, &packet, sizeof(ntp_packet_t)) < 0) {
        ds_printf("DS_ERROR: Reading response from socket failed.\n");
        return CMD_ERROR;
    }

    /* Grab time from the structure, and subtract 70 years to convert
       from NTP's 1900 epoch to Unix time's 1970 epoch */
    ntp_time = (ntohl(packet.trns_time_s) - NTP_DELTA);
    
    Settings_t *settings = GetSettings();
    ntp_time += settings->time_zone * 60;

    struct tm *time_info;
    char time_str[80];

    time_info = localtime(&ntp_time);
    strftime(time_str, sizeof(time_str), "%c", time_info);
    ds_printf("DS_INFO: NTP time: %s\n", time_str);

    /* Print the current system time */
    dc_time = rtc_unix_secs();
    time_info = localtime(&dc_time);
    strftime(time_str, sizeof(time_str), "%c", time_info);
    ds_printf("DS_INFO: Old system time: %s\n", time_str);

    /* Set the system time to the NTP time and read it back */
    rtc_set_unix_secs(ntp_time);
    dc_time = rtc_unix_secs();
    time_info = localtime(&dc_time);
    strftime(time_str, sizeof(time_str), "%c", time_info);
    ds_printf("DS_OK: New system time: %s\n", time_str);

    return CMD_OK;
}
