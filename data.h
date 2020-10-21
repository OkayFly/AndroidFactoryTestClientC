#ifndef __ANDROID_DATA_H__
#define __ANDROID_DATA_H__

#include<stdbool.h>

#define TTYS3Port "/dev/ttyS3"
#define TTYS1Port "/dev/ttyS1"
#define USBPort "/dev/ttyUSB0"
#define CAN0Port "can0"
#define CAN1Port "can1"

extern char CPU_ID[64];
typedef enum DataProcessStatus
{
    DATA_PROCESS_SUCCESS,
    DATA_HEAD_ERROR,
    DATA_TAIL_ERROR,

}DPStatus;

typedef struct 
{
    unsigned char cpu_sn[20]; //cpu serial number
    bool TTYS1;
    bool TTYS3;
    bool CAN0;
    bool CAN1;
    bool SAMECPU;
}AndriodProduct;

typedef enum {
    CTRL_SEND_TTYS1_MAC = 0x80,
    CTRL_SEND_TTYS3_MAC,
    CTRL_SEND_CAN0_MAC,
    CTRL_SEND_CAN1_MAC,
    CTRL_END,
}ctrl_t;

DPStatus get_data( unsigned char* in,  int length,  unsigned char* out, int* out_length);
void process_data( unsigned char* data, int length);
void get_mac( unsigned char* data, int length);
void get_cpu_sn(char* cpu_sn_buffer);
void wrap_dat(unsigned char* data, int id);
void save_test_result(AndriodProduct* product);

#endif//__ANDROID_DATA_H__
