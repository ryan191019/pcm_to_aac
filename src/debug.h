#ifndef __DEBUG_H__
#define __DEBUG_H__

//#define TESTMODE_DEBUG
#define UARTD_DEBUG  
#define DISNET_TEST 

#ifdef UARTD_DEBUG  
#define DEBUG_ERR(fmt, args...)    do { fprintf(stderr,"[%s:%d]"#fmt"\r\n", __func__, __LINE__, ##args); } while(0) 
#define DEBUG_INFO(fmt, args...)   do { printf("[%s:%d]"#fmt"\r\n", __func__, __LINE__, ##args); } while(0) 
#else
#define DEBUG_ERR(fmt, args...)   do {} while(0)  
#define DEBUG_INFO(fmt, args...)  do {} while(0)  
#endif

			

#endif