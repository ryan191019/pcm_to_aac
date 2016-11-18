#include <stdio.h>
#include <stdlib.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>
#include <fdk-aac/aacenc_lib.h>
#include <alsa/asoundlib.h>
#include <semaphore.h> 
#include <pthread.h>
#include "fifobuffer.h"

#include "wav_parser.h"  
#include "debug.h"

static char *outfile="/tmp/222.aac";




pthread_t Encoder_Pthread = 0;

sem_t encoder_sem;

#define DECODE_FIFO_BUFFLTH (44100 * 2 * 5)
//#define PLAY_FIFO_BUFFLTH (44100 * 2 * 5)
#define ENCODER_FIFO_BUFFLTH (1024 * 1024 )

FT_FIFO * EncoderFifo = NULL;

extern HANDLE_AACENCODER handle;
extern AACENC_InfoStruct info;
extern int channel;

int g_DecodeFlag = 0;


void encoder_pthread_init()
{
	EncoderFifo = ft_fifo_alloc(ENCODER_FIFO_BUFFLTH);
	if (EncoderFifo == NULL) {
		DEBUG_ERR("ft_fifo_alloc failed");
	}
}
void *encoder_pcm_pthread(void)
{
	int iCount = 0;

	int input_size;
	uint8_t* input_buf;
	int16_t* convert_buf;
	FILE *out;
	int exit = 0;
	unsigned int iLength = 0;
	unsigned char byFlag = 0;
	DEBUG_INFO("encoder_pcm_pthread start....");
	

	input_size = channel*2*info.frameLength;
	input_buf = (uint8_t*) malloc(input_size);
	convert_buf = (int16_t*) malloc(input_size);

	out = fopen(outfile, "wb");
	if (!out) {
		perror(outfile);
		goto failed;
	}
	
	while (1)
	{

		AACENC_BufDesc in_buf = { 0 }, out_buf = { 0 };
		AACENC_InArgs in_args = { 0 };
		AACENC_OutArgs out_args = { 0 };
		int in_identifier = IN_AUDIO_DATA;
		int in_size, in_elem_size;
		int out_identifier = OUT_BITSTREAM_DATA;
		int out_size, out_elem_size;
		int read, i;
		void *in_ptr, *out_ptr;
		uint8_t outbuf[20480];
		AACENC_ERROR err;
		
		//if (0 == byFlag)
		//{
		//    if (0 == sem_wait(&encoder_sem))
		//    {
		//        byFlag = 1;
		//    }
		//}

		iLength = ft_fifo_getlenth(EncoderFifo);
		DEBUG_INFO("iLength:%d", read);
		if (iLength > 0)
		{
    		if (iLength >= input_size)
			{
			    read = input_size;
			}else {
				if(0 == g_DecodeFlag){
					continue;
				} else {
					read = iLength;
					exit = 1;
				}
			}

			

			ft_fifo_seek(EncoderFifo, input_buf, 0, read);

			ft_fifo_setoffset(EncoderFifo, read);

			for (i = 0; i < read/2; i++) {
					const uint8_t* in = &input_buf[2*i];
					convert_buf[i] = in[0] | (in[1] << 8);
			}
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
				if (err == AACENC_ENCODE_EOF){
					DEBUG_INFO("aacEncEncode  err:%d", AACENC_ENCODE_EOF);
					break;
				}
				fprintf(stderr, "Encoding failed\n");
				return 1;
			}
			if (out_args.numOutBytes == 0)
				continue;
			fwrite(outbuf, 1, out_args.numOutBytes, out);
			memset(input_buf, 0, input_size);
			memset(convert_buf, 0, input_size);
			

		}
		if(exit)
				break;

		/*if (1 == g_DecodeFlag)
		{
			    DEBUG_INFO("encoder_pthread over..");
				g_DecodeFlag = 0;
				
				free(input_buf);
				input_buf = NULL;
				free(convert_buf);	
				convert_buf = NULL;
				fclose(out);
				aacEncClose(&handle);
		    	byFlag = 0;
				break;

		}*/
		//DEBUG_PRINTF("iCount:%d\r\n", iCount);
	}
failed:
	DEBUG_INFO("end encoder_pthread");
	DEBUG_INFO("encoder_pthread over..");
	g_DecodeFlag = 0;				
	free(input_buf);
	input_buf = NULL;
	free(convert_buf);	
	convert_buf = NULL;
	fclose(out);
	aacEncClose(&handle);
	byFlag = 0;
}

void create_encoder_pthread(void)
{
	int iRet;

	iRet = pthread_create(&Encoder_Pthread, NULL, encoder_pcm_pthread, NULL);
	if(iRet != 0)
	{
		DEBUG_INFO("pthread_create error:%s\n", strerror(iRet));
		exit(-1);
	}
}


void encoder_pthread_exit()
{
	ft_fifo_clear(EncoderFifo);
}











