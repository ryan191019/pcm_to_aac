//File   : lrecord.c  
//Author : Loon <sepnic@gmail.com>  
  
#include <stdio.h>  
#include <malloc.h>  
#include <unistd.h>  
#include <stdlib.h>  
#include <string.h>  
#include <getopt.h>  
#include <fcntl.h>  
#include <ctype.h>  
#include <errno.h>  
#include <limits.h>  
#include <time.h>  
#include <locale.h>  
#include <sys/unistd.h>   
#include <sys/stat.h>  
#include <sys/types.h>  
#include <assert.h>  
#include "wav_parser.h"  
#include "sndwav_common.h"  
#include "debug.h"

#include <fdk-aac/aacenc_lib.h>
#include <alsa/asoundlib.h>
#include "sndwav_common.h"
#include "wavreader.h"



extern HANDLE_AACENCODER handle;
extern AACENC_InfoStruct info;

WAVContainer_t wav;  
SNDPCMContainer_t record;  
int fd; 
off64_t g_len;
static char *outfile="/tmp/222.aac";
static char *filename="/tmp/snd.wav";


  
#define DEFAULT_CHANNELS         (2)  
#define DEFAULT_SAMPLE_RATE      (44100)  
#define DEFAULT_SAMPLE_LENGTH    (16)  
#define DEFAULT_DURATION_TIME    (4)  

#define	BUFFER_SIZE	4096*2
#define FRAME_LEN	640 
#define HINTS_SIZE  100

 
int SNDWAV_PrepareWAVParams(WAVContainer_t *wav)  
{  
    assert(wav);  
  
    uint16_t channels = DEFAULT_CHANNELS;  
    uint16_t sample_rate = DEFAULT_SAMPLE_RATE;  
    uint16_t sample_length = DEFAULT_SAMPLE_LENGTH;  
    uint32_t duration_time = DEFAULT_DURATION_TIME;  
  
    /* Const */  
    wav->header.magic = WAV_RIFF;  
    wav->header.type = WAV_WAVE;  
    wav->format.magic = WAV_FMT;  
    wav->format.fmt_size = LE_INT(16);  
    wav->format.format = LE_SHORT(WAV_FMT_PCM);  
    wav->chunk.type = WAV_DATA;  
  
    /* User definition */  
    wav->format.channels = LE_SHORT(channels);  
    wav->format.sample_rate = LE_INT(sample_rate);  
    wav->format.sample_length = LE_SHORT(sample_length);  
  
    /* See format of wav file */  
    wav->format.blocks_align = LE_SHORT(channels * sample_length / 8);  
    wav->format.bytes_p_second = LE_INT((uint16_t)(wav->format.blocks_align) * sample_rate);  
	
	g_len = duration_time*sample_rate * channels * sample_length / 8;
	//DEBUG_INFO(" sample_rate=%u", sample_rate);
	//DEBUG_INFO(" wav->format.blocks_align=%u", channels * sample_length / 8);
	DEBUG_INFO(" len=%lld",g_len); 
    wav->chunk.length = LE_INT(duration_time * (uint32_t)(wav->format.bytes_p_second));  
    wav->header.length = LE_INT((uint32_t)(wav->chunk.length) +  
        sizeof(wav->chunk) + sizeof(wav->format) + sizeof(wav->header) - 8);  
	//DEBUG_INFO(" wav->chunk.length=%u",LE_INT( wav->chunk.length));
    return 0;  
}  
  

void SNDWAV_Record(SNDPCMContainer_t *sndpcm, WAVContainer_t *wav, int fd)  
{  
    off64_t rest;  
    size_t c, frame_size;  
      
    if (WAV_WriteHeader(fd, wav) < 0) {  
        exit(-1);  
    }  
  
    rest = wav->chunk.length;  
    while (rest > 0) {  
        c = (rest <= (off64_t)sndpcm->chunk_bytes) ? (size_t)rest : sndpcm->chunk_bytes;  
        frame_size = c * 8 / sndpcm->bits_per_frame;  
        if (SNDWAV_ReadPcm(sndpcm, frame_size) != frame_size)  
            break;  
          
        if (write(fd, sndpcm->data_buf, c) != c) {  
           DEBUG_ERR( "Error SNDWAV_Record[write]/n");  
            exit(-1);  
        }  
  
        rest -= c;  


    }  
}  

