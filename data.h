#ifndef __ANDROID_DATA_H__
#define __ANDROID_DATA_H__

#include<stdbool.h>


typedef enum DataProcessStatus
{
    DATA_PROCESS_SUCCESS,
    DATA_HEAD_ERROR,
    DATA_TAIL_ERROR,

}DPStatus;

typedef enum {
    CTRL_SEND_MAC = 0x80,
    CTRL_SEND_DATA = 0x81,
    CTRL_DATA_END,
}ctrl_t;

DPStatus get_data( unsigned char* in,  int length,  unsigned char* out, int* out_length);
void process_data( unsigned char* data, int length);
void get_mac( unsigned char* data, int length);
void get_cpu_sn(char* cpu_sn_buffer);
void wrap_dat(unsigned char* data, int id);
#endif//__ANDROID_DATA_H__
