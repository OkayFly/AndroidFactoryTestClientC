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
const int canfd_on = 1;

static void can_process_read_data(int s, char* canport)
{
    struct timeval timeout, timeout_config = { 0, 0 }, *timeout_current = NULL;
    timeout_config.tv_usec = 1000;//msecs// -T <msecs>  (terminate after <msecs> without any reception)\n"); //1s
    
    //???????????
    timeout_config.tv_sec = timeout_config.tv_usec / 1000;
    timeout_config.tv_usec = (timeout_config.tv_usec % 1000) * 1000;
    timeout_current = &timeout;

    fd_set rdfs;

    

    unsigned char receive_data[1024];
    int receive_len = 0;

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
    bool keepRead = true;
    struct  timespec start_time;
	clock_gettime(CLOCK_MONOTONIC, &start_time);

    while(keepRead){

        FD_ZERO(&rdfs);
        FD_SET(s,&rdfs);

        
		if (timeout_current)
			*timeout_current = timeout_config;
        printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~select~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");

        //???????????s+1
		if ((select(s+1, &rdfs, NULL, NULL, timeout_current)) <= 0) {
			perror("select");
            printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~Iiiiii~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
			keepRead = false;
            continue;
		}

        printf("\n\t~~~~~~~~~~~~~~~read\n\t");
		struct timespec current;
		clock_gettime(CLOCK_MONOTONIC, &current);

        unsigned char data[95] = {0};
	    int data_length;
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
        if(frame.can_dlc <=0 )
            continue;
        if(receive_len + frame.can_dlc > 1000)
            return;
        memcpy(&receive_data[receive_len], &frame.data[0], frame.can_dlc);
        receive_len += frame.can_dlc;





    if(get_data(receive_data, receive_len, data, &data_length) == DATA_PROCESS_SUCCESS)
    {
        memset(receive_data, 0, sizeof(receive_data));
        receive_len = 0;
        printf("=====================================>get data:%s, length[%d]\n",data, data_length);
        for(int i=0; i< data_length; i++)
        {
            printf("-------------------get data:%02x\n", data[i]);
        }

        // for(int i=0; i<data_length; i++)
        // {
        // 	printf("\t\t %02x\t", data[i]);
        // }
        process_data(data, data_length);
        keepRead = false;

    }


    if(diff_ms(&current,&start_time) >= 1000 )//1s
    {
        printf("\t\t Error %s time consuming >1s but can't read data\n", canport);
        keepRead = false;
    }



    }


}


static int can_frame_process_write_data(int s, char* canport) // just use 8 bytes for write
{
    printf("00000000000000000000000000000000000000000000000000000000000:%s\n", canport);

        char cmd;
        if( !strncmp(canport, CAN0Port, strlen(CAN0Port)))
        {
            cmd = CTRL_SEND_CAN0_MAC;
        }
        else if(!strncmp(canport, CAN1Port, strlen(CAN1Port)))
        {
            cmd = CTRL_SEND_CAN1_MAC;
        }
        else
        {
            printf("EEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEE:%s\n", canport);
            return 1;
        }
        
        struct can_frame frame_send;

        int cursor = 0;
        /* configure can_id and can data length */
	    frame_send.can_id = 0x88;
		frame_send.can_dlc = 8;
		printf("%s ID=%#x data length=%d\n", "ifr.ifr_name", frame_send.can_id, frame_send.can_dlc);
		/* prepare data for sending: 0xAA,0x81,0x11,0x11,0x11,0x11,0x11,0x55 */
        printf("sizeof can_frame.data:%d\n",sizeof(frame_send.data));

        //第一帧
        frame_send.data[0] = 0xAA;
        frame_send.data[1] = cmd;
        for(int i=2;i<8;i++)//6
        {
            frame_send.data[i] = CPU_ID[cursor++];
        }
		for (int i=0; i<8; i++) 
		{
            printf("%02x ", frame_send.data[i]);
		}
		printf("success to Sent out\n");
		/* Sending data */
		if(write(s, &frame_send, sizeof(frame_send)) < 0)
        {
        perror("Send failed");
        sleep(1);
        //close(s);
        //exit(-4);
        return 2;
        }
        usleep(100000);
        {
           // can_process_read_data(s, canport);
        }
        //第2帧
        for(int i=0;i<8;i++) //8
        {
            frame_send.data[i] = CPU_ID[cursor++];
        }
		for (int i=0; i<8; i++)
		{
		    // frame_send.data[i] = ((i+1)<<4) | (i+1);
            //         frame_send.data[7] =number;
		    // printf("%#x ", frame_send.data[i]);
            printf("%02x ", frame_send.data[i]);
		}
		printf("success to Sent out\n");
		/* Sending data */
		if(write(s, &frame_send, sizeof(frame_send)) < 0)
        {
        perror("Send failed");
        sleep(1);
        //close(s);
        //exit(-4);
        return 2;
        }

         usleep(100000);
        {
           // can_process_read_data(s, canport);
        }

        //第3帧
        frame_send.can_dlc = 3;  //2
        frame_send.data[0] = CPU_ID[cursor++];
        frame_send.data[1] = CPU_ID[cursor++];
        frame_send.data[2] = 0x55;

     
		for (int i=0; i<2; i++)
		{
		    // frame_send.data[i] = ((i+1)<<4) | (i+1);
            //         frame_send.data[7] =number;
		    // printf("%#x ", frame_send.data[i]);
            printf("%02x ", frame_send.data[i]);
		}
		printf("success to Sent out\n");
		/* Sending data */
		if(write(s, &frame_send, sizeof(frame_send)) < 0)
        {
        perror("Send failed");
        sleep(1);
        //close(s);
        //exit(-4);
        return 2;
        }

        usleep(100000);
        {
            can_process_read_data(s, canport);
        }

        return 0;



}