int SNDWAV_Record_Aac(SNDPCMContainer_t *sndpcm, WAVContainer_t *wav, int fd)  
{  
	FILE *out;
	AACENC_BufDesc in_buf = { 0 }, out_buf = { 0 };
	AACENC_InArgs in_args = { 0 };
	AACENC_OutArgs out_args = { 0 };
	int in_identifier = IN_AUDIO_DATA;
	int in_size, in_elem_size;
	int out_identifier = OUT_BITSTREAM_DATA;
	int out_size, out_elem_size;
	int read, i;
	void *in_ptr, *out_ptr;
	uint8_t* input_buf;
	int16_t* convert_buf;
	uint8_t outbuf[20480];
	AACENC_ERROR err;
    off64_t rest;  
	off64_t count;
    size_t c, frame_size;  

	
	//input_size = channels*2*info.frameLength;
	//input_buf = (uint8_t*) malloc(input_size);
	DEBUG_ERR( "sndpcm->chunk_bytes=%u",sndpcm->chunk_bytes);  
	convert_buf = (int16_t*) malloc(sndpcm->chunk_bytes);
      
    if (WAV_WriteHeader(fd, wav) < 0) {  
        exit(-1);  
    }  
	DEBUG_ERR( "after WAV_WriteHeader");  
	out = fopen(outfile, "wb");
	if (!out) {
		perror(outfile);
		return 1;
	}
	DEBUG_ERR( "after fopen");  
  
    rest = wav->chunk.length; 
	//rest = len;
	info.frameLength;
	DEBUG_INFO("info.frameLength;=%u",info.frameLength);
	DEBUG_INFO("length=%lld",rest);
    while (rest > 0) {  
        c = (rest <= (off64_t)sndpcm->chunk_bytes) ? (size_t)rest : sndpcm->chunk_bytes;  
        frame_size = c * 8 / sndpcm->bits_per_frame;  
		
        if (SNDWAV_ReadPcm(sndpcm, frame_size) != frame_size)  
            break;  
      
        if (write(fd, sndpcm->data_buf, c) != c) {  
           DEBUG_ERR( "Error SNDWAV_Record[write]/n");  
            exit(-1);  
        }  
  		count += c;
        rest -= c;  
		// DEBUG_ERR( "after SNDWAV_ReadPcm");     
		read = c;
		for (i = 0; i < read/2; i++) {

			//DEBUG_ERR( "before sndpcm->data_buf");  
			const uint8_t* in = &sndpcm->data_buf[2*i];
			//DEBUG_ERR( "after sndpcm->data_buf");  
			convert_buf[i] = in[0] | (in[1] << 8);
		}
		//DEBUG_ERR( "before aacEncEncode");  
		if (read <= 0) {
			in_args.numInSamples = -1;
		} else {
			in_ptr = convert_buf;
			in_size = read;
			in_elem_size = 2;

			in_args.numInSamples = read/2;
			in_buf.numBufs = 1;
			in_buf.bufs = &in_ptr;
			in_buf.bufferIdentifiers = &in_identifier;
			in_buf.bufSizes = &in_size;
			in_buf.bufElSizes = &in_elem_size;
		}

		out_ptr = outbuf;
		out_size = sizeof(outbuf);
		out_elem_size = 1;
		out_buf.numBufs = 1;
		out_buf.bufs = &out_ptr;
		out_buf.bufferIdentifiers = &out_identifier;
		out_buf.bufSizes = &out_size;
		out_buf.bufElSizes = &out_elem_size;

		if ((err = aacEncEncode(handle, &in_buf, &out_buf, &in_args, &out_args)) != AACENC_OK) {
			if (err == AACENC_ENCODE_EOF)
				break;
			fprintf(stderr, "Encoding failed\n");
			return 1;
		}
		if (out_args.numOutBytes == 0)
			continue;
		//DEBUG_INFO("out_args.numOutBytes=%d",out_args.numOutBytes);
		//DEBUG_INFO("rest=%lld",rest);
		fwrite(outbuf, 1, out_args.numOutBytes, out);
    }  
	//end_wave(fd,count);
	free(input_buf);
	free(convert_buf);
	fclose(out);
	fclose(fd);
	aacEncClose(&handle);
	return 0;
}  





ssize_t SNDWAV_P_SaveRead(int fd, void *buf, size_t count)  
{  
    ssize_t result = 0, res;  
  
    while (count > 0) {  
        if ((res = read(fd, buf, count)) == 0)  
            break;  
        if (res < 0)  
            return result > 0 ? result : res;  
        count -= res;  
        result += res;  
        buf = (char *)buf + res;  
    }  
    return result;  
}  
  
