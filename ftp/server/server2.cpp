#include "Winsock.h" 
#include "windows.h" 
#include "stdio.h"  
#include "string.h"
#define RECV_PORT 3312  
#define SEND_PORT 4302  

#pragma   comment(lib, "wsock32.lib")    //windows��socket���� ws2_32.dll��������ǰ����

//���� ws2_32.dll,֧�����еĹ淶
//���µ�DLL�� ws2_32.dll����СΪ 69KB����Ӧ��ͷ�ļ�Ϊ winsock2.h��
//Winsock����Ӧ�ó�������API ��������accept��send��recv�Ⱥ���������I/O����ʱ�������ͷ���������ģʽ��


SOCKET sockclient, sockserver;     //����socket����
sockaddr_in ServerAddr;    //��������ַ
sockaddr_in ClientAddr;    //�ͻ��˵�ַ 


/*******************************ȫ�ֱ���***********************************/
int Addrlen;         //��ַ���� 
char filename[20];   //�ļ��� 
char order[10];      //����  
char rbuff[1024];    //���ջ�����  
char sbuff[1024];    //���ͻ�����  


DWORD StartSock()    //��ʼ��winsock��Winsock��Windows�µ������̽ӿ�   
{
	//ÿ��Winsock�������ʹ��WSAStartup������ʵ�Winsock��̬���ӿ⣬�������ʧ�ܣ�WSAStartup������SOCKET_ERROR��
	//WSAStartup(MAKEWORD(2, 2), &wsaData);  //���汾��Ϊ2�����汾��Ϊ2������ 0x0202
	//WSAStartup() ����ִ�гɹ��󣬻Ὣ�� ws2_32.dll �йص���Ϣд�� WSAData �ṹ�����
	//WinSock ��̵ĵ�һ�����Ǽ��� ws2_32.dll��Ȼ����� WSAStartup() �������г�ʼ������ָ��Ҫʹ�õİ汾�š�	

	WSADATA WSAData;
	if (WSAStartup(MAKEWORD(2, 2), &WSAData) != 0)
	{
		printf("socket init fail!\n");
		return (-1);
	}
	return(1);
}

DWORD CreateSocket()
{
	//1.�����׽���
	sockclient = socket(AF_INET, SOCK_STREAM, 0);
	//ÿ�� socket �������󣬶���������������������뻺�����������������
	// IP ��ַ���ͣ����õ��� AF_INET �� AF_INET6��AF_INET ��ʾ IPv4 ��ַ������ 127.0.0.1��AF_INET6 ��ʾ IPv6 ��ַ������ 1030::C9B4:FF12:48AA:1A2B��

	if (sockclient == SOCKET_ERROR)
	{
		printf("sockclient create fail ! \n");
		WSACleanup();     //�����¼
		return(-1);
	}
	ServerAddr.sin_family = AF_INET;   // IPv4 ��ַ
	ServerAddr.sin_addr.s_addr = htonl(INADDR_ANY);  //ServerAddr.sin_addr.s_addr�Ƿ�����ip��ַ��
	//��Ļ����Ͽ����ж��������Ҳ���ж��IP��ַ��ָ��ΪINADDR_ANY����ôϵͳ����Ĭ�ϵ���������IP��ַ����
	ServerAddr.sin_port = htons(RECV_PORT);    //ת��˿ڣ�sin_prot Ϊ�˿ں�

   //2.���׽���
	if (bind(sockclient, (struct  sockaddr  FAR*) & ServerAddr, sizeof(ServerAddr)) == SOCKET_ERROR)
	{                     //bind�������׽������ض���IP��ַ�Ͷ˿ڰ�����   
		printf("bind is the error");
		return(-1);
	}
	return (1);
}

