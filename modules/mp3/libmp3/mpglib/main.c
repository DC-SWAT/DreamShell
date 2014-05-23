#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/soundcard.h>
#include "config.h"
#include "mpg123.h"
//#include "mpglib.h"
#include "output.h"
#include "interface.h"

char smp_buf[16384];
struct mpstr_tag mp;
struct audio_out *audio_out = &oss_out;

int main(int argc,char **argv)
{
	int size;
	char out[8192];
	int len,ret;
	int f;
	void *fo;

	f = open(argv[1], O_RDONLY);
	fo = audio_out->open(NULL, 44100, 2, AFMT_S16_LE, 0, 8192);

	printf("f %d fo %d\n", f, fo);

	InitMP3(&mp);

	while(1) {
		len = read(f,smp_buf,16384);
		if(len <= 0)
			break;
		ret = decodeMP3(&mp,smp_buf,len,out,8192,&size);
		while(ret == MP3_OK) {
		//fprintf(stderr, "Sampling Frequency: %d Hz \r",freqs[mp.fr.sampling_frequency]);
			if (ret == MP3_OK)audio_out->write((void *)fo,out,size);
			ret = decodeMP3(&mp,NULL,0,out,8192,&size);
		}
	}

	close(f);
	audio_out->close(audio_out);//close(fo);

  return 0;

}

