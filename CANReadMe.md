# 打开can
    ip link set can0 down
    ip link set can0 type can bitrate 500000
    ip link set can0 up

# Can utils 源码注释
    1，message格式
        char* input = "008#abcd1234cdef567830";
        其中‘008表示can_id，必须是三位十六进制数据，可以表示标准帧或者扩展帧；符号#“是can_id和can数据的分界符，最后的数据是报文的数据，标准帧一次最多只能发送8bits的数据，多余的数据自动丢弃

    基本编程模板的流程为：

（1）创建套接字socket

（2）指定can设备号

（3）绑定bind

（4）如果是发送，就禁用过滤；如果是接受就设置过滤条件

（5）对套接字的fd进行read，write操作实现收发功能


ps


# 1 can通信发送提示 No buffer space available

由于缓冲队列空间不足 

 

设置即可

 

echo  4096 > /sys/class/net/can0/tx_queue_len\


         write_data:��270277098f2097dbU
aa 81 32 37 30 32 37 37 30 39 38 66 32 30 39 37 64 62 55 get_data

aa 82 32 37 30 32 37 55 success to Sent out
30 39 38 66 32 30 39 37 success to Sent out
64 55 success to Sent out



                 cpu_sn:270277098f2097db
32 37 30 32 37 37 30 39 38 66 32 30 39 37 64 62