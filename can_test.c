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

static void can_process_read_data(char* buff, int* buff_len, int s, char* canport, AndriodProduct* product, fsm_state_t* fsm)
{
    struct timeval timeout, timeout_config = { 0, 0 }, *timeout_current = NULL;
    timeout_config.tv_usec = 1000;//msecs// -T <msecs>  (terminate after <msecs> without any reception)\n"); //1s
    
    //???????????
    timeout_config.tv_sec = timeout_config.tv_usec / 1000;
    timeout_config.tv_usec = (timeout_config.tv_usec % 1000) * 1000;
    timeout_current = &timeout;

    fd_set rdfs;

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

        FD_ZERO(&rdfs);
        FD_SET(s,&rdfs);

        
		if (timeout_current)
			*timeout_current = timeout_config;
        printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~select~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");

        //???????????s+1
		if ((select(s+1, &rdfs, NULL, NULL, timeout_current)) <= 0) {
			perror("select");
            printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~Iiiiii~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
            return;;
		}

        printf("\n\t~~~~~~~~~~~~~~~read\n\t");

        unsigned char data[95] = {0};
	    int data_length;
        int   nbytes = read(s, &frame, sizeof(frame));
        if(nbytes <=0)
            return ;

       // printf("read datas:%s ID=%#x data length=%d\n", ifr.ifr_name, frame.can_id, frame.can_dlc);
        printf("read datas:xx ID=%#x data length=%d\n", frame.can_id, frame.can_dlc);
        for (int i=0; i < frame.can_dlc; i++)
            printf("%#x ", frame.data[i]);
        printf("\n");
        
        printf("read can data over\n");
        if(frame.can_dlc <=0 )
        {
            printf("@@@@@@@@@@@%s__wtf\n");
            return ;
        }
        //put int serail buffer //TODO 超过了怎么说
        // if(receive_len + frame.can_dlc > 1000)
        // {
        //     memset(receive_data, 0, 1024);
        //     receive_len = 0;
        // }
        *buff_len = strlen(buff);
        memcpy(&buff[*buff_len], &frame.data[0], frame.can_dlc);
        *buff_len += frame.can_dlc;

        if(parse_data(buff, *buff_len, data, &data_length) == DATA_PROCESS_SUCCESS)
        {
            printf("=====================================>get data:%s, length[%d]\n",data, data_length);
            for(int i=0; i< data_length; i++)
            {
                printf("-------------------get data:%02x\n", data[i]);
            }

            // for(int i=0; i<data_length; i++)
            // {
            // 	printf("\t\t %02x\t", data[i]);
            // }
            process_data(data, data_length, product, fsm);
            memset(buff,0,1024);
		    *buff_len = 0;
            return;
        }

        return;


}


