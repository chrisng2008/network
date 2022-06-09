#include "Winsock.h" 
#include "windows.h" 
#include "stdio.h"  
#include "string.h"
#define RECV_PORT 3312  
#define SEND_PORT 4302  

#pragma   comment(lib, "wsock32.lib")    //windows下socket依赖 ws2_32.dll，必须提前加载

//加载 ws2_32.dll,支持所有的规范
//最新的DLL是 ws2_32.dll，大小为 69KB，对应的头文件为 winsock2.h。
//Winsock网络应用程序利用API 函数（如accept、send、recv等函数）进行I/O操作时有阻塞和非阻塞两种模式。


SOCKET sockclient, sockserver;     //定义socket对象
sockaddr_in ServerAddr;    //服务器地址
sockaddr_in ClientAddr;    //客户端地址 


/*******************************全局变量***********************************/
int Addrlen;         //地址长度 
char filename[20];   //文件名 
char order[10];      //命令  
char rbuff[1024];    //接收缓冲区  
char sbuff[1024];    //发送缓冲区  


DWORD StartSock()    //初始化winsock，Winsock是Windows下的网络编程接口   
{
	//每个Winsock程序必须使用WSAStartup载入合适的Winsock动态链接库，如果载入失败，WSAStartup将返回SOCKET_ERROR，
	//WSAStartup(MAKEWORD(2, 2), &wsaData);  //主版本号为2，副版本号为2，返回 0x0202
	//WSAStartup() 函数执行成功后，会将与 ws2_32.dll 有关的信息写入 WSAData 结构体变量
	//WinSock 编程的第一步就是加载 ws2_32.dll，然后调用 WSAStartup() 函数进行初始化，并指明要使用的版本号。	

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
	//1.创建套接字
	sockclient = socket(AF_INET, SOCK_STREAM, 0);
	//每个 socket 被创建后，都会分配两个缓冲区，输入缓冲区和输出缓冲区。
	// IP 地址类型，常用的有 AF_INET 和 AF_INET6。AF_INET 表示 IPv4 地址，例如 127.0.0.1；AF_INET6 表示 IPv6 地址，例如 1030::C9B4:FF12:48AA:1A2B。

	if (sockclient == SOCKET_ERROR)
	{
		printf("sockclient create fail ! \n");
		WSACleanup();     //清除记录
		return(-1);
	}
	ServerAddr.sin_family = AF_INET;   // IPv4 地址
	ServerAddr.sin_addr.s_addr = htonl(INADDR_ANY);  //ServerAddr.sin_addr.s_addr是服务器ip地址。
	//你的机器上可能有多块网卡，也就有多个IP地址，指定为INADDR_ANY，那么系统将绑定默认的网卡【即IP地址】。
	ServerAddr.sin_port = htons(RECV_PORT);    //转码端口，sin_prot 为端口号

   //2.绑定套接字
	if (bind(sockclient, (struct  sockaddr  FAR*) & ServerAddr, sizeof(ServerAddr)) == SOCKET_ERROR)
	{                     //bind函数将套接字与特定的IP地址和端口绑定起来   
		printf("bind is the error");
		return(-1);
	}
	return (1);
}

int SendFileRecord(SOCKET datatcps, WIN32_FIND_DATA* pfd)     //用来发送当前文件记录 
{
	char filerecord[MAX_PATH + 32];
	FILETIME ft;               //文件建立时间   
	FileTimeToLocalFileTime(&pfd->ftLastWriteTime, &ft);

	SYSTEMTIME lastwtime;     //systemtime系统时间数据结构   
	FileTimeToSystemTime(&ft, &lastwtime);
	char* dir = pfd->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ? "<DIR>" : " ";
	sprintf(filerecord, "%04d-%02d-%02d %02d:%02d  %5s %10d   %-20s\n",
		lastwtime.wYear,
		lastwtime.wMonth,
		lastwtime.wDay,
		lastwtime.wHour,
		lastwtime.wMinute,
		dir,
		pfd->nFileSizeLow,       //文件大小    
		pfd->cFileName);        //文件名
	if (send(datatcps, filerecord, strlen(filerecord), 0) == SOCKET_ERROR)
	{
		//通过datatcps接口发送filerecord数据，成功返回发送的字节数  
		//send向客户端发送数据 
		//原形：  send(Sock, buf, Len, 0);  //将数据原样返回
		//sock 为要发送数据的套接字，buf 为要发送的数据的缓冲区地址，len 为要发送的数据的字节数，flags 为发送数据时的选项。
		//最后的 flags 参数一般设置为 0 或 NULL
		printf("Error occurs when sending file list!\n");
		return 0;
	}
	return 1;
}