void SNDWAV_Play(SNDPCMContainer_t *sndpcm, WAVContainer_t *wav, int fd)  
{  
    int load, ret;  
    off64_t written = 0;  
    off64_t c;  
    off64_t count = LE_INT(wav->chunk.length);  
  
    load = 0;  
    while (written < count) {  
        /* Must read [chunk_bytes] bytes data enough. */  
        do {  
            c = count - written;  
            if (c > sndpcm->chunk_bytes)  
                c = sndpcm->chunk_bytes;  
            c -= load;  
  
            if (c == 0)  
                break;  
            ret = SNDWAV_P_SaveRead(fd, sndpcm->data_buf + load, c);  
            if (ret < 0) {  
               DEBUG_ERR( "Error safe_read/n");  
                exit(-1);  
            }  
            if (ret == 0)  
                break;  
            load += ret;  
        } while ((size_t)load < sndpcm->chunk_bytes);  
  
        /* Transfer to size frame */  
        load = load * 8 / sndpcm->bits_per_frame;  
        ret = SNDWAV_WritePcm(sndpcm, load);  
        if (ret != load)  
            break;  
          
        ret = ret * sndpcm->bits_per_frame / 8;  
        written += ret;  
        load = 0;  
    }  
}   



int aplay_init(SNDPCMContainer_t *playback, SNDPCMParams_t *params)
{
    char *devicename = "default";    
  
    if (snd_output_stdio_attach(&playback->log, stderr, 0) < 0) {  
        DEBUG_ERR("Error snd_output_stdio_attach");  
        goto Err;  
    }  
  
    if (snd_pcm_open(&playback->handle, devicename, SND_PCM_STREAM_PLAYBACK, 0) < 0) {  
         DEBUG_ERR("Error snd_pcm_open [ %s]", devicename);  
        goto Err;  
    }  

	if (SND_SetParams(playback, params)) {  
       	DEBUG_ERR( "Error SND_SetParams");  
        goto Err;  
    } 
	DEBUG_INFO("After SND_SetParams");
	return 0;
Err:  
	if (playback->data_buf) free(playback->data_buf);  
	if (playback->log) snd_output_close(playback->log);  
	if (playback->handle) snd_pcm_close(playback->handle);  
	return 1;
  
}

int aplay_close(SNDPCMContainer_t *playback) 
{
	if (playback->handle) snd_pcm_drain(playback->handle);
	if (playback->data_buf) free(playback->data_buf);  
    if (playback->log) snd_output_close(playback->log);  
    if (playback->handle) snd_pcm_close(playback->handle); 
}




static int aplay(const char *filename)  
{  

    char *devicename = "default";  
    int fd;  
    WAVContainer_t wav;  
    SNDPCMContainer_t playback;  
      
    memset(&playback, 0x0, sizeof(playback));  
  
    fd = open(filename, O_RDONLY);  
    if (fd < 0) {  
       DEBUG_ERR( "Error open [%s]/n", filename);  
        return -1;  
    }  
      
    if (WAV_ReadHeader(fd, &wav) < 0) {  
       DEBUG_ERR( "Error WAV_Parse [%s]/n", filename);  
        goto Err;  
    }  
  	DEBUG_INFO("wav.format.format=%d",LE_SHORT(wav.format.format));
		DEBUG_INFO("wav.format.channels=%d",LE_SHORT(wav.format.channels));
		DEBUG_INFO("wav.format.sample_rate=%d",LE_INT(wav.format.sample_rate));

    if (snd_output_stdio_attach(&playback.log, stderr, 0) < 0) {  
       DEBUG_ERR( "Error snd_output_stdio_attach/n");  
        goto Err;  
    }  
  
    if (snd_pcm_open(&playback.handle, devicename, SND_PCM_STREAM_PLAYBACK, 0) < 0) {  
       DEBUG_ERR( "Error snd_pcm_open [ %s]/n", devicename);  
        goto Err;  
    }  
  
    if (SNDWAV_SetParams(&playback, &wav) < 0) {  
       DEBUG_ERR( "Error set_snd_pcm_params/n");  
        goto Err;  
    }  
    //snd_pcm_dump(playback.handle, playback.log);  
  
    SNDWAV_Play(&playback, &wav, fd);  
  
    snd_pcm_drain(playback.handle);  
  
    close(fd);  
    free(playback.data_buf);  
    snd_output_close(playback.log);  
    snd_pcm_close(playback.handle);  
    return 0;  
  
Err:  
    close(fd);  
    if (playback.data_buf) free(playback.data_buf);  
    if (playback.log) snd_output_close(playback.log);  
    if (playback.handle) snd_pcm_close(playback.handle);  
    return -1;  
}  


