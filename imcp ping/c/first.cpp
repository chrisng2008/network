#define _WINSOCK_DEPRECATED_NO_WARNINGS 1
#include<iostream>
#include<WinSock2.h> //调库
#pragma comment(lib,"Ws2_32.lib")
using namespace std;
sockaddr_in dest_addr; // 目的地址的socket地址
sockaddr_in from_addr;	//对端socket地址

//ICMP类型字段
const int DEF_ICMP_DATA_SIZE = 32;    //ICMP报文默认数据字段长度
const int MAX_ICMP_PACKET_SIZE = 1024; //ICMP报文最大长度（包括报头）
const BYTE ICMP_ECHO_REQUEST = 8;   //请求回显
const BYTE ICMP_ECHO_REPLY = 0;     //回显应答
int r = 0;//接收分组数
int o = 0;//丢失分组数
struct icmp//icmp数据首部，8字节

{
	unsigned char icmp_type;    //类型
	unsigned char icmp_code;    //编码
	unsigned short icmp_chksum; //校验和
	unsigned short icmp_id;     //标示符
	unsigned short icmp_seq;    //顺序号
};
struct ip //ip数据首部，20字节
{
	unsigned char ip_hl : 4;       //报头长度
	unsigned char ip_v : 4;        //版本号
	unsigned char ip_tos;        //服务类型
	unsigned short ip_len;       //总长度
	unsigned short ip_id;        //标识
	unsigned short ip_off;       //标志和片偏移
	unsigned char ip_ttl;        //生存时间
	unsigned char ip_p;          //协议号
	unsigned short ip_sum;       //首部校验和
	unsigned long ip_src;        //源IP地址
	unsigned long ip_dst;        //目的IP地址
};

unsigned long sendtime;//声明发送时间
char sendpacket[sizeof(icmp) + DEF_ICMP_DATA_SIZE]; //声明发送包
char recvpacket[MAX_ICMP_PACKET_SIZE]; //声明接受包

SOCKET sockRaw;
//原始套接口(SOCK_RAW)允许对较低层协议(如IP或ICMP)进行直接访问
void initsocket(char* desthost) // 参数为目标ip地址或域名
{
	struct hostent* host; // 用来获取目标主机信息
	int timeout = 3000; //定义超时时间
	//初始化DLL
	WSADATA wsaData;         //建立WSADATA结构，存放windows socket初始化信息
	int err =  WSAStartup(MAKEWORD(2, 2), &wsaData);//用来接收函数的返回值，即启动socket时是否出现错误
	//wsaData用来存储系统传回的关于WINSOCK的资料
	if (err) {
		exit(1);
	};
	// 创建原始套接字
	sockRaw = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
	
	if (setsockopt(sockRaw, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout)) != 0)  //设置套接口的选项，设置接收超时时间
		cout << "failed to set recv timeout: " << endl << WSAGetLastError();          //输出并返回上次网络错误

	if (setsockopt(sockRaw, SOL_SOCKET, SO_SNDTIMEO, (char*)&timeout, sizeof(timeout)) != 0) //设置套接口的选项，设置发送超时时间
		cout << "failed to set send timeout: " << endl << WSAGetLastError();        //输出并返回上次网络错误

	// 获取目标ip地址
	char* destIP;
	if (desthost[0] == '-' && desthost[1] == 't' && desthost[2] == ' ')
	{
		destIP = desthost + 3;
	}
	else
	{
		destIP = desthost;
	}
	//转换IP地址，将一个点分十进制的IP转换成一个长整型数
	unsigned long uldestip = inet_addr(destIP);
	//转换不成功就按域名解析
	if (uldestip == INADDR_NONE)
	{
		host = gethostbyname(destIP); //检索与主机名对应的主机信息，返回一个指向上述 hostent结构的指针
		if (host)
		{
			uldestip = (*(in_addr*)host->h_addr).s_addr;
		}
		else //若返回指针为空
		{
			cout << "输入的IP地址或域名无效" << endl;
			WSACleanup();
			exit(1);
		}
	}
	
	// 填充目的端口socket地址
	ZeroMemory(&dest_addr, sizeof(dest_addr));//清空目的地址socket
	dest_addr.sin_family = AF_INET;
	dest_addr.sin_addr.s_addr = uldestip;//接收解析后的ip地址
}


