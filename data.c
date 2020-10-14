#include "data.h"
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>

#define LENGTHBUF 1024

#define DATA_HEAD (0xAA)
#define DATA_TAIL (0x55)


char CPU_ID[64];
//#define DATA_TRANSLATE (0xCC)


DPStatus get_data( unsigned char* in,  int length, unsigned char* out, int* out_length)
{
    bool has_head = false;
    int  head_index = 0;

    for(int i=0; i<length; i++)
    {
        if(in[i] == DATA_HEAD)
        {
            has_head = true;
            head_index = i;
            break;
        }
    }

    if(!has_head)
        return DATA_HEAD_ERROR;
    
    int  tail_index =0;

    for(int i=head_index+1; i<length; i++)
    {
        if(in[i] == DATA_TAIL)
        {
            tail_index = i;
            break;
        }
    }

    if(tail_index <1)
        return DATA_TAIL_ERROR;

    *out_length = tail_index - head_index-1;
    printf("tail_index:%d, *outlength:%d\n", tail_index, *out_length);

    memcpy(out, in+head_index+1, *out_length);
    
    return DATA_PROCESS_SUCCESS;
}

void wrap_data(unsigned char* data, int id)
{
	int len = strlen(data);
	

}

void process_data(unsigned char* data, int length)
{
    switch (data[0])
    {
    case CTRL_SEND_MAC:
        get_mac(data+1, length-1);
        break;
    case CTRL_SEND_DATA:
        get_mac(data+1, length-1);
        break;
    case CTRL_DATA_END:
        get_mac(data+1, length-1);
        break;
    default:
        break;
    }

}


void get_mac(unsigned char* data, int length)
{
    printf("** get server data mac:\n");

    for(int i=0; i<length; i++)
    {
        printf("\t\t %02x \t\t", data[i]);
    }
    printf("CPU_ID:%s\n",CPU_ID);
    if(strcmp(data, CPU_ID) == 0)
    {
	printf("okkkkkkkkkkkkkkkkkkkkkkkkk\n");
    }
}

void get_cpu_sn(char* cpu_sn_buffer)
{
	    FILE *fp;
    int nread =0;
    char *file = "/proc/cpuinfo";
    char *buffer = NULL;
    char content[40] = "";
    size_t len =0; 

    fp = fopen(file, "rb");
    if(fp == NULL)
    {
        printf("error happen to open:%s\n",file);
        return;
    }

    while(nread = getline(&buffer, &len, fp) != -1)
    {
        if(strstr(buffer,"Serial") != NULL)
        {
            buffer[strlen(buffer)-1] = 0;
            char* ptr =strchr(buffer,':');
            strcpy(content, buffer+(ptr-buffer)+2);
            strcpy(cpu_sn_buffer,content);
            break;
        }
    }
    memcpy(CPU_ID, cpu_sn_buffer,strlen(cpu_sn_buffer));
}