int SendFileRecord(SOCKET datatcps, WIN32_FIND_DATA* pfd)     //�������͵�ǰ�ļ���¼ 
{
	char filerecord[MAX_PATH + 32];
	FILETIME ft;               //�ļ�����ʱ��   
	FileTimeToLocalFileTime(&pfd->ftLastWriteTime, &ft);

	SYSTEMTIME lastwtime;     //systemtimeϵͳʱ�����ݽṹ   
	FileTimeToSystemTime(&ft, &lastwtime);
	char* dir = pfd->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ? "<DIR>" : " ";
	sprintf(filerecord, "%04d-%02d-%02d %02d:%02d  %5s %10d   %-20s\n",
		lastwtime.wYear,
		lastwtime.wMonth,
		lastwtime.wDay,
		lastwtime.wHour,
		lastwtime.wMinute,
		dir,
		pfd->nFileSizeLow,       //�ļ���С    
		pfd->cFileName);        //�ļ���
	if (send(datatcps, filerecord, strlen(filerecord), 0) == SOCKET_ERROR)
	{
		//ͨ��datatcps�ӿڷ���filerecord���ݣ��ɹ����ط��͵��ֽ���  
		//send��ͻ��˷������� 
		//ԭ�Σ�  send(Sock, buf, Len, 0);  //������ԭ������
		//sock ΪҪ�������ݵ��׽��֣�buf ΪҪ���͵����ݵĻ�������ַ��len ΪҪ���͵����ݵ��ֽ�����flags Ϊ��������ʱ��ѡ�
		//���� flags ����һ������Ϊ 0 �� NULL
		printf("Error occurs when sending file list!\n");
		return 0;
	}
	return 1;
}

int SendFileList(SOCKET datatcps)
{
	HANDLE hff;    //����һ���߳�  
	WIN32_FIND_DATA fd;   //�����ļ�   
	hff = FindFirstFile("*", &fd);  //����ͨ��FindFirstFile�����������ݵ�ǰ���ļ����·�����Ҹ��ļ����Ѵ������ļ���������Զ�ȡ��WIN32_FIND_DATA�ṹ��ȥ��  
	if (hff == INVALID_HANDLE_VALUE)  //��������  
	{
		const char* errstr = "can't list files!\n";
		printf("list file error!\n");
		if (send(datatcps, errstr, strlen(errstr), 0) == SOCKET_ERROR)
		{
			printf("error occurs when senging file list!\n");
		}
		closesocket(datatcps);  // closesocket��˼�Ƿ�ղ���:datatcps�ӿڡ� 
		return 0;
	}

	BOOL fMoreFiles = TRUE;
	while (fMoreFiles)
	{   //���ʹ����ļ���Ϣ   
		if (!SendFileRecord(datatcps, &fd))
		{
			closesocket(datatcps);
			return 0;
		}
		//������һ���ļ�    
		fMoreFiles = FindNextFile(hff, &fd);
	}
	closesocket(datatcps);
	return 1;
}

int SendFile(SOCKET datatcps, FILE* file)
{
	printf(" sending file data..");
	while(true)     //���ļ���ѭ����ȡ���ݲ����Ϳͻ���   
	{
		int r = fread(sbuff, 1, 1024, file);//��file��������ݶ���sbuff������   
		if (send(datatcps, sbuff, r, 0) == SOCKET_ERROR)
		{
			printf("lost the connection to client!\n");
			closesocket(datatcps);    // closesocket��˼�Ƿ�ղ����� 
			return 0;
		}
		if (r < 1024)//�ļ����ͽ���    
			break;
	}
	closesocket(datatcps);
	printf("done\n");
	return 1;
}