unsigned short cal_chksum(unsigned short* addr, int len)          //ICMP检查算法
{
	int nleft = len;
	int sum = 0;
	unsigned short* w = addr;
	unsigned short answer = 0;
	/*把ICMP报头二进制数据以2字节为单位累加起来*/
	while (nleft > 1)
	{
		sum += *w++;
		nleft -= 2;
	}

	if (nleft == 1)
		//处理ICMP报头为奇数个字节时累加最后一个
	{
		/*若ICMP报头为奇数个字节，会剩下最后一字节。把最后一个字节视为一个2字节数据的高字节，这个2字节数据的低字节为0，继续累加*/
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

unsigned long pack(int pack_no) //封装icmp包
{
	int packetsize = 40; //数据包大小为40个字节，首部8字节，数据32字节
	struct icmp* icmp;
	icmp = (struct icmp*)sendpacket; //定义一个结构体指针，并把char*缓存区强制转换成结构体指针
	icmp->icmp_type = ICMP_ECHO_REQUEST;//修改icmp内容，即sendpacket内容
	icmp->icmp_code = 0;
	icmp->icmp_chksum = 0;
	icmp->icmp_seq = pack_no;;
	icmp->icmp_id = (unsigned short)GetCurrentProcessId();
	icmp->icmp_chksum = cal_chksum((unsigned short*)icmp, packetsize); //校验算法
	
	return GetTickCount64();//返回发送时间，GetTickCount()能记录发送时间

}
int unpack(char *buf,int len)//解包
{
	struct ip* ip;
	struct icmp* icmp;
	unsigned long rtt; //定义往返时间
	int ipaddrlen;
	ip = (struct ip*) buf;
	ipaddrlen = ip->ip_hl * 4; //ip数据包首部长度
	icmp = (struct icmp*)(buf + ipaddrlen);//跳过ip数据包首部长度，直达icmp报文
	if((icmp->icmp_type == ICMP_ECHO_REPLY) && icmp->icmp_id == (unsigned short)GetCurrentProcessId())
	{
		len = len - 8 - ipaddrlen;
		rtt = GetTickCount64() - sendtime;
		cout << "Reply from " << inet_ntoa(from_addr.sin_addr) << ": bytes=" << len << " time=" << rtt << "ms TTL= " << fabs((double)ip->ip_ttl)
			<< endl;
		return 1;
	}
	return 0;
}
void send() // 发送icmp包
{
	
	static int pack_no = 0;
	sendtime = pack(pack_no++);
	//发送数据报
	if (sendto(sockRaw, sendpacket, 40, 0, (struct sockaddr*)&dest_addr, sizeof(dest_addr)) < 0)
	{
		cout << "Destination host unreachable." << endl;
	}
	
}
void revice() //接收icmp包
{
	int n, fromlen;
	int success=0;
	fromlen = sizeof(from_addr);
	memset(recvpacket, 0, sizeof(recvpacket));     //初始化接收缓冲区
	do
	{
		if ((n = recvfrom(sockRaw, recvpacket, sizeof(recvpacket), 0, (struct sockaddr*)&from_addr, &fromlen))>=0)
		{
			success = unpack(recvpacket, n);         //剥去ICMP报头
			r++;
		}
		else if (WSAGetLastError() == WSAETIMEDOUT)
		{
			cout << "Request timed out." << endl;
			o++;
			return;
			
		}
	} while (!success);
}
int main()
{
	int i = 0;
	char* par_host;
	char m_Input[100];
	while (1)
	{
		r = o = 0;
		cout << "ping ";
		cin.getline(m_Input, 40);       //输入一个可含空格的字符数组

		//如果是输入了-t
		if (m_Input[0] == '-' && m_Input[1] == 't' && m_Input[2] == ' ')
		{
			char* a = m_Input;
			par_host = a + 3;
		}
		else  par_host = m_Input;
		initsocket(par_host);                   //初始化socket以及检查socket有效性
		//如果为'-t '则循环4次，否则无限循环
		if (m_Input[0] == '-' && m_Input[1] == 't' && m_Input[2] == ' ')
		{
			while (1)
			{
				send();
				revice();
				Sleep(500);
			}
		}
		else
		{
			for (i = 0; i < 4; i++)
			{
				send();
				revice();
				Sleep(500);
			}
			cout << endl;
			cout << inet_ntoa(from_addr.sin_addr) << " 的 Ping 统计信息：" << endl;
			cout << '\t' << "数据包：已发送=4，已接收=" << r << "，丢失=" << o << endl;
		}
	}
	system("pause");
	return 0;

	
	
}
