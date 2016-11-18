//File   : sndwav_common.c  
//Author : Loon <sepnic@gmail.com>  
  
#include <assert.h>  
#include <stdio.h>  
#include <stdlib.h>  
#include <unistd.h>  
#include <fcntl.h>  
#include <alsa/asoundlib.h>  
  
#include "sndwav_common.h"
#include "debug.h"
  
int SNDWAV_P_GetFormat(WAVContainer_t *wav, snd_pcm_format_t *snd_format)  
{     
    if (LE_SHORT(wav->format.format) != WAV_FMT_PCM)  
        return -1;  
      
    switch (LE_SHORT(wav->format.sample_length)) {  
    case 16:  
        *snd_format = SND_PCM_FORMAT_S16_LE;  
        break;  
    case 8:  
        *snd_format = SND_PCM_FORMAT_U8;  
        break;  
    default:  
        *snd_format = SND_PCM_FORMAT_UNKNOWN;  
        break;  
    }  
  
    return 0;  
}  
  
ssize_t SNDWAV_ReadPcm(SNDPCMContainer_t *sndpcm, size_t rcount)  
{  
    ssize_t r;  
    size_t result = 0;  
    size_t count = rcount;  
    uint8_t *data = sndpcm->data_buf;  
  
    if (count != sndpcm->chunk_size) {  
        count = sndpcm->chunk_size;  
    }  
  
    while (count > 0) {  
        r = snd_pcm_readi(sndpcm->handle, data, count);  
          
        if (r == -EAGAIN || (r >= 0 && (size_t)r < count)) {  
            snd_pcm_wait(sndpcm->handle, 1000);  
        } else if (r == -EPIPE) {  
            snd_pcm_prepare(sndpcm->handle);  
             DEBUG_ERR( " Buffer Underrun");  
        } else if (r == -ESTRPIPE) {  
             DEBUG_ERR( "Need suspend");  
        } else if (r < 0) {  
             DEBUG_ERR( "Error snd_pcm_writei: [%s]", snd_strerror(r));  
            exit(-1);  
        }  
          
        if (r > 0) {  
            result += r;  
            count -= r;  
            data += r * sndpcm->bits_per_frame / 8;  
        }  
    }  
    return rcount;  
}  
  
ssize_t SNDWAV_WritePcm(SNDPCMContainer_t *sndpcm, size_t wcount)  
{  
    ssize_t r;  
    ssize_t result = 0;  
    uint8_t *data = sndpcm->data_buf;  
  
    if (wcount < sndpcm->chunk_size) {  
        snd_pcm_format_set_silence(sndpcm->format,   
            data + wcount * sndpcm->bits_per_frame / 8,   
            (sndpcm->chunk_size - wcount) * sndpcm->channels);  
        wcount = sndpcm->chunk_size;  
    }  
    while (wcount > 0) {  
        r = snd_pcm_writei(sndpcm->handle, data, wcount);  
        if (r == -EAGAIN || (r >= 0 && (size_t)r < wcount)) {  
            snd_pcm_wait(sndpcm->handle, 1000);  
        } else if (r == -EPIPE) {  
            snd_pcm_prepare(sndpcm->handle);  
            DEBUG_ERR( " Buffer Underrun ");  
        } else if (r == -ESTRPIPE) {              
            DEBUG_ERR( " Need suspend");          
        } else if (r < 0) {  
            DEBUG_ERR( "Error snd_pcm_writei: [%s]", snd_strerror(r));  
            exit(-1);  
        }  
        if (r > 0) {  
            result += r;  
            wcount -= r;  
            data += r * sndpcm->bits_per_frame / 8;  
        }  
    }  
    return result;  
}  

