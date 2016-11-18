#include <stdio.h>
#include <stdlib.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>
#include <fdk-aac/aacenc_lib.h>
#include <alsa/asoundlib.h>
#include <pthread.h>

#include <semaphore.h>  
#include "wav_parser.h"  
#include "debug.h"
#include "fifobuffer.h"
#include "sndwav_common.h"


extern WAVContainer_t wav;  
extern SNDPCMContainer_t record;  
extern int fd; 
extern off64_t g_len;
extern sem_t encoder_sem;

extern  FT_FIFO * EncoderFifo;
extern int g_DecodeFlag;
pthread_t Record_Pthread = 0;




void *record_pthread(void)
{
	off64_t rest;  
    size_t c, frame_size;  
      
    if (WAV_WriteHeader(fd, &wav) < 0) {  
        exit(-1);  
    }  
  
   // rest = wav->chunk.length;  
	rest = g_len;
   	DEBUG_INFO("**********len:%lld", g_len);
    while (rest > 0) {  
        c = (rest <= (off64_t)record.chunk_bytes) ? (size_t)rest : record.chunk_bytes;  
        frame_size = c * 8 / record.bits_per_frame;  
        if (SNDWAV_ReadPcm(&record, frame_size) != frame_size)  
            break;  
          
        if (write(fd, record.data_buf, c) != c) {  
          	 DEBUG_ERR( "Error SNDWAV_Record[write]/n");  
             exit(-1);  
        }  
  		
		ft_fifo_put(EncoderFifo, record.data_buf, c);

		//sem_post(&encoder_sem);
		
        rest -= c;  
		//DEBUG_INFO("rest:%lld", rest);
    }  
	DEBUG_INFO("end record_pthread");

	g_DecodeFlag = 1;
}

void create_record_pthread(void)
{
	int iRet;

	iRet = pthread_create(&Record_Pthread, NULL, record_pthread, NULL);
	if(iRet != 0)
	{
		DEBUG_ERR("pthread_create error:%s\n", strerror(iRet));
		exit(-1);
	}
}