//��������  
DWORD ConnectProcess()
{
	Addrlen = sizeof(sockaddr_in);
	if (listen(sockclient, 5) < 0)
	{
		printf("Listen error");
		return(-1);
	}
	printf("������������...\n");
	while(true)
	{
		//���տͻ�������
		sockserver = accept(sockclient, (struct sockaddr FAR*) & ClientAddr, &Addrlen);
		//accept����ȡ�����Ӷ��еĵ�һ����������sockclient�Ǵ��ڼ������׽��֣�ClientAddr �Ǽ����Ķ����ַ��         
		//Addrlen�Ƕ����ַ�ĳ��� 
		while (true)
		{
			memset(rbuff, 0, 1024);    //˵����memset(ch,0,sizeof(ch));��ʾ��ch��������Ϊ0��Ҳ�������ch
			memset(sbuff, 0, 1024);
			if (recv(sockserver, rbuff, 1024, 0) <= 0)
			{	//recv() ���������뻺�����ж�ȡ���ݣ�������ֱ�Ӵ������ж�ȡ��
				break;
			}
			printf("\n");
			printf("��ȡ��ִ�е�����Ϊ��");
			printf(rbuff);


			//ԭ�ͣ�extern int strcmp(char *s1,char * s2��int n);
		   // strncmp�������ܣ��Ƚ��ַ���s1��s2��ǰn���ַ�
			if (strncmp(rbuff, "get", 3) == 0)
			{
				strcpy(filename, rbuff + 4);
				printf(filename);
				FILE* file; //����һ���ļ�����ָ��    
				 //���������ļ�����    
				file = fopen(filename, "rb");   //�����ص��ļ���ֻ�����д   
				if (file)
				{
					sprintf(sbuff, "get file %s\n", filename);
					if (!send(sockserver, sbuff, 1024, 0))
					{
						fclose(file);       //�ر��ļ��� 
						return 0;
					}
					else
					{  //���������������Ӵ�������     
						if (!SendFile(sockserver, file))
							return 0;
						fclose(file);
					}
				}//file   

				else//���ļ�ʧ��    
				{
					strcpy(sbuff, "can't open file!\n");
					if (send(sockserver, sbuff, 1024, 0))
						return 0;
				} //lost 
			}//get

			if (strncmp(rbuff, "put", 3) == 0)  //put�ϴ��ļ�
			{
				FILE* fd;
				int count;
				strcpy(filename, rbuff + 4);
				fd = fopen(filename, "wb");
				if (fd == NULL)
				{
					printf("open file %s for weite failed!\n", filename);
					return 0;
				}
				sprintf(sbuff, "put file %s", filename);
				if (!send(sockserver, sbuff, 1024, 0))   //��������1024�ֽ�   
				{
					fclose(fd);
					return 0;
				}
				while ((count = recv(sockserver, rbuff, 1024, 0)) > 0)
					//recv�������ؽ��ܵ��ֽ�������count  

					fwrite(rbuff, sizeof(char), count, fd);
				//��count�����ݳ���Ϊsize0f���������ݴ�rbuff���뵽fdָ���Ŀ���ļ�
				printf(" get %s succed!\n", filename);
				fclose(fd);
			}//put 

			if (strncmp(rbuff, "pwd", 3) == 0) {     //pwd..........��ʾ��ǰ�ļ��еľ���·��
				char   path[1000];
				GetCurrentDirectory(1000, path);//�ҵ���ǰ���̵ĵ�ǰĿ¼  
				strcpy(sbuff, path);
				send(sockserver, sbuff, 1024, 0);
			}//pwd   

			if (strncmp(rbuff, "dir", 3) == 0) {
				strcpy(sbuff, rbuff);
				send(sockserver, sbuff, 1024, 0);
				SendFileList(sockserver);//���͵�ǰ�б� 
			}//dir 

			if (strncmp(rbuff, "cd", 2) == 0)
			{
				strcpy(filename, rbuff + 3);
				strcpy(sbuff, rbuff);
				send(sockserver, sbuff, 1024, 0);
				SetCurrentDirectory(filename);   //���õ�ǰĿ¼������·��
			}
			closesocket(sockserver);
		}
	}
}


int main()
{
	if (StartSock() == -1)       //��ʼ��winsock
		return(-1);
	if (CreateSocket() == -1)    //�����׽��֣��󶨵�һϵ�в���
		return(-1);
	if (ConnectProcess() == -1)    //�������ӣ���������Ӧ���������
		return(-1);
	return(1);
}