int record_init()
{
	int ret ;
    char *devicename = "default";  


	memset(&record, 0x0, sizeof(record));  
  
    remove(filename);  
    if ((fd = open(filename, O_WRONLY | O_CREAT, 0644)) == -1) {  
       DEBUG_ERR( "Error open: [%s]/n", filename);  
        return -1;  
    }  
  
    if (snd_output_stdio_attach(&record.log, stderr, 0) < 0) {  
       DEBUG_ERR( "Error snd_output_stdio_attach/n");  
        goto Err;  
    }  
  
    if (snd_pcm_open(&record.handle, devicename, SND_PCM_STREAM_CAPTURE, 0) < 0) {  
       DEBUG_ERR( "Error snd_pcm_open [ %s]/n", devicename);  
        goto Err;  
    }  
  
    if (SNDWAV_PrepareWAVParams(&wav) < 0) {  
       DEBUG_ERR( "Error SNDWAV_PrepareWAVParams/n");  
        goto Err;  
    }  
  
    if (SNDWAV_SetParams(&record, &wav) < 0) {  
       DEBUG_ERR( "Error set_snd_pcm_params/n");  
        goto Err;  
    }  
	DEBUG_ERR( "before adcenc_init");  
   	aacenc_init(&wav);
	DEBUG_ERR( "after adcenc_init"); 

	return ret;
	  
Err:  
	close(fd);	
	remove(filename);  
	if (record.data_buf) free(record.data_buf);  
	if (record.log) snd_output_close(record.log);  
	if (record.handle) snd_pcm_close(record.handle);  
	return -1;	

}


void  record_close()
{
	close(fd);  
	snd_pcm_drain(record.handle);  
	if (record.data_buf) free(record.data_buf);  
	if (record.log) snd_output_close(record.log);  
	if (record.handle) snd_pcm_close(record.handle);  

}

int arecord(const char *filename)  
{  
	int ret ;
    char *devicename = "default";  
  
    WAVContainer_t wav;  
    SNDPCMContainer_t record;  
	int fd; 
      
    memset(&record, 0x0, sizeof(record));  
  
    remove(filename);  
    if ((fd = open(filename, O_WRONLY | O_CREAT, 0644)) == -1) {  
       DEBUG_ERR( "Error open: [%s]/n", filename);  
        return -1;  
    }  
  
    if (snd_output_stdio_attach(&record.log, stderr, 0) < 0) {  
       DEBUG_ERR( "Error snd_output_stdio_attach/n");  
        goto Err;  
    }  
  
    if (snd_pcm_open(&record.handle, devicename, SND_PCM_STREAM_CAPTURE, 0) < 0) {  
       DEBUG_ERR( "Error snd_pcm_open [ %s]/n", devicename);  
        goto Err;  
    }  
  
    if (SNDWAV_PrepareWAVParams(&wav) < 0) {  
       DEBUG_ERR( "Error SNDWAV_PrepareWAVParams/n");  
        goto Err;  
    }  
  
    if (SNDWAV_SetParams(&record, &wav) < 0) {  
       DEBUG_ERR( "Error set_snd_pcm_params/n");  
        goto Err;  
    }  
	DEBUG_ERR( "before adcenc_init");  
   	aacenc_init(&wav);
	DEBUG_ERR( "after adcenc_init");  
	
   // snd_pcm_dump(record.handle, record.log);  
  	ret = SNDWAV_Record_Aac(&record, &wav, fd);
   DEBUG_ERR( "after SNDWAV_Record_Aac");  
   	//ret = SNDWAV_Record(&record, &wav, fd, result);  
  
    snd_pcm_drain(record.handle);  
  
    close(fd);  
    free(record.data_buf);  
    snd_output_close(record.log);  
    snd_pcm_close(record.handle);  
    return ret;  
  
Err:  
    close(fd);  
    remove(filename);  
    if (record.data_buf) free(record.data_buf);  
    if (record.log) snd_output_close(record.log);  
    if (record.handle) snd_pcm_close(record.handle);  
    return -1;  
}  


