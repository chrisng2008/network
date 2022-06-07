#define _WINSOCK_DEPRECATED_NO_WARNINGS 1
#include <Winsock2.h> //WINSOCK API的头文件
#include <process.h>
#include <iostream>
#include <string>
#include <math.h>
using namespace std;
#define SEND_SIZE 32 //定义包的大小
#define PACKET_SIZE 1024
#define ICMP_ECHO 8
#define ICMP_ECHOREPLY 0 //静态加入一个lib文件
#pragma comment(lib, "Ws2_32.lib")

struct icmp

{
    unsigned char icmp_type;    //类型
    unsigned char icmp_code;    //编码
    unsigned short icmp_chksum; //校验和
    unsigned short icmp_id;     //标示符
    unsigned short icmp_seq;    //顺序号
    unsigned long icmp_data;    //数据
};

struct ip
{
    unsigned char ip_hl : 4; //报头长度
    unsigned char ip_v : 4;  //版本号
    unsigned char ip_tos;    //服务类型
    unsigned short ip_len;   //总长度
    unsigned short ip_id;    //标识
    unsigned short ip_off;   //标志
    unsigned char ip_ttl;    //生存时间
    unsigned char ip_p;      //协议号
    unsigned short ip_sum;   //报头校验和
    unsigned long ip_src;    //源IP地址
    unsigned long ip_dst;    //目的IP地址
};

//发送包
char sendpacket[PACKET_SIZE];
//接受包
char recvpacket[PACKET_SIZE];
//网络地址
struct sockaddr_in dest_addr;
struct sockaddr_in from_addr;

int sockfd; //Socket状态变量
int pid;    //程序标志位，取得进程识别码，在process.h下

int socketInit(char* par_host);
unsigned short cal_chksum(unsigned short* addr, int len);
int pack(int pack_no);
int unpack(unsigned char* buf, int len);
void sendPacket(void);
void recvPacket(void);

int main()
{
    int i = 0;
    char* par_host;
    char m_Input[100];
    while (1)
    {
        cout << "ping ";
        cin.getline(m_Input, 40); //输入一个可含空格的字符数组

        //如果是输入了-t
        if (m_Input[0] == '-' && m_Input[1] == 't' && m_Input[2] == ' ')
        {
            char* a = m_Input;
            par_host = a + 3;
        }
        else
            par_host = m_Input;
        socketInit(par_host); //初始化socket以及检查socket有效性
        pid = _getpid();      //程序随机数标志
        //如果为'-t '则循环4次，否则无限循环
        if (m_Input[0] == '-' && m_Input[1] == 't' && m_Input[2] == ' ')
        {
            while (1)
            {
                sendPacket();
                recvPacket();
                Sleep(100);
            }
        }
        else
        {
            for (i = 0; i < 4; i++)
            {
                sendPacket();
                recvPacket();
                Sleep(100);
            }
        }
    }
    system("pause");
    return 0;
}

int socketInit(char* par_host)
{
    struct hostent* host;
    struct protoent* protocol;
    int timeout = 1000;                            //设置发送超时1000m
    WORD wVersionRequested;                        //定义类型，word类型：16位短整数
    WSADATA wsaData;                               //建立WSADATA结构，存放windows socket初始化信息
    int err;                                       //用来接收函数的返回值，即启动socket时是否出现错误
    wVersionRequested = MAKEWORD(2, 2);            //宏，高字节为2，低字节为2。514
    err = WSAStartup(wVersionRequested, &wsaData); //启动Socket，WSAStartup函数是连接应用程序与winsock.dll的第一个调用。第一个参数是WINSOCK版本号，第二个参数指向WSADATA指针，该函数返回一个int值

    //wsaData用来存储系统传回的关于WINSOCK的资料
    if (err)
    {
        exit(1);
    };
    if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2)
        //如果16进制最低那个字节内容不等于2或最高那个字节不等于2
        //字段wVersion：Windows Sockets DLL期望调用者使用的Windows Sockets规范的版本，为WORD类型
    {
        WSACleanup(); //终止Winsock 2 DLL(Ws2_32.dll)使用
        return 0;
    }

    if ((protocol = getprotobyname("icmp")) == NULL) //返回对应于给定协议名的包含名字和协议号
    {
        cout << "getprotobyname error" << endl;
        exit(1);
    } //生成使用ICMP的原始套接字，这种套接字只有root才能生成

    //初始话socket
    if ((sockfd = socket(AF_INET, SOCK_RAW, protocol->p_proto)) < 0) //初始化socket套接口，异常时返回-1
    {
        cout << "socket error" << endl;
        exit(1);
    }
    //回收root权限，设置当前用户权限

    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout)) < 0) //设置套接口的选项，设置接收超时时间
        cout << "failed to set recv timeout: " << endl
        << WSAGetLastError(); //输出并返回上次网络错误