ssize_t SNDWAV_WritePcm_Buf(SNDPCMContainer_t *sndpcm,char *buf, size_t wcount)  
{  
    ssize_t r;  
    ssize_t result = 0;  
    uint8_t *data = buf;  
  
    if (wcount < sndpcm->chunk_size) {  
        snd_pcm_format_set_silence(sndpcm->format,   
            data + wcount * sndpcm->bits_per_frame / 8,   
            (sndpcm->chunk_size - wcount) * sndpcm->channels);  
        wcount = sndpcm->chunk_size;  
    }  
    while (wcount > 0) {  
        r = snd_pcm_writei(sndpcm->handle, data, wcount);  
        if (r == -EAGAIN || (r >= 0 && (size_t)r < wcount)) {  
            snd_pcm_wait(sndpcm->handle, 1000);  
        } else if (r == -EPIPE) {  
            snd_pcm_prepare(sndpcm->handle);  
            DEBUG_ERR( " Buffer Underrun ");  
        } else if (r == -ESTRPIPE) {              
            DEBUG_ERR( " Need suspend");          
        } else if (r < 0) {  
            DEBUG_ERR( "Error snd_pcm_writei: [%s]", snd_strerror(r));  
            exit(-1);  
        }  
        if (r > 0) {  
            result += r;  
            wcount -= r;  
            data += r * sndpcm->bits_per_frame / 8;  
        }  
    }  
    return result;  
}  

int SND_SetParams(SNDPCMContainer_t *sndpcm, SNDPCMParams_t *param)   
{  
    snd_pcm_hw_params_t *hwparams;  
    snd_pcm_format_t format;  
    uint32_t exact_rate;  
    uint32_t buffer_time, period_time;  
  
    /* Allocate the snd_pcm_hw_params_t structure on the stack. */  
    snd_pcm_hw_params_alloca(&hwparams);  
      
    /* Init hwparams with full configuration space */  
    if (snd_pcm_hw_params_any(sndpcm->handle, hwparams) < 0) {  
       DEBUG_ERR( "Error snd_pcm_hw_params_any");  
        goto ERR_SET_PARAMS;  
    }  
  
    if (snd_pcm_hw_params_set_access(sndpcm->handle, hwparams, SND_PCM_ACCESS_RW_INTERLEAVED) < 0) {  
       DEBUG_ERR( "Error snd_pcm_hw_params_set_access");  
        goto ERR_SET_PARAMS;  
    }  
  	format = param->format;
    /* Set sample format */  
    //if (SNDWAV_P_GetFormat(wav, &format) < 0) {  
    //   DEBUG_ERR( "Error get_snd_pcm_format");  
    //    goto ERR_SET_PARAMS;  
    //}  
    
    if (snd_pcm_hw_params_set_format(sndpcm->handle, hwparams, format) < 0) {  
       DEBUG_ERR( "Error snd_pcm_hw_params_set_format");  
        goto ERR_SET_PARAMS;  
    }  
    sndpcm->format = format;  
  
    /* Set number of channels */  
    if (snd_pcm_hw_params_set_channels(sndpcm->handle, hwparams, param->channels ) < 0) {  
       DEBUG_ERR( "Error snd_pcm_hw_params_set_channels");  
        goto ERR_SET_PARAMS;  
    }  
    sndpcm->channels =  param->channels;  
  
    /* Set sample rate. If the exact rate is not supported */  
    /* by the hardware, use nearest possible rate.         */   
    exact_rate = param->sample_rate ;  
    if (snd_pcm_hw_params_set_rate_near(sndpcm->handle, hwparams, &exact_rate, 0) < 0) {  
       DEBUG_ERR( "Error snd_pcm_hw_params_set_rate_near");  
        goto ERR_SET_PARAMS;  
    }  
    if ( param->sample_rate != exact_rate) {  
       DEBUG_ERR( "The rate %d Hz is not supported by your hardware. ==> Using %d Hz instead.",   
            param->sample_rate ,exact_rate);  
    }  
  
    if (snd_pcm_hw_params_get_buffer_time_max(hwparams, &buffer_time, 0) < 0) {  
       DEBUG_ERR( "Error snd_pcm_hw_params_get_buffer_time_max");  
        goto ERR_SET_PARAMS;  
    }  
    if (buffer_time > 500000) buffer_time = 500000;  
	buffer_time = 500000/2;
    period_time = buffer_time / 4;  
  
    if (snd_pcm_hw_params_set_buffer_time_near(sndpcm->handle, hwparams, &buffer_time, 0) < 0) {  
       DEBUG_ERR( "Error snd_pcm_hw_params_set_buffer_time_near");  
        goto ERR_SET_PARAMS;  
    }  
  
    if (snd_pcm_hw_params_set_period_time_near(sndpcm->handle, hwparams, &period_time, 0) < 0) {  
       DEBUG_ERR( "Error snd_pcm_hw_params_set_period_time_near");  
        goto ERR_SET_PARAMS;  
    }  
  
    /* Set hw params */  
    if (snd_pcm_hw_params(sndpcm->handle, hwparams) < 0) {  
       DEBUG_ERR( "Error snd_pcm_hw_params(handle, params)");  
        goto ERR_SET_PARAMS;  
    }  
  
    snd_pcm_hw_params_get_period_size(hwparams, &sndpcm->chunk_size, 0);   
    snd_pcm_hw_params_get_buffer_size(hwparams, &sndpcm->buffer_size);  
    if (sndpcm->chunk_size == sndpcm->buffer_size) {        
       DEBUG_ERR( ("Can't use period equal to buffer size (%lu == %lu)"), sndpcm->chunk_size, sndpcm->buffer_size);         
        goto ERR_SET_PARAMS;  
    }  
  
    sndpcm->bits_per_sample = snd_pcm_format_physical_width(format);  
    sndpcm->bits_per_frame = sndpcm->bits_per_sample * param->channels;  
 	     
    sndpcm->chunk_bytes = sndpcm->chunk_size * sndpcm->bits_per_frame / 8;  
	DEBUG_ERR( "sndpcm->bits_per_sample=%u",sndpcm->bits_per_sample); 
	DEBUG_ERR( "sndpcm->bits_per_frame=%u",sndpcm->bits_per_frame); 
  	DEBUG_ERR( "sndpcm->chunk_bytes=%u",sndpcm->chunk_bytes);  
    /* Allocate audio data buffer */  
    sndpcm->data_buf = (uint8_t *)malloc(sndpcm->chunk_bytes);  
    if (!sndpcm->data_buf) {  
       DEBUG_ERR( "Error malloc: [data_buf]");  
        goto ERR_SET_PARAMS;  
    }  
  
    return 0;  
  
ERR_SET_PARAMS:  
    return -1;  
}  
 