static int reply_end(char* buff, int* buff_len, int s, char* canport, AndriodProduct* product, fsm_state_t* fsm)
{
    printf("00000000000000000000000000000000000000000000000000000000000:%s\n", canport);

        char cmd = CTRL_SEND_END;
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
            frame_send.data[i] = product->cpu_sn[cursor++];
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
        usleep(100);
        {
           // can_process_read_data(s, canport);
        }
        //第2帧
        for(int i=0;i<8;i++) //8
        {
            frame_send.data[i] = product->cpu_sn[cursor++];
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

         usleep(100);
        {
           // can_process_read_data(s, canport);
        }

        //第3帧
        frame_send.can_dlc = 3;  //2
        frame_send.data[0] = product->cpu_sn[cursor++];
        frame_send.data[1] = product->cpu_sn[cursor++];
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

        usleep(100);
        {
            can_process_read_data(buff, buff_len, s, canport, product, fsm);
        }

        return 0;

}


static int reply_idle( int s, char* canport, AndriodProduct* product, fsm_state_t* fsm)
{
    printf("00000000000000000000000000000000000000000000000000000000000:%s\n", canport);

        char cmd = CTRL_SEND_IDLE;
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
            frame_send.data[i] = product->cpu_sn[cursor++];
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
        usleep(100);
        {
           // can_process_read_data(s, canport);
        }
        //第2帧
        for(int i=0;i<8;i++) //8
        {
            frame_send.data[i] = product->cpu_sn[cursor++];
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

         usleep(100);
        {
           // can_process_read_data(s, canport);
        }

        //第3帧
        frame_send.can_dlc = 3;  //2
        frame_send.data[0] = product->cpu_sn[cursor++];
        frame_send.data[1] = product->cpu_sn[cursor++];
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

        // usleep(100);
        // {
        //     can_process_read_data(s, canport, product, fsm);
        // }

        return 0;

}

static int can_frame_process_write_data(char* buff, int* buff_len, int s, char* canport, AndriodProduct* product, fsm_state_t* fsm)
{
    printf("00000000000000000000000000000000000000000000000000000000000:%s\n", canport);

        char cmd = CTRL_SEND_MAC;

        
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
            frame_send.data[i] = product->cpu_sn[cursor++];
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
        usleep(100);
        {
           // can_process_read_data(s, canport);
        }
        //第2帧
        for(int i=0;i<8;i++) //8
        {
            frame_send.data[i] = product->cpu_sn[cursor++];
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

         usleep(100);
        {
           // can_process_read_data(s, canport);
        }

        //第3帧
        frame_send.can_dlc = 3;  //2
        frame_send.data[0] = product->cpu_sn[cursor++];
        frame_send.data[1] = product->cpu_sn[cursor++];
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

        usleep(100);
        {
            can_process_read_data(buff, buff_len, s, canport, product, fsm);
        }

        return 0;



}

// static void canfd_frame_process_write_data(int s, char* canport)
// {
//     // int required_mtu;//传输内容的最大传输内容
//     // int mtu; // 发送数据长度
//     // int enable_canfd = 1; // 开启关闭flag位


//     char write_data[120];
// 	write_data[0] = 0xAA;
// 	write_data[1] = CTRL_SEND_MAC;
// 	memcpy(write_data+2, product->cpu_sn, strlen(product->cpu_sn));
// 	write_data[2+strlen(product->cpu_sn)] = 0x55;
// 	write_data[3+strlen(product->cpu_sn)] = '\0';
// 	printf("\n\t write_data:%s\n",write_data);
// 	for(int i=0; i<strlen(write_data);i++)
// 	{	
// 		printf("%02x ", write_data[i]);
// 	}

//     /* configure can_id and can data length */
//     struct canfd_frame frame_send;
//     frame_send.can_id = 0x88;
//     frame_send.len = strlen(write_data);
//    // printf("%s ID=%#x data length=%d\n", ifr.ifr_name, frame_send.can_id, frame_send.can_dlc);
//     printf("xx ID=%#x data length=%d\n", frame_send.can_id, frame_send.len);

//     /* Sending data */
//     if(write(s, &frame_send, sizeof(frame_send)) != sizeof(frame_send))
//     {
//     perror("Send failed");
//      sleep(1);
//    // close(s);
//     return;
//     //exit(-4);
//     }
//     sleep(1);

// 	//usleep(1000000);//1s
// 	//while(1)
// 	{
// 	//	can_process_read_data(s);
// 	}

// }


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
void start_can(char* canport) //TODO
{
    //ip link set can0 down
    set_can_down(canport);
    set_can_bitrate(canport);
    set_can_up(canport);
    judge_can_status(canport);
}

// void can_process(char* canport, AndriodProduct* product)
// {
//     int s; // can raw socket
//     int i;
//     unsigned char number = 0;
//     int nbytes;
    
//     pid_t pid = -1;

//     struct sockaddr_can addr;
//     struct ifreq ifr;   
//     struct can_filter rfilter[1];
//     struct can_frame frame;
//     struct can_frame frame_send;

//     start_can(canport);
//     printf("**\t ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~can_process:%s\n", canport);
//     printf("**\t serail_process:%s\n", canport);

//     //create socket
//     if ((s = socket(PF_CAN, SOCK_RAW, CAN_RAW)) < 0)
// 	{
// 	   perror("Create socket failed");
// 	   return;
// 	}

//     /* set up can interface */
// 	strcpy(ifr.ifr_name, canport);
// 	printf("can port is %s\n",ifr.ifr_name);

//     /* assign can device */
// 	ioctl(s, SIOCGIFINDEX, &ifr);//指定can设备
// 	addr.can_family = AF_CAN;
// 	addr.can_ifindex = ifr.ifr_ifindex;

//     /* try to switch the socket into CAN FD mode */
//     setsockopt(s, SOL_CAN_RAW, CAN_RAW_FD_FRAMES, &canfd_on, sizeof(canfd_on));

// 	/* bind can device */
// 	if(bind(s, (struct sockaddr *)&addr, sizeof(addr)) < 0)//将套接字与can0绑定
// 	{
// 	     perror("Bind can device failed\n");
// 	     close(s);
// 	     return;
// 	}

// 	printf("** \t wait write cpu ID\n");
// 	fflush(stdout);
// 	struct timespec start_time;
// 	clock_gettime(CLOCK_MONOTONIC, &start_time);

// 	struct timespec current;

// 	fsm_state_t* fsm;
// 	if(!strcmp(canport, CAN0Port))
// 		fsm = &product->CAN0;
// 	else if(!strcmp(canport, CAN1Port))
// 		fsm = &product->CAN1;
// 	else
// 	{
// 		printf("mddddddddddddddddddddddddddddddddddddddddd\n");
// 	}

//     while(!STOPTEST)
//     {

//         switch (*fsm)
// 		{
// 		case FSM_IDLE:
// 			can_frame_process_write_data( s, canport, product,  fsm);
// 			break;
// 		case FSM_GET_MAC:
// 			reply_end( s, canport, product, fsm);
// 			break;
// 		case FSM_GET_END:
//             reply_idle( s, canport, product, fsm);
//             usleep(100);
//             reply_idle( s, canport, product, fsm);
//             usleep(100);
//             reply_idle( s, canport, product, fsm);
// 			STOPTEST = true;
// 			break;
// 		default:
// 			break;
// 		}

//         clock_gettime(CLOCK_MONOTONIC, &current);

//         if(diff_ms(&current,&start_time) >= 30000 )//30s
//         {
//             printf("\t\t Error %s time consuming >30s but can't receive corrent data\n", canport);
//             STOPTEST = true;
//         }
            
// 	 }

// 	close(s);
//     STOPTEST = false;
// 	return ;
// }


void can_process_t(void* params)
{
    printf_func_mark(__func__);
    parameters *data = (parameters*) params;

    char *can_port;
	can_port = data->port;

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

    start_can(data->port);

    //create socket
    if ((s = socket(PF_CAN, SOCK_RAW, CAN_RAW)) < 0)
	{
	   perror("Create socket failed");
	   return ;
	}

    /* set up can interface */
	strcpy(ifr.ifr_name, data->port);
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
	     return ;
	}

	fflush(stdout);

    fsm_state_t* fsm;
    if( !strcmp(data->port, CAN0Port))
	{
		printf("@@@1\n");
		fsm = &data->product->CAN0;
	}
	else if(!strcmp(data->port, CAN1Port))
	{
		printf("@@@1\n");
		fsm = &data->product->CAN1;
	}
	else
	{
		//fsm = &data->product->TTYS1;
		printf("????????????????wtftttyCAN???\n");
	}


	printf("** \n\t %s ------ wait write cpu ID\n", can_port);
	struct timespec  start_time;
	clock_gettime(CLOCK_MONOTONIC, &start_time);
	struct timespec current_time;
    char can_buff[1024] = {0};
	int  buff_len =0;
    bool stoptest = false;

    while(!stoptest)
    {   

        switch (*fsm)
		{
		case FSM_IDLE:
			can_frame_process_write_data(can_buff, &buff_len, s, can_port, data->product, fsm);
			break;
		case FSM_GET_MAC:
			reply_end(can_buff, &buff_len, s, can_port, data->product, fsm);
			break;
		case FSM_GET_END:
            reply_idle( s, can_port, data->product, fsm);
            usleep(100);
            reply_idle( s, can_port, data->product, fsm);
            usleep(100);
            reply_idle( s, can_port, data->product, fsm);
            *fsm = FSM_TEST_OK;
			stoptest = true;
			break;
		default:
			break;
		}

        clock_gettime(CLOCK_MONOTONIC, &current_time);

        if(diff_ms(&current_time, &start_time) >= 30000 )//30s
        {
            printf("\t\t Error %s time consuming >30s but can't receive corrent data\n", can_port);
            *fsm = FSM_TEST_FAIL;
            stoptest = true;
        }
            
	 }

	close(s);
	return;

}