// if (setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout, sizeof(timeout)) < 0) //设置套接口的选项，设置发送超时时间
//     cout << "failed to set send timeout: " << endl
//          << WSAGetLastError(); //输出并返回上次网络错误

    memset(&dest_addr, 0, sizeof(dest_addr)); //设置目标主机初始化，即清零

    dest_addr.sin_family = AF_INET; //设置地址族

    if (host = gethostbyname(par_host)) // 返回对应于给定主机名的主机信息，par_host是ping的目的主机的Ip
    {
        //取指针地址所指的变量，放入函数 memcpy中计算得到结果
        //将host->h_addr 的地址复制到dest_adr.sin_addr中
        memcpy((char*)&dest_addr.sin_addr, host->h_addr, host->h_length);
        //将获取到的IP值赋给目的地址中相应字段
        if (host = gethostbyaddr(host->h_addr, 4, PF_INET))
            par_host = host->h_name; //将ip解析为主机名
    }
    //INADDR_NONE 是个宏定义，代表IpAddress 无效的IP地址。
    else if (dest_addr.sin_addr.s_addr = inet_addr(par_host) == INADDR_NONE) //查找目标IP地址失败
    {
        cout << "Unkown host " << endl
            << par_host;
        exit(1);
    }
}

unsigned short cal_chksum(unsigned short* addr, int len) //ICMP检查算法
{
    int nleft = len;
    int sum = 0;
    unsigned short* w = addr;
    unsigned short answer = 0;

    /*把ICMP报头二进制数据以2字节为单位累加起来*/
    //unsigned short的大小为2字节，因此每次相加，长度减2

    while (nleft > 1)
    {
        sum += *w++;
        nleft -= 2;
    }

    if (nleft == 1)
        //处理ICMP报头为奇数个字节时累加最后一个
    {
        /*若ICMP报头为奇数个字节，会剩下最后一字节。把最后一个字节视为一个2字节数据的高字节，这个2字节数据的低字节为0，继续累加*/
        //unsigned char为1个字节
        *(unsigned char*)(&answer) = *(unsigned char*)w;
        sum += answer;
    }

    /*校验和是以16位为单位进行求和计算的，sum是32位的，sum&0xffff是低16位，sum>>16是右移16位，取到的是高16位，相加就是高16位和低16位的和。*/
    sum = (sum >> 16) + (sum & 0xffff);
    /*这一步是有可能上面加的时候有进位到高16位的，再把高16位加进来。*/
    sum += (sum >> 16);
    /*上一步肯定不会再有进位了，即使上面sum高16位非0也不要紧，此处sum只能把低字节数赋值给answer，因为answer为16位*/
    answer = ~sum;
    return answer;
}

int pack(int pack_no) //封装ICMP包
{
    int packsize;
    struct icmp* icmp;

    packsize = 8 + SEND_SIZE; //数据报大小为64字节
    icmp = (struct icmp*)sendpacket;
    icmp->icmp_type = ICMP_ECHO;
    icmp->icmp_code = 0;
    icmp->icmp_chksum = 0;
    icmp->icmp_seq = pack_no; //发送的数据报编号
    icmp->icmp_id = pid;
    icmp->icmp_data = GetTickCount();                                 //记录发送时间
    icmp->icmp_chksum = cal_chksum((unsigned short*)icmp, packsize); //校验算法
    return packsize;
}


int unpack(char* buf, int len) //解析IP包
{
    struct ip* ip;
    struct icmp* icmp;
    double rtt;
    int iphdrlen;

    ip = (struct ip*)buf;
    iphdrlen = ip->ip_hl * 4;               /*求ip报头长度,即ip报头的长度标志乘4*/
    icmp = (struct icmp*)(buf + iphdrlen); /*越过ip报头,指向ICMP报头*/
    /*确保所接收的是我所发的的ICMP的回应*/

    if ((icmp->icmp_type == ICMP_ECHOREPLY) && (icmp->icmp_id == pid))
    {
        len = len - iphdrlen - 8;
        rtt = GetTickCount() - icmp->icmp_data;
        cout << "Reply from " << inet_ntoa(from_addr.sin_addr) << ": bytes=" << len << " time=" << rtt << "ms TTL= " << fabs((double)ip->ip_ttl)
            << endl;
        return 1;
    }
    return 0;
}

void sendPacket() //发送ICMP包
{
    int packetsize; //设置icmp报头
    static int pack_no = 0;
    packetsize = pack(pack_no++);

    //发送数据报
    if (sendto(sockfd, sendpacket, packetsize, 0, (struct sockaddr*)&dest_addr, sizeof(dest_addr)) < 0)
        cout << "Destination host unreachable." << endl;
}

void recvPacket() //接收IP包
{
    int n, fromlen;
    int success;

    fromlen = sizeof(from_addr);
    do
    {
        if ((n = recvfrom(sockfd, recvpacket, sizeof(recvpacket), 0, (struct sockaddr*)&from_addr, &fromlen)) >= 0)
            success = unpack(recvpacket, n); //剥去ICMP报头
        else if (WSAGetLastError() == WSAETIMEDOUT)
        {
            cout << "Request timed out." << endl;
            return;
        }
    } while (!success);
}