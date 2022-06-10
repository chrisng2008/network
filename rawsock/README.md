# 局域网抓包

Linux系统套接字主要分为3类: TCP套接字、UDP套接字和原始套接字。TCP套接字又称流式套接字，是建立在传输层TCP协议上的套接字;UDP套接字又称数据报套接字，是建立在传输层UDP协议上的套接字;原始套接字是一种比较特殊的套接字，虽然创建原始套接字的方法和创建TCP、UDP套接字的方法基本相同,但是其功能和TCP、UDP套接字相比却存在很大的差异。

TCP/UDP套接字只能接收和操作传输层或者传输层之上的数据报,因为当IP层把数据报往上传给传输层的时候，下层的数据报头部信息(如IP数据报头部和Ethernet 帧头部)都已经去除了。而原始套接字可以直接对数据链路层的数据报进行操作。

## 实现过程

以捕获arp为例

### 帧首部结构体

```cpp
typedef struct ether_header_t{
    BYTE des_hw_addr[6];    //目的地址
    BYTE src_hw_addr[6];    //源地址
    WORD frametype;         //帧格式
} ether_header_t;   
```

### arp首部结构体

```cpp
typedef struct arp_header_t{
    WORD hw_type;           //硬件地址
    WORD prot_type;         //协议类型
    BYTE hw_addr_len;       //硬件地址长度
    BYTE prot_addr_len;     //协议地址长度
    WORD flag;              //操作类型
    BYTE send_hw_addr[6];   //发送端MAC地址
    DWORD send_prot_addr;   //发送端IP地址
    BYTE des_hw_addr[6];    //目标MAC地址
    DWORD des_prot_addr;    //目标IP地址
} arp_header_t;
```

### arp数据包结构体

```cpp
//arp数据包结构体
typedef struct arp_packet_t{
    ether_header_t etherheader;     //帧首部结构体
    arp_header_t arpheader;         //arp报文首部
} arp_packet_t;
```

### 定义rawsocket类

```cpp
class rawsocket
{
    private:
    int sockfd;
    public:
    rawsocket(const int protocol);
    ~rawsocket() ;
    bool dopromisc(char *nif);   //设置混杂模式
    int receive(char *recvbuf,int buflen,struct sockaddr_in *from,int *addrlen);   //rawsocket抓包的关键
}; 
```

**工作在混杂模式下的网卡接收所有的流过网卡的帧，信包捕获程序就是在这种模式下运行的。一般的网络分析工具，都是通过把网卡设置为混杂模式来获取底层数据流。**

### 定义过滤器结构体

```cpp
typedef struct filter
{
    unsigned long sip;
    unsigned long dip;
    unsigned int protocol;    
} filter;
```

### 定义嗅探器结构体

```cpp
class rawsocsniffer:public rawsocket
{
    private:
        filter simfilter;
        char *packet;
        const int max_packet_len;
    public: 
        rawsocsniffer(int protocol);
        ~rawsocsniffer();
        bool init();
        void setfilter(filter myfilter);
        bool testbit(const unsigned int p, int k);
        void setbit(unsigned int &p,int k);     
        void sniffer();
        void analyze();
        void ParseARPPacket();
    	......
        ......
}
```

### analyze()分析函数

```cpp
void rawsocsniffer::analyze()
{
    ether_header_t *etherpacket=(ether_header_t *)packet;
    if(simfilter.protocol==0)
    simfilter.protocol=0xff;
    switch (ntohs(etherpacket->frametype))
    {
    case 0x0800:
        ...
    case 0x0806:
        if(testbit(simfilter.protocol,1))
        {
            cout<<"\n\n/*--------------arp packet--------------------*/"<<endl;
            ParseARPPacket();
        }
        break;
    case 0x0835:
        ...
    default:
        cout<<"\n\n/*--------------Unknown packet----------------*/"<<endl;
        cout<<"Unknown ethernet frametype!"<<endl;
        break;
    }
}
```

![image-20220610013932336](README.assets/image-20220610013932336.png)

![image-20220610014350297](README.assets/image-20220610014350297.png)



### ParseARPPacket()解析函数

```cpp
void rawsocsniffer::ParseARPPacket()
{
    arp_packet_t *arppacket=(arp_packet_t *)packet;
    print_hw_addr(arppacket->arpheader.send_hw_addr);
    print_hw_addr(arppacket->arpheader.des_hw_addr);
    cout<<endl;
    print_ip_addr(arppacket->arpheader.send_prot_addr)
    print_ip_addr(arppacket->arpheader.des_prot_addr);
    cout<<endl;
    cout<<setw(15)<<"Hardware type: "<<"0x"<<hex<<ntohs(arppacket->arpheader.hw_type);
    cout<<setw(15)<<" Protocol type: "<<"0x"<<hex<<ntohs(arppacket->arpheader.prot_type);
    cout<<setw(15)<<" Operation code: "<<"0x"<<hex<<ntohs(arppacket->arpheader.flag);
    cout<<endl;
} 
```



### 设置嗅探器并初始化

```cpp
sniffer.setfilter(myfilter);
if(!sniffer.init())
{
    cout<<"sniffer initialize error!"<<endl;
    exit(-1);
} 
//开始嗅探
sniffer.sniffer();
```



### ip数据报协议字段取值

| 取值 | 协议        |
| ---- | ----------- |
| 0    | HOPOPT      |
| 1    | ICMP        |
| 2    | IGMP        |
| 3    | GGP         |
| 4    | IP-in-IP    |
| 5    | ST          |
| 6    | TCP         |
| 7    | CBT         |
| 8    | EGP         |
| 9    | IGP         |
| 10   | BBN-RCC-MON |
| 11   | NVP-II      |
| 12   | PUP         |
| 13   | ARGUS       |
| 14   | EMCON       |
| 15   | XNET        |
| 16   | CHAOS       |
| 17   | UDP         |
| 18   | MUX         |
| 19   | DCN-MEAS    |
| .... | ....        |









