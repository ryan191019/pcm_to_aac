
#include <stdlib.h>
#include <stdio.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>


#include "debug.h"
#include "sndwav_common.h"


extern pthread_t Encoder_Pthread;
extern pthread_t Record_Pthread;

extern sem_t encoder_sem;
extern off64_t g_len;

static void Semaphore_Init(void)
{
	int iRet = -1;

	iRet = sem_init(&encoder_sem, 0, 0);
	if(iRet < 0)
	{
		DEBUG_ERR("Semaphore initialization failed..");
		exit(-1);  
	}
	
}

static void Semaphore_Destroy(void)
{
	sem_destroy(&encoder_sem);
}

static void thread_wait(void)
{

	if (Encoder_Pthread != 0)
	{
		pthread_join(Encoder_Pthread, NULL);
		DEBUG_INFO("Request_Pthread end...\n");
	}
	
	if (Record_Pthread != 0)
	{
			pthread_join(Record_Pthread, NULL);
			DEBUG_INFO("Request_Pthread end...\n");
	}

}
#if 1
int main()
{	
	Semaphore_Init();
	encoder_pthread_init();
	record_init();
	create_record_pthread();
	create_encoder_pthread();
	DEBUG_INFO("g_len=%lld",g_len);
	thread_wait();
	record_close();
	encoder_pthread_exit();
	Semaphore_Destroy();
	DEBUG_INFO("main end...\n");
	//aac_encoder("/tmp/snd.wav");
	//aac_encoder1("/tmp/snd.wav");
}
#else
int main()
{
	int ret;
	ret=arecord("/tmp/snd.wav");
	return ret;
}
#endif

