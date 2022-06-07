#define _WINSOCK_DEPRECATED_NO_WARNINGS 1
#include<iostream>
#include<WinSock2.h> //����
#pragma comment(lib,"Ws2_32.lib")
using namespace std;
sockaddr_in dest_addr; // Ŀ�ĵ�ַ��socket��ַ
sockaddr_in from_addr;	//�Զ�socket��ַ

//ICMP�����ֶ�
const int DEF_ICMP_DATA_SIZE = 32;    //ICMP����Ĭ�������ֶγ���
const int MAX_ICMP_PACKET_SIZE = 1024; //ICMP������󳤶ȣ�������ͷ��
const BYTE ICMP_ECHO_REQUEST = 8;   //�������
const BYTE ICMP_ECHO_REPLY = 0;     //����Ӧ��
int r = 0;//���շ�����
int o = 0;//��ʧ������
struct icmp//icmp�����ײ���8�ֽ�

{
	unsigned char icmp_type;    //����
	unsigned char icmp_code;    //����
	unsigned short icmp_chksum; //У���
	unsigned short icmp_id;     //��ʾ��
	unsigned short icmp_seq;    //˳���
};
struct ip //ip�����ײ���20�ֽ�
{
	unsigned char ip_hl : 4;       //��ͷ����
	unsigned char ip_v : 4;        //�汾��
	unsigned char ip_tos;        //��������
	unsigned short ip_len;       //�ܳ���
	unsigned short ip_id;        //��ʶ
	unsigned short ip_off;       //��־��Ƭƫ��
	unsigned char ip_ttl;        //����ʱ��
	unsigned char ip_p;          //Э���
	unsigned short ip_sum;       //�ײ�У���
	unsigned long ip_src;        //ԴIP��ַ
	unsigned long ip_dst;        //Ŀ��IP��ַ
};

unsigned long sendtime;//��������ʱ��
char sendpacket[sizeof(icmp) + DEF_ICMP_DATA_SIZE]; //�������Ͱ�
char recvpacket[MAX_ICMP_PACKET_SIZE]; //�������ܰ�

SOCKET sockRaw;
//ԭʼ�׽ӿ�(SOCK_RAW)����ԽϵͲ�Э��(��IP��ICMP)����ֱ�ӷ���
void initsocket(char* desthost) // ����ΪĿ��ip��ַ������
{
	struct hostent* host; // ������ȡĿ��������Ϣ
	int timeout = 3000; //���峬ʱʱ��
	//��ʼ��DLL
	WSADATA wsaData;         //����WSADATA�ṹ�����windows socket��ʼ����Ϣ
	int err =  WSAStartup(MAKEWORD(2, 2), &wsaData);//�������պ����ķ���ֵ��������socketʱ�Ƿ���ִ���
	//wsaData�����洢ϵͳ���صĹ���WINSOCK������
	if (err) {
		exit(1);
	};
	// ����ԭʼ�׽���
	sockRaw = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
	
	if (setsockopt(sockRaw, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout)) != 0)  //�����׽ӿڵ�ѡ����ý��ճ�ʱʱ��
		cout << "failed to set recv timeout: " << endl << WSAGetLastError();          //����������ϴ��������

	if (setsockopt(sockRaw, SOL_SOCKET, SO_SNDTIMEO, (char*)&timeout, sizeof(timeout)) != 0) //�����׽ӿڵ�ѡ����÷��ͳ�ʱʱ��
		cout << "failed to set send timeout: " << endl << WSAGetLastError();        //����������ϴ��������

	// ��ȡĿ��ip��ַ
	char* destIP;
	if (desthost[0] == '-' && desthost[1] == 't' && desthost[2] == ' ')
	{
		destIP = desthost + 3;
	}
	else
	{
		destIP = desthost;
	}
	//ת��IP��ַ����һ�����ʮ���Ƶ�IPת����һ����������
	unsigned long uldestip = inet_addr(destIP);
	//ת�����ɹ��Ͱ���������
	if (uldestip == INADDR_NONE)
	{
		host = gethostbyname(destIP); //��������������Ӧ��������Ϣ������һ��ָ������ hostent�ṹ��ָ��
		if (host)
		{
			uldestip = (*(in_addr*)host->h_addr).s_addr;
		}
		else //������ָ��Ϊ��
		{
			cout << "�����IP��ַ��������Ч" << endl;
			WSACleanup();
			exit(1);
		}
	}
	
	// ���Ŀ�Ķ˿�socket��ַ
	ZeroMemory(&dest_addr, sizeof(dest_addr));//���Ŀ�ĵ�ַsocket
	dest_addr.sin_family = AF_INET;
	dest_addr.sin_addr.s_addr = uldestip;//���ս������ip��ַ
}


