#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <math.h>
#include "fifobuffer.h"

#define min(x, y) ((x) < (y) ? (x) : (y))

FT_FIFO *ft_fifo_init(unsigned char *buffer, unsigned int size)
{
	FT_FIFO *fifo = NULL;

	fifo = malloc(sizeof(FT_FIFO));
	if (!fifo) {
		perror("Malloc failed!(ft_fifo)\n");
		return fifo;
	}

	fifo->buffer = buffer;
	fifo->size = size;
	fifo->in = fifo->out = 0;
	
	if(pthread_mutex_init(&fifo->lock, NULL) != 0) {
		perror("Mutex initialization failed!\n");
		free(fifo);
		return NULL;
	}

	return fifo;
}

FT_FIFO *ft_fifo_alloc(unsigned int size)
{
	unsigned char *buffer = NULL;
	FT_FIFO *ret = NULL;

	buffer = malloc(size);
	if (!buffer)
		return ret;

	ret = ft_fifo_init(buffer, size);

	if (!ret) {
		free(buffer);
	}

	return ret;
}

void ft_fifo_free(FT_FIFO *fifo)
{
	free(fifo->buffer);
	free(fifo);
}

void _ft_fifo_clear(FT_FIFO *fifo)
{
	fifo->in = fifo->out = 0;
}

unsigned int _ft_fifo_put(FT_FIFO *fifo,
			   unsigned char *buffer, unsigned int len)
{
	unsigned int l,temp;
	unsigned int freesize;

	if(fifo->out == ((fifo->in + 1) % fifo->size))
		return 0;
	
	if(fifo->in >= fifo->out)
		freesize = fifo->size - fifo->in + fifo->out - 1;
	else
		freesize = fifo->out - fifo->in - 1;
	
	len = min(len, freesize);

	l = min(len, fifo->size - fifo->in % fifo->size );

	memcpy(fifo->buffer + (fifo->in % fifo->size), buffer, l);
	memcpy(fifo->buffer, buffer + l, len - l);
	
	temp = fifo->in;
	temp = temp + len;
	temp = temp % fifo->size;
	fifo->in = temp;

	return len;
}

unsigned int _ft_fifo_get(FT_FIFO *fifo,
			   unsigned char *buffer, unsigned int offset, unsigned int len)
{
	unsigned int l,temp;
	unsigned int datasize;

	if( fifo->out  == fifo->in )
		return 0;

	if(fifo->out >= fifo->in)
		datasize = fifo->size - fifo->out + fifo->in;
	else
		datasize = fifo->in - fifo->out;

	if(offset > datasize)
		return -1;
	else
	{
		datasize = datasize - offset;
		temp = fifo->out;
		temp = temp + offset;
		temp = temp % fifo->size;
		fifo->out = temp;
		}

	len = min(len, datasize);

	l = min(len, fifo->size - (fifo->out % fifo->size));
	memcpy(buffer, fifo->buffer + (fifo->out % fifo->size), l);
	memcpy(buffer + l, fifo->buffer, len - l);

	temp = fifo->out;
	temp = temp + len;
	temp = temp % fifo->size;
	fifo->out = temp;

	return len;
}

unsigned int _ft_fifo_seek(FT_FIFO *fifo,
			   unsigned char *buffer, unsigned int offset, unsigned int len)
{
	unsigned int l,temp;
	unsigned int datasize;

	if( fifo->out  == fifo->in )
		return 0;

	if(fifo->out > fifo->in)
		datasize = fifo->size - fifo->out + fifo->in;
	else
		datasize = fifo->in - fifo->out;

	if(offset > datasize)
		return -1;
	else
	{
		datasize = datasize - offset;
		temp = fifo->out;
		temp = temp + offset;
		temp = temp % fifo->size;
		}
	
	len = min(len, datasize);

	l = min(len, fifo->size - (temp % fifo->size));
	memcpy(buffer, fifo->buffer + (temp % fifo->size), l);
	memcpy(buffer + l, fifo->buffer, len - l);

	return len;
}

unsigned int _ft_fifo_setoffset(FT_FIFO *fifo, unsigned int offset)
{
	unsigned int datasize;
	unsigned int l,temp;
	if( fifo->out  == fifo->in )
		return 0;

	if(fifo->out > fifo->in)
		datasize = fifo->size - fifo->out + fifo->in;
	else
		datasize = fifo->in - fifo->out;

	l = min(offset, datasize);
	
	temp = fifo->out;
	temp = temp + l;
	temp = temp % fifo->size;
	fifo->out = temp;

	return l;
	}

unsigned int _ft_fifo_getlenth(FT_FIFO *fifo)
{
	unsigned int datasize;
	
	if( fifo->out  == fifo->in )
		return 0;
	
	if(fifo->out >= fifo->in)
		datasize = fifo->size - fifo->out + fifo->in;
	else
		datasize = fifo->in - fifo->out;

	return datasize;
}