static void canfd_frame_process_write_data(int s, char* canport)
{
    // int required_mtu;//传输内容的最大传输内容
    // int mtu; // 发送数据长度
    // int enable_canfd = 1; // 开启关闭flag位


    char write_data[120];
	//wrap_data(CPU_ID, CTRL_SEND_TTYS1_MAC);
	write_data[0] = 0xAA;
	write_data[1] = CTRL_SEND_TTYS1_MAC;
	memcpy(write_data+2, CPU_ID, strlen(CPU_ID));
	write_data[2+strlen(CPU_ID)] = 0x55;
	write_data[3+strlen(CPU_ID)] = '\0';
	printf("\n\t write_data:%s\n",write_data);
	for(int i=0; i<strlen(write_data);i++)
	{	
		printf("%02x ", write_data[i]);
	}

    /* configure can_id and can data length */
    struct canfd_frame frame_send;
    frame_send.can_id = 0x88;
    frame_send.len = strlen(write_data);
   // printf("%s ID=%#x data length=%d\n", ifr.ifr_name, frame_send.can_id, frame_send.can_dlc);
    printf("xx ID=%#x data length=%d\n", frame_send.can_id, frame_send.len);

    /* Sending data */
    if(write(s, &frame_send, sizeof(frame_send)) != sizeof(frame_send))
    {
    perror("Send failed");
     sleep(1);
   // close(s);
    return;
    //exit(-4);
    }
    sleep(1);

	//usleep(1000000);//1s
	//while(1)
	{
	//	can_process_read_data(s);
	}

}


void set_can_down(char* canport)
{
    // ip link set can0 down
    char* commit_buf[120] = {0};
    sprintf(commit_buf, " ip link set %s down", canport);

    system(commit_buf);    

}
void set_can_bitrate(char* canport)
{
    //ip link set can0 type can bitrate 500000
    char* commit_buf[120] = {0};
    sprintf(commit_buf, "ip link set %s type can bitrate 500000", canport);

    system(commit_buf);   
}
void set_can_up(canport)
{
    // ip link set can0 up
    char* commit_buf[120] = {0};
    sprintf(commit_buf, " ip link set %s up", canport);

    system(commit_buf);   
    system(commit_buf);
    system(commit_buf);
    system(commit_buf);
    char commit_echo[] =  "echo  4096 > /sys/class/net/can0/tx_queue_len";

     system(commit_echo);
    
}


void judge_can_status(char* canport)
{

}
void start_can(char* canport)
{
    //ip link set can0 down
    set_can_down(canport);
    set_can_bitrate(canport);
    set_can_up(canport);
    judge_can_status(canport);
}

void can_process(char* canport)
{
    int s; // can raw socket
    int i;
    unsigned char number = 0;
    int nbytes;
    
    pid_t pid = -1;

    struct sockaddr_can addr;
    struct ifreq ifr;   
    struct can_filter rfilter[1];
    struct can_frame frame;
    struct can_frame frame_send;

    start_can(canport);
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
	ioctl(s, SIOCGIFINDEX, &ifr);//指定can设备
	addr.can_family = AF_CAN;
	addr.can_ifindex = ifr.ifr_ifindex;

    /* try to switch the socket into CAN FD mode */
    setsockopt(s, SOL_CAN_RAW, CAN_RAW_FD_FRAMES, &canfd_on, sizeof(canfd_on));

	/* bind can device */
	if(bind(s, (struct sockaddr *)&addr, sizeof(addr)) < 0)//将套接字与can0绑定
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
        if( can_frame_process_write_data(s,canport) )
        {
            STOPTEST = true;
        }

        
		if(diff_ms(&current,&start_time) >= 10000 )//10s
		{
			printf("\t\t Error %s time consuming >10s but can't translate corrent data\n", canport);
			STOPTEST = true;
		}
            
	 }
     STOPTEST = false;
	close(s);
	return ;
}

void can_test()
{
    can_process(CAN0Port);
    can_process(CAN1Port);
}