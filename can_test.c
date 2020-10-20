#include "can_test.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <linux/can.h>
#include <linux/can/raw.h>

#include "data.h"
#include <time.h>

extern bool STOPTEST;

static void can_process_read_data(int s)
{
    	unsigned char data[95] = {};
	int data_length;
    /* set filter for only receiving packet with can id 0x88 */
        struct can_frame frame;
    struct can_filter rfilter[1];
    rfilter[0].can_id = 0x88;
    rfilter[0].can_mask = CAN_SFF_MASK;
    if(setsockopt(s, SOL_CAN_RAW, CAN_RAW_FILTER, &rfilter, sizeof(rfilter)) < 0)
    {
        perror("set receiving filter error\n");
        close(s);
        //exit(-3);
        return;
    }
    /* keep reading */
   // while(1){

      int   nbytes = read(s, &frame, sizeof(frame));
        if(nbytes > 0)
        {
       // printf("read datas:%s ID=%#x data length=%d\n", ifr.ifr_name, frame.can_id, frame.can_dlc);
        printf("read datas:xx ID=%#x data length=%d\n", frame.can_id, frame.can_dlc);
        for (int i=0; i < frame.can_dlc; i++)
            printf("%#x ", frame.data[i]);
        printf("\n");
        }
        printf("read can data over\n");



    if(get_data(frame.data, frame.can_dlc, data, &data_length) == DATA_PROCESS_SUCCESS)
    {
        
        printf("get data:%s, length[%d]\n",data, data_length);

        // for(int i=0; i<data_length; i++)
        // {
        // 	printf("\t\t %02x\t", data[i]);
        // }
        process_data(data, data_length);

    }
}


static void can_process_write_data(int s)
{
    char write_data[120];
	//wrap_data(CPU_ID, CTRL_SEND_MAC);
	write_data[0] = 0xAA;
	write_data[1] = CTRL_SEND_MAC;
	memcpy(write_data+2, CPU_ID, strlen(CPU_ID));
	write_data[2+strlen(CPU_ID)] = 0x55;
	write_data[3+strlen(CPU_ID)] = '\0';
	printf("\n\t write_data:%s\n",write_data);
	for(int i=0; i<strlen(write_data);i++)
	{	
		printf("%02x ", write_data[i]);
	}

    /* configure can_id and can data length */
    struct can_frame frame_send;
    frame_send.can_id = 0x88;
    frame_send.can_dlc = strlen(write_data);
   // printf("%s ID=%#x data length=%d\n", ifr.ifr_name, frame_send.can_id, frame_send.can_dlc);
    printf("xx ID=%#x data length=%d\n", frame_send.can_id, frame_send.can_dlc);

    /* Sending data */
    if(write(s, &frame_send, sizeof(frame_send)) < 0)
    {
    perror("Send failed");
    close(s);
    return;
    //exit(-4);
    }
    sleep(1);

	//usleep(1000000);//1s
	//while(1)
	{
		can_process_read_data(s);
	}
	
}



void can_process(char* canport)
{
    int s;
    int i;
    unsigned char number = 0;
    int nbytes;
    
    pid_t pid = -1;

    struct sockaddr_can addr;
    struct ifreq ifr;   
    struct can_filter rfilter[1];
    struct can_frame frame;
    struct can_frame frame_send;
    printf("**\t ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~serail_process:%s\n", canport);

    printf("**\t serail_process:%s\n", canport);

    //create socket
    if ((s = socket(PF_CAN, SOCK_RAW, CAN_RAW)) < 0)
	{
	   perror("Create socket failed");
	   return;
	}

    /* set up can interface */
	strcpy(ifr.ifr_name, canport);
	printf("can port is %s\n",ifr.ifr_name);

    /* assign can device */
	ioctl(s, SIOCGIFINDEX, &ifr);
	addr.can_family = AF_CAN;
	addr.can_ifindex = ifr.ifr_ifindex;
	/* bind can device */
	if(bind(s, (struct sockaddr *)&addr, sizeof(addr)) < 0)
	{
	     perror("Bind can device failed\n");
	     close(s);
	     return;
	}

    printf("** \t wait write cpu ID\n");
	fflush(stdout);
	struct timespec start_time, last_time, last_read, last_write;
	clock_gettime(CLOCK_MONOTONIC, &start_time);
	last_time = start_time;
	last_read = start_time;
	last_write = start_time;
    while(!STOPTEST)
    {
        struct timespec current;
        clock_gettime(CLOCK_MONOTONIC, &current);
        can_process_write_data(s);
        if(diff_ms(&current,&start_time) >= 10000 )//10s
		{
			printf("\t\t Error %s time consuming >5s but can't receive corrent data\n", canport);
			STOPTEST = true;
		}

    }


    // pid = fork();
    // if(pid == -1) 
    // { 
    //     perror("\n\tfailed to fork!!\n\n");
    //     return ;
    // }
    // else if(pid==0)/* configure receiving can data*/ //在子进程中，fork返回0；
	// {
	//    /* set filter for only receiving packet with can id 0x88 */
	//    rfilter[0].can_id = 0x88;
	//    rfilter[0].can_mask = CAN_SFF_MASK;
	//    if(setsockopt(s, SOL_CAN_RAW, CAN_RAW_FILTER, &rfilter, sizeof(rfilter)) < 0)
	//    {
	//         perror("set receiving filter error\n");
	//         close(s);
	//         //exit(-3);
    //         return;
	//    }
	//    /* keep reading */
	//    while(1){
 
	//        nbytes = read(s, &frame, sizeof(frame));
	//        if(nbytes > 0)
	//        {
	// 	   printf("read datas:%s ID=%#x data length=%d\n", ifr.ifr_name, frame.can_id, frame.can_dlc);
	// 	   for ( i=0; i < frame.can_dlc; i++)
	// 	        printf("%#x ", frame.data[i]);
	// 	   printf("\n");
	//        }
	//        printf("read can data over\n");
 
	//    }
	// }/* configure sending can data*/ //在父进程中，fork返回新创建子进程的进程ID；
    // else
	// {   
    //     while(1)
    //     {
	// 	/* configure can_id and can data length */
	//     frame_send.can_id = 0x88;
	// 	frame_send.can_dlc = 8;
	// 	printf("%s ID=%#x data length=%d\n", ifr.ifr_name, frame_send.can_id, frame_send.can_dlc);
	// 	/* prepare data for sending: 0x11,0x22...0x88 */
	// 	for (i=0; i<8; i++)
	// 	{
	// 	    frame_send.data[i] = ((i+1)<<4) | (i+1);
    //                 frame_send.data[7] =number;
	// 	    printf("%#x ", frame_send.data[i]);
	// 	}
	// 	printf("success to Sent out\n");
	// 	/* Sending data */
	// 	if(write(s, &frame_send, sizeof(frame_send)) < 0)
	//         {
	// 	    perror("Send failed");
	// 	    close(s);
    //         return;
	// 	    //exit(-4);
	//         }
    //             sleep(1);
    //             number++;
    //          }
            
	//  }
	close(s);
	return ;
}

void can_test()
{
    can_process(CAN0Port);
    can_process(CAN1Port);
}