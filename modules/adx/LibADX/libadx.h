#ifndef ADX_H
#define ADX_H

#define ADX_CRI_SIZE 0x06
#define ADX_PAD_SIZE 0x0e
#define ADX_HDR_SIZE 0x2c
#define ADX_HDR_SIG  0x80
#define ADX_EXIT_SIG 0x8001

#define ADX_ADDR_START      0x02
#define ADX_ADDR_CHUNK      0x05
#define ADX_ADDR_CHAN       0x07
#define ADX_ADDR_RATE       0x08
#define ADX_ADDR_SAMP       0x0c
#define ADX_ADDR_TYPE       0x12
#define ADX_ADDR_LOOP       0x18
#define ADX_ADDR_SAMP_START 0x1c
#define ADX_ADDR_BYTE_START 0x20
#define ADX_ADDR_SAMP_END   0x24
#define ADX_ADDR_BYTE_END   0x28

typedef struct
{
    int sample_offset;              
    int chunk_size;
    int channels;
    int rate;
    int samples;
    int loop_type;
    int loop;
    int loop_start;
    int loop_end;
    int loop_samp_start;
    int loop_samp_end;
    int loop_samples;
}ADX_INFO;

typedef struct {
	int s1,s2;
} PREV;

/* LibADX Public Function Definitions */
/* Return 1 on success, 0 on failure */

/* Start Straming the ADX in a seperate thread */
int adx_dec( char * adx_file, int loop_enable );

/* Stop Streaming the ADX */
int adx_stop();

/* Restart Streaming the ADX */
int adx_restart();

/* Pause Streaming the ADX (if streaming) */
int adx_pause();

/* Resume Streaming the ADX (if paused) */
int adx_resume();

#endif