int SNDWAV_SetParams(SNDPCMContainer_t *sndpcm, WAVContainer_t *wav)  
{  
    snd_pcm_hw_params_t *hwparams;  
    snd_pcm_format_t format;  
    uint32_t exact_rate;  
    uint32_t buffer_time, period_time;  
  
    /* Allocate the snd_pcm_hw_params_t structure on the stack. */  
    snd_pcm_hw_params_alloca(&hwparams);  
      
    /* Init hwparams with full configuration space */  
    if (snd_pcm_hw_params_any(sndpcm->handle, hwparams) < 0) {  
       DEBUG_ERR( "Error snd_pcm_hw_params_any");  
        goto ERR_SET_PARAMS;  
    }  
  
    if (snd_pcm_hw_params_set_access(sndpcm->handle, hwparams, SND_PCM_ACCESS_RW_INTERLEAVED) < 0) {  
       DEBUG_ERR( "Error snd_pcm_hw_params_set_access");  
        goto ERR_SET_PARAMS;  
    }  
  	
    /* Set sample format */  
    if (SNDWAV_P_GetFormat(wav, &format) < 0) {  
       DEBUG_ERR( "Error get_snd_pcm_format");  
        goto ERR_SET_PARAMS;  
    }  
    if (snd_pcm_hw_params_set_format(sndpcm->handle, hwparams, format) < 0) {  
       DEBUG_ERR( "Error snd_pcm_hw_params_set_format");  
        goto ERR_SET_PARAMS;  
    }  
    sndpcm->format = format;  
  
    /* Set number of channels */  
    if (snd_pcm_hw_params_set_channels(sndpcm->handle, hwparams, LE_SHORT(wav->format.channels)) < 0) {  
       DEBUG_ERR( "Error snd_pcm_hw_params_set_channels");  
        goto ERR_SET_PARAMS;  
    }  
    sndpcm->channels = LE_SHORT(wav->format.channels);  
  	
    /* Set sample rate. If the exact rate is not supported */  
    /* by the hardware, use nearest possible rate.         */   
    exact_rate = LE_INT(wav->format.sample_rate);  
    if (snd_pcm_hw_params_set_rate_near(sndpcm->handle, hwparams, &exact_rate, 0) < 0) {  
       DEBUG_ERR( "Error snd_pcm_hw_params_set_rate_near");  
        goto ERR_SET_PARAMS;  
    }  
    if (LE_INT(wav->format.sample_rate) != exact_rate) {  
       DEBUG_ERR( "The rate %d Hz is not supported by your hardware. ==> Using %d Hz instead.",   
            LE_INT(wav->format.sample_rate), exact_rate);  
    }  
  
    if (snd_pcm_hw_params_get_buffer_time_max(hwparams, &buffer_time, 0) < 0) {  
       DEBUG_ERR( "Error snd_pcm_hw_params_get_buffer_time_max");  
        goto ERR_SET_PARAMS;  
    }  
	DEBUG_ERR( "sndpcm->buffer_time=%u",buffer_time); 
	buffer_time = 500000;
    if (buffer_time > 500000) buffer_time = 500000;  
	buffer_time = 500000 /4 ;
	DEBUG_ERR( "sndpcm->buffer_time=%u",buffer_time); 
    period_time = buffer_time / 4;  
  
    if (snd_pcm_hw_params_set_buffer_time_near(sndpcm->handle, hwparams, &buffer_time, 0) < 0) {  
       DEBUG_ERR( "Error snd_pcm_hw_params_set_buffer_time_near");  
        goto ERR_SET_PARAMS;  
    }  
  
    if (snd_pcm_hw_params_set_period_time_near(sndpcm->handle, hwparams, &period_time, 0) < 0) {  
       DEBUG_ERR( "Error snd_pcm_hw_params_set_period_time_near");  
        goto ERR_SET_PARAMS;  
    }  
	
    /* Set hw params */  
    if (snd_pcm_hw_params(sndpcm->handle, hwparams) < 0) {  
       DEBUG_ERR( "Error snd_pcm_hw_params(handle, params)");  
        goto ERR_SET_PARAMS;  
    }  
  
    snd_pcm_hw_params_get_period_size(hwparams, &sndpcm->chunk_size, 0);   
    snd_pcm_hw_params_get_buffer_size(hwparams, &sndpcm->buffer_size);  
    if (sndpcm->chunk_size == sndpcm->buffer_size) {        
       DEBUG_ERR( ("Can't use period equal to buffer size (%lu == %lu)"), sndpcm->chunk_size, sndpcm->buffer_size);         
        goto ERR_SET_PARAMS;  
    }  
  
    sndpcm->bits_per_sample = snd_pcm_format_physical_width(format);  
    sndpcm->bits_per_frame = sndpcm->bits_per_sample * LE_SHORT(wav->format.channels);  
      
    sndpcm->chunk_bytes = sndpcm->chunk_size * sndpcm->bits_per_frame / 8;  
  	DEBUG_ERR( "sndpcm->bits_per_sample=%u",sndpcm->bits_per_sample); 
	DEBUG_ERR( "sndpcm->bits_per_frame=%u",sndpcm->bits_per_frame); 
  	DEBUG_ERR( "sndpcm->chunk_bytes=%u",sndpcm->chunk_bytes);  
    /* Allocate audio data buffer */  
    sndpcm->data_buf = (uint8_t *)malloc(sndpcm->chunk_bytes);  
    if (!sndpcm->data_buf) {  
       DEBUG_ERR( "Error malloc: [data_buf]");  
        goto ERR_SET_PARAMS;  
    }  
	
	DEBUG_INFO( "after SNDWAV_SetParams");  
  
    return 0;  
  
ERR_SET_PARAMS:  
    return -1;  
}  

void SNDWAV_Play_Buf(SNDPCMContainer_t *sndpcm, char *buf, unsigned int count)  
{  
	int ret;
    unsigned int  load;  
    unsigned int  written = 0;  
    unsigned int  c;  
  
    load = 0;

	//int iReadNumber = count / sndpcm->chunk_bytes;	
	//int iReadLastNumber = count % sndpcm->chunk_bytes;
	DEBUG_INFO("count=%u",count);
	DEBUG_INFO("sndpcm->chunk_bytes=%lu",sndpcm->chunk_bytes);
    while (written < count) {  
		
	  	c = count - written;	 
		if(c > sndpcm->chunk_bytes)
			load = sndpcm->chunk_bytes;
		else
			load = c; 
		DEBUG_INFO("load=%lu",load);
        /* Transfer to size frame */  
        load = load * 8 / sndpcm->bits_per_frame; 
		//SNDWAV_WritePcm_Buf()
        ret = SNDWAV_WritePcm(sndpcm, load);  
        if (ret != load)  
            break;  
        
        ret = ret * sndpcm->bits_per_frame / 8;  
        written += ret;  
		DEBUG_INFO("written=%lu",written);
        load = 0;  
    }  
}   

