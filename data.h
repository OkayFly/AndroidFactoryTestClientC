#ifndef __ANDROID_DATA_H__
#define __ANDROID_DATA_H__

#include<stdbool.h>
#include <pthread.h>
#include <string.h>
#include <sys/types.h>

#define TTYS3Port "/dev/ttyS3"
#define TTYS1Port "/dev/ttyS1"
#define USBPort "/dev/ttyUSB0"
#define CAN0Port "can0"
#define CAN1Port "can1"

typedef enum DataProcessStatus
{
    DATA_PROCESS_SUCCESS,
    DATA_HEAD_ERROR,
    DATA_TAIL_ERROR,

}DPStatus;

typedef enum 
{
    FSM_IDLE,
    FSM_GET_MAC,
    FSM_GET_END,
    FSM_TEST_OK,
    FSM_TEST_FAIL,
} fsm_state_t;

typedef struct 
{
    unsigned char cpu_sn[20]; //cpu serial number
    fsm_state_t TTYS1;
    fsm_state_t TTYS3;
    fsm_state_t CAN0;
    fsm_state_t CAN1;
    fsm_state_t SAMESN;
}AndriodProduct;

typedef struct 
{
    char port[14];//open which port
    AndriodProduct* product; //The point to the product  

}parameters;


typedef enum {
    CTRL_SEND_MAC = 0x80,
    CTRL_SEND_END,
    CTRL_SEND_IDLE,
}ctrl_t;

DPStatus parse_data( unsigned char* in,  int length,  unsigned char* out, int* out_length);
void process_data( unsigned char* data, int length, AndriodProduct* product, fsm_state_t* fsm);
void get_sn( unsigned char* data, int length, AndriodProduct* AndriodProduct, fsm_state_t* serial_fsm);
void get_end( unsigned char* data, int length, AndriodProduct* AndriodProduct, fsm_state_t* serial_fsm);
void get_cpu_sn(char* cpu_sn_buffer);
void wrap_dat(unsigned char* data, int id);

void save_test_result(AndriodProduct* product);

void save_process_t(void* params);

#endif//__ANDROID_DATA_H__
