/* KallistiOS ##version##

   libmp3 main.c
   (c)2000-2001 Dan Potter
    
	2011 Modified by SWAT
*/

#include <kos.h>
#include "mp3.h"
/*
void *sndserver_thread(void *blagh) {
	printf("snd_mp3_server: started\r\n");
	printf("snd_mp3_server: pid is %d; capabilities: MP1, MP2, MP3\r\n", thd_get_current()->tid);

	sndmp3_mainloop();
	
	printf("snd_mp3_server: exited\r\n");
	return NULL;
}*/


int mp3_init() {
	/*
	if (snd_stream_init() < 0)
		return -1;
	if (thd_create(1, sndserver_thread, NULL) != NULL) {
		sndmp3_wait_start();
		return 0;
	} else
		return -1;
	*/
	return 0;
}

int mp3_start(const char *fn, int loop) {
	return sndmp3_start(fn, loop); 
	//return spu_mp3_dec(fn); 
	//sndmp3_start(fn, loop);
}

int mp3_stop() {
	return sndmp3_stop();
	//sndmp3_stop();
	//return 0;
}

int mp3_shutdown() {
	//sndmp3_shutdown();
	return 0;
}

void mp3_volume(int vol) {
	//sndmp3_volume(vol);
}
