#include "data.h"
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>

#define LENGTHBUF 1024

#define DATA_HEAD (0xAA)
#define DATA_TAIL (0x55)

bool STOPTEST = false;
extern char CPU_ID[64];
//#define DATA_TRANSLATE (0xCC)


DPStatus get_data( unsigned char* in,  int length, unsigned char* out, int* out_length)
{
    printf("get_data\n");
    for(int i=0; i<length; i++)
    {
        printf("%0x2x ", in[i]);
    }
    printf("\n");
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

void process_data(unsigned char* data, int length, AndriodProduct* product, fsm_state_t* fsm)
{
    switch (data[0])
    {
    case CTRL_SEND_MAC:
        get_mac(data+1, length-1, product);
        *fsm= FSM_GET_MAC;
        break;
    case CTRL_SEND_END:
        get_end(data+1, length-1, product);
        *fsm= FSM_GET_END;
        break;
    default:
        break;
    }

}


void get_mac(unsigned char* data, int length, AndriodProduct* product)
{
    printf("** get server data mac:\n");

    for(int i=0; i<length; i++)
    {
        printf("%02x", data[i]);
    }
    printf("\nCPU_ID:%s\n",CPU_ID);
    if(strncmp(data, CPU_ID, strlen(CPU_ID)) == 0)
    {
	printf("okkkkkkkkkkkkkkkkkkkkkkkkk\n");
    fflush(stdout);
    }
    else
    {
    printf("wfk\n");
	for(int i=0; i<strlen(CPU_ID); i++ )
	{
		printf("%02x ", CPU_ID[i]);
	}
	printf("\n");
   
    }
    
}

//can not receive the cmd
void get_end(unsigned char* data, int length, AndriodProduct* product)
{
    printf("** get_endget_endget_endget_endget_endget_endget_endget_endget_endget_endget_endget_endget_endget_endget_endget_endget_endget_endget_endget_endget_endget_endget_endget_endget_endget_end:\n");

    for(int i=0; i<length; i++)
    {
        printf("%02x", data[i]);
    }
    printf("\nCPU_ID:%s\n",CPU_ID);
    if(strncmp(data, CPU_ID, strlen(CPU_ID)) == 0)
    {
        printf("mmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm\n");
        fflush(stdout);
    }
    else
    {
        printf("wfk\n");
        for(int i=0; i<strlen(CPU_ID); i++ )
        {
            printf("%02x ", CPU_ID[i]);
        }
        printf("\n");
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

void save_test_result(AndriodProduct* product)
{
    printf("\n** \t\t %s\n",__func__);
    FILE *fp;
    unsigned  char* filename = (unsigned char*)malloc(strlen(product->cpu_sn)+5);
    if(filename == NULL)
        printf("error maclloc\n");
    //fprintf(filename, "%s.txt",name);
    memcpy(filename, product->cpu_sn, strlen(product->cpu_sn));
    memcpy(filename+ strlen(product->cpu_sn), ".txt", sizeof(".txt"));
    printf("filename:%s\n",filename);

    if((fp = fopen(filename, "wb")) == NULL)
    {
        printf("\t\tXXX error cant open %s\n",filename);
        return;
    }

    fprintf(fp, "%s:\n",product->cpu_sn);
    fprintf(fp, "%s:[%s]\n",TTYS1Port ,product->TTYS1 ? "OK":"Fail");
    fprintf(fp, "%s:[%s]\n",TTYS3Port ,product->TTYS3 ? "OK":"Fail");
    fprintf(fp, "%s:[%s]\n",CAN0Port ,product->CAN0 ? "OK":"Fail");
    fprintf(fp, "%s:[%s]\n",CAN1Port ,product->CAN1 ? "OK":"Fail");

    fclose(fp);
    for(int i=0; i<30;i++)
    {
        printf(">");
    }
    printf("%s\n",filename);
    printf("save %s ok\n",filename);
    free(filename);
}