int SendFileList(SOCKET datatcps)
{
	HANDLE hff;    //建立一个线程  
	WIN32_FIND_DATA fd;   //搜索文件   
	hff = FindFirstFile("*", &fd);  //可以通过FindFirstFile（）函数根据当前的文件存放路径查找该文件来把待操作文件的相关属性读取到WIN32_FIND_DATA结构中去：  
	if (hff == INVALID_HANDLE_VALUE)  //发生错误  
	{
		const char* errstr = "can't list files!\n";
		printf("list file error!\n");
		if (send(datatcps, errstr, strlen(errstr), 0) == SOCKET_ERROR)
		{
			printf("error occurs when senging file list!\n");
		}
		closesocket(datatcps);  // closesocket意思是封闭插座:datatcps接口。 
		return 0;
	}

	BOOL fMoreFiles = TRUE;
	while (fMoreFiles)
	{   //发送此项文件信息   
		if (!SendFileRecord(datatcps, &fd))
		{
			closesocket(datatcps);
			return 0;
		}
		//搜索下一个文件    
		fMoreFiles = FindNextFile(hff, &fd);
	}
	closesocket(datatcps);
	return 1;
}

int SendFile(SOCKET datatcps, FILE* file)
{
	printf(" sending file data..");
	while(true)     //从文件中循环读取数据并发送客户端   
	{
		int r = fread(sbuff, 1, 1024, file);//把file里面的内容读到sbuff缓冲区   
		if (send(datatcps, sbuff, r, 0) == SOCKET_ERROR)
		{
			printf("lost the connection to client!\n");
			closesocket(datatcps);    // closesocket意思是封闭插座。 
			return 0;
		}
		if (r < 1024)//文件传送结束    
			break;
	}
	closesocket(datatcps);
	printf("done\n");
	return 1;
}

//建立连接  
DWORD ConnectProcess()
{
	Addrlen = sizeof(sockaddr_in);
	if (listen(sockclient, 5) < 0)
	{
		printf("Listen error");
		return(-1);
	}
	printf("服务器监听中...\n");
	while(true)
	{
		//接收客户端请求
		sockserver = accept(sockclient, (struct sockaddr FAR*) & ClientAddr, &Addrlen);
		//accept函数取出连接队列的第一个连接请求，sockclient是处于监听的套接字，ClientAddr 是监听的对象地址，         
		//Addrlen是对象地址的长度 
		while (true)
		{
			memset(rbuff, 0, 1024);    //说明：memset(ch,0,sizeof(ch));表示将ch的内容置为0，也就是清空ch
			memset(sbuff, 0, 1024);
			if (recv(sockserver, rbuff, 1024, 0) <= 0)
			{	//recv() 函数从输入缓冲区中读取数据，而不是直接从网络中读取。
				break;
			}
			printf("\n");
			printf("获取并执行的命令为：");
			printf(rbuff);


			//原型：extern int strcmp(char *s1,char * s2，int n);
		   // strncmp函数功能：比较字符串s1和s2的前n个字符
			if (strncmp(rbuff, "get", 3) == 0)
			{
				strcpy(filename, rbuff + 4);
				printf(filename);
				FILE* file; //定义一个文件访问指针    
				 //处理下载文件请求    
				file = fopen(filename, "rb");   //打开下载的文件，只允许读写   
				if (file)
				{
					sprintf(sbuff, "get file %s\n", filename);
					if (!send(sockserver, sbuff, 1024, 0))
					{
						fclose(file);       //关闭文件流 
						return 0;
					}
					else
					{  //创建额外数据连接传送数据     
						if (!SendFile(sockserver, file))
							return 0;
						fclose(file);
					}
				}//file   

				else//打开文件失败    
				{
					strcpy(sbuff, "can't open file!\n");
					if (send(sockserver, sbuff, 1024, 0))
						return 0;
				} //lost 
			}//get

			if (strncmp(rbuff, "put", 3) == 0)  //put上传文件
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
				if (!send(sockserver, sbuff, 1024, 0))   //发送数据1024字节   
				{
					fclose(fd);
					return 0;
				}
				while ((count = recv(sockserver, rbuff, 1024, 0)) > 0)
					//recv函数返回接受的字节数赋给count  

					fwrite(rbuff, sizeof(char), count, fd);
				//把count个数据长度为size0f（）的数据从rbuff输入到fd指向的目标文件
				printf(" get %s succed!\n", filename);
				fclose(fd);
			}//put 

			if (strncmp(rbuff, "pwd", 3) == 0) {     //pwd..........显示当前文件夹的绝对路径
				char   path[1000];
				GetCurrentDirectory(1000, path);//找到当前进程的当前目录  
				strcpy(sbuff, path);
				send(sockserver, sbuff, 1024, 0);
			}//pwd   

			if (strncmp(rbuff, "dir", 3) == 0) {
				strcpy(sbuff, rbuff);
				send(sockserver, sbuff, 1024, 0);
				SendFileList(sockserver);//发送当前列表 
			}//dir 

			if (strncmp(rbuff, "cd", 2) == 0)
			{
				strcpy(filename, rbuff + 3);
				strcpy(sbuff, rbuff);
				send(sockserver, sbuff, 1024, 0);
				SetCurrentDirectory(filename);   //设置当前目录，更换路径
			}
			closesocket(sockserver);
		}
	}
}


int main()
{
	if (StartSock() == -1)       //初始化winsock
		return(-1);
	if (CreateSocket() == -1)    //创建套接字，绑定等一系列操作
		return(-1);
	if (ConnectProcess() == -1)    //建立连接，并进行相应的命令操作
		return(-1);
	return(1);
}