unsigned short cal_chksum(unsigned short* addr, int len)          //ICMP����㷨
{
	int nleft = len;
	int sum = 0;
	unsigned short* w = addr;
	unsigned short answer = 0;
	/*��ICMP��ͷ������������2�ֽ�Ϊ��λ�ۼ�����*/
	while (nleft > 1)
	{
		sum += *w++;
		nleft -= 2;
	}

	if (nleft == 1)
		//����ICMP��ͷΪ�������ֽ�ʱ�ۼ����һ��
	{
		/*��ICMP��ͷΪ�������ֽڣ���ʣ�����һ�ֽڡ������һ���ֽ���Ϊһ��2�ֽ����ݵĸ��ֽڣ����2�ֽ����ݵĵ��ֽ�Ϊ0�������ۼ�*/
		*(unsigned char*)(&answer) = *(unsigned char*)w;
		sum += answer;
	}

	/*У�������16λΪ��λ������ͼ���ģ�sum��32λ�ģ�sum&0xffff�ǵ�16λ��sum>>16������16λ��ȡ�����Ǹ�16λ����Ӿ��Ǹ�16λ�͵�16λ�ĺ͡�*/
	sum = (sum >> 16) + (sum & 0xffff);
	/*��һ�����п�������ӵ�ʱ���н�λ����16λ�ģ��ٰѸ�16λ�ӽ�����*/
	sum += (sum >> 16);
	/*��һ���϶��������н�λ�ˣ���ʹ����sum��16λ��0Ҳ��Ҫ�����˴�sumֻ�ܰѵ��ֽ�����ֵ��answer����ΪanswerΪ16λ*/
	answer = ~sum;
	return answer;
}

unsigned long pack(int pack_no) //��װicmp��
{
	int packetsize = 40; //���ݰ���СΪ40���ֽڣ��ײ�8�ֽڣ�����32�ֽ�
	struct icmp* icmp;
	icmp = (struct icmp*)sendpacket; //����һ���ṹ��ָ�룬����char*������ǿ��ת���ɽṹ��ָ��
	icmp->icmp_type = ICMP_ECHO_REQUEST;//�޸�icmp���ݣ���sendpacket����
	icmp->icmp_code = 0;
	icmp->icmp_chksum = 0;
	icmp->icmp_seq = pack_no;;
	icmp->icmp_id = (unsigned short)GetCurrentProcessId();
	icmp->icmp_chksum = cal_chksum((unsigned short*)icmp, packetsize); //У���㷨
	
	return GetTickCount64();//���ط���ʱ�䣬GetTickCount()�ܼ�¼����ʱ��

}
int unpack(char *buf,int len)//���
{
	struct ip* ip;
	struct icmp* icmp;
	unsigned long rtt; //��������ʱ��
	int ipaddrlen;
	ip = (struct ip*) buf;
	ipaddrlen = ip->ip_hl * 4; //ip���ݰ��ײ�����
	icmp = (struct icmp*)(buf + ipaddrlen);//����ip���ݰ��ײ����ȣ�ֱ��icmp����
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
void send() // ����icmp��
{
	
	static int pack_no = 0;
	sendtime = pack(pack_no++);
	//�������ݱ�
	if (sendto(sockRaw, sendpacket, 40, 0, (struct sockaddr*)&dest_addr, sizeof(dest_addr)) < 0)
	{
		cout << "Destination host unreachable." << endl;
	}
	
}
void revice() //����icmp��
{
	int n, fromlen;
	int success=0;
	fromlen = sizeof(from_addr);
	memset(recvpacket, 0, sizeof(recvpacket));     //��ʼ�����ջ�����
	do
	{
		if ((n = recvfrom(sockRaw, recvpacket, sizeof(recvpacket), 0, (struct sockaddr*)&from_addr, &fromlen))>=0)
		{
			success = unpack(recvpacket, n);         //��ȥICMP��ͷ
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
		cin.getline(m_Input, 40);       //����һ���ɺ��ո���ַ�����

		//�����������-t
		if (m_Input[0] == '-' && m_Input[1] == 't' && m_Input[2] == ' ')
		{
			char* a = m_Input;
			par_host = a + 3;
		}
		else  par_host = m_Input;
		initsocket(par_host);                   //��ʼ��socket�Լ����socket��Ч��
		//���Ϊ'-t '��ѭ��4�Σ���������ѭ��
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
			cout << inet_ntoa(from_addr.sin_addr) << " �� Ping ͳ����Ϣ��" << endl;
			cout << '\t' << "���ݰ����ѷ���=4���ѽ���=" << r << "����ʧ=" << o << endl;
		}
	}
	system("pause");
	return 0;

	
	
}
