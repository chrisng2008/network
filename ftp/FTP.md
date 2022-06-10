# FTP

## 客户端

客户端调用socket()通信过程：
socket()—>connect()—>recv()/recvfrom()—>send()/sendto();

### 启动WSA，设置服务器端口的地址结构

```c++
DWORD StartSock()//启动winsock
{
    WSADATA WSAData;
    char a[20];
    memset(a, 0, 20);
    if (WSAStartup(MAKEWORD(2, 2), &WSAData) != 0)//加载winsock版本
    {
        printf("sock init fail!\n");
        return (-1);
    }
    if (strncmp(InputIP, a, 20) == 0)
    {
        printf("请输入连接的主机IP：");
        cin.getline(InputIP, 10);
    }
    //设置地址结构
    ServerAddr.sin_family = AF_INET;//AF_INET表示使用IP地址族
    ServerAddr.sin_addr.s_addr = inet_addr(InputIP);//指定服务器IP
    ServerAddr.sin_port = htons(RECV_PORT);//设置端口号，将本地字节序转为网络字节序
    return(1);
}
```

### 创建套接字

```c++
DWORD CreateSocket()//创建套接字
{
    sockclient = socket(AF_INET, SOCK_STREAM, 0);//当socket函数成功调用时返回一个新的SOCKET(Socket Descriptor)
    if (sockclient == SOCKET_ERROR)
    {
        printf("sockclient create fail! \n");
        WSACleanup();
        return(-1);
    }
    return(1);
}
```

**浅谈socket套接字**

```cpp
int socket(int domain, int type, int protocol);
```

> domain ：指定通信协议族（protocol family/address）
>
> type：指定socket类型（type）
>
> protocol ：协议类型（protocol）

```
IPPROTO_IP = 0,         /* Dummy protocol for TCP       */
```

protocol = 0 意味着创建socket的时候我们使用了tcp虚拟协议。

### 发起连接请求

```c++
DWORD CallServer() //发送连接请求  
{
    CreateSocket();
    if (connect(sockclient, (struct  sockaddr*)&ServerAddr, sizeof(ServerAddr)) == SOCKET_ERROR)
    {//connect函数创建与指定外部端口的连接
        printf("Connect fail \n");
        memset(InputIP, 0, 20);
        return(-1);
    }
    return(1);
}
```

connect()函数将客户端的socket与服务器端口连接起来，发送和接收信息都使用 sockclient

### 功能的实现

指令的传输通过TCPSend()函数实现，服务器接收指令后做出响应

#### get 文件

```c++
if (strncmp(rbuff, "get", 3) == 0)      //get
    {
        fd = fopen(messge2, "wb");//使用二进制方式，打开文件，wb只写打开或新 建一个二进制文件；只允许写数据。     
        if (fd == NULL)
        {
            printf("open file %s for weite failed!\n", messge2);
            return 0;
        }
        while ((count = recv(sockclient, rbuff, 1024, 0)) > 0)
        {
            fwrite(rbuff, sizeof(rbuff), count, fd);
        }
        //把count个数据长度为size0f（）的数据从 rbuff输入到fd指向的目标文件             
        fclose(fd);        //关闭文件    
    }
```

本质上是在当前文件目录下，使用fopen()打开或新建文件，后通过recv()函数接收指定文件信息，将该信息写入打开的文件中，实现文件传输。

#### put 文件

strcpy()函数将服务器返回的文件名copy到filename，后使用sendFile函数发送文件。

sendFile实现原理：使用二进制打开一个文件，然后不断循环读取数据，(每次的读取长度为1024，因为发送缓存区的大小为1024)，并且返回成功读取的个数。如果一次读取，文件还未读完，理论上成功的个数为1024，如果成功读取的个数小于1024，那么意味着已经全部读取完成。

```c++
int SendFile(SOCKET datatcps, FILE* file)
{
    printf(" sending file data..");
    while(1)  //从文件中循环读取数据并发送客户端       
    {
        int r = fread(sbuff, 1, 1024, file);//fread函数从file文件读取1个1024长度的数据到sbuff，返回成功读取的元素个数            
        if (send(datatcps, sbuff, r, 0) == SOCKET_ERROR)
        {
            printf("lost the connection to client!\n");
            closesocket(datatcps);
            return 0;
        }
        if (r < 1024)                      //文件传送结束    
            break;
    }
    closesocket(datatcps);
    printf("done\n");
    return 1;
}
```

#### dir 功能

列出服务端文件目录，利用recv接收服务端传来的文件目录信息，保存在rbuff中，若读取数据结束，即nRead为0后，将rbuff中的信息打印在终端。

```c++
void listdir(SOCKET sockfd)
{
    int nRead;
    while (true)
    {
        nRead = recv(sockclient, rbuff, 1024, 0);
        /*if (nRead <= 0 || strcmp(FileBuffer, "n") == 0) break;*/
        //recv函数通过sockclient套接口接受数据存入rbuff缓冲区，返回接受到的字节数      
        if (nRead == SOCKET_ERROR)
        {
            printf("read response error!\n");
            exit(1);
        }
        if (nRead == 0)//数据读取结束        
            break;
        //显示数据   
        rbuff[nRead] = '\0';
        //if (rbuff == "exit")
        //    break;
        printf("%s", rbuff);
    }
}
```

#### pwd 功能

列出服务端当前的绝对路径，这部分功能的实现放在服务端，客户端主要接收其发来的绝对路径信息

#### cd功能

切换服务端当前路径，这部分功能实现同样放在服务端，客户端主要用来传达指令

#### ls 功能

该功能列出客户端的文件目录

HANDLE 指向结构的指针

通过FindFirstFile()函数将该目录下查找到的第一个文件传递给指针fd，返回找到文件或目录的句柄。在FindNextFile函数中会用到此句柄；后使用GetFileRecord函数

```
int GetFileList()
{
    HANDLE hff;    //建立一个线程  
    WIN32_FIND_DATA fd;   //搜索文件   
    hff = FindFirstFile("*", &fd);  //可以通过FindFirstFile（）函数根据当前的文件存放路径查找该文件来把待操作文件的相关属性读取到WIN32_FIND_DATA结构中去：  //查找指定目录的第一个文件或目录并返回它的句柄
    BOOL fMoreFiles = TRUE;
    while (fMoreFiles)
    {   //发送此项文件信息  
        GetFileRecord(&fd);
        fMoreFiles = FindNextFile(hff, &fd);
        //FindNextFile判断当前目录下是否有下一个目录或文件，返回值为bool类型
    }
    return 1;
}
```

将fd传递给该函数，并读取文件信息打印在终端；由于只需要打印在客户端终端，因此我们不需要使用send函数

```
int GetFileRecord(WIN32_FIND_DATA* pfd)     //用来发送当前文件记录 
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
    cout << filerecord;
    return 1;
}
```



## 服务端

服务器端调用socket()通信过程：
socket()—>bind()—>listen()—>accept()—>recv()/recvfrom()—>send()/sendto();**

服务端的主要作用是接收来自客户端的指令，然后分别处理指令，最后将结果发送给客户端，因此服务端设置了两个socket，一个主要用于监听指令，另一个用来发送指令

### 初始化Winsock

```c++
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
```

### 创建监听套接字，并与客户端地址绑定

```c++
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
```

### 监听，创建发送套接字

监听连接请求，并通过accept创建发送套接字，利用recv函数接收指令，指令信息保存在rbuff中

```c++
Addrlen = sizeof(sockaddr_in);
	if (listen(sockclient, 5) < 0)
	{
		printf("Listen error");
		return(-1);
	}
	printf("服务器监听中...\n");
	while (true)
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
```

### 功能的实现

#### get 文件

原理类似于客户端的put 文件请求，不同在于这是服务端发送文件给客户端

首先，通过字符数组匹配，将文件名写入filename中，以二进制方式打开filename，file指针指向filename，后通过SendFile函数将file内容传输给客户端

```c++
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
				}  
				else//打开文件失败    
				{
					strcpy(sbuff, "can't open file!\n");
					if (send(sockserver, sbuff, 1024, 0))
						return 0;
				} //lost 
			}//get
```

下列代码为SendFile函数的实现，文件读取以1024字节为一组，当成功读取的字节数小于1024，默认文件读取完毕。

```c++
int SendFile(SOCKET datatcps, FILE* file)
{
	printf(" sending file data..");
	while (true)     //从文件中循环读取数据并发送客户端   
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
```

#### put 文件

指令对应客户端的get 请求，通过接收客户端发送的文件数据，存储在rbuff中，后通过fwrite将rbuff写入打开的fd中

```c++
if (strncmp(rbuff, "put", 3) == 0)  //put上传文件
			{
				FILE* fd;
				int count;
				strcpy(filename, rbuff + 4);
				fd = fopen(filename, "wb");//只写方式打开或新建一个二进制文件，只允许写数据
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

```

#### dir 功能

打印出服务端目录下所有文件信息，实现类似客户端的ls功能，但不同的是dir会将找到的文件信息send给客户端

```c++
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
```



```c++
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
```

#### pwd 功能

打印出当前进程所在的绝对路径，send给客户端；

GetCurrentDirectory()将当前目录路径输入到path中，缓存区长度最多为1000字节

```c++
if (strncmp(rbuff, "pwd", 3) == 0) 
{     //pwd..........显示当前文件夹的绝对路径
				char   path[1000];
				GetCurrentDirectory(1000, path);//找到当前进程的当前目录  
				strcpy(sbuff, path);
				send(sockserver, sbuff, 1024, 0);
}
```

#### cd 功能

切换服务端的文件路径，SetCurrentDirectory帮助我们实现这个功能

```c++
if (strncmp(rbuff, "cd", 2) == 0)
{
				strcpy(filename, rbuff + 3);
				strcpy(sbuff, rbuff);
				send(sockserver, sbuff, 1024, 0);
				SetCurrentDirectory(filename);   //设置当前目录，更换路径
}
```

## 思考题

### 1.本题目采用的是C/S模式下实现文件传输协议，考虑当前应用广泛的B/S模式，这两种编程模式优缺点如何？

1.1、B/S模式的优点和缺点

B/S结构的优点

（1）、具有分布性特点，可以随时随地进行查询、浏览等业务处理。

（2）、业务扩展简单方便，通过增加网页即可增加服务器功能。

（3）、维护简单方便，只需要改变网页，即可实现所有用户的同步更新。

（4）、开发简单，共享性强

B/S 模式的缺点

（1）、个性化特点明显降低，无法实现具有个性化的功能要求。

（2）、操作是以鼠标为最基本的操作方式，无法满足快速操作的要求。

（3）、页面动态刷新，响应速度明显降低。

（4）、无法实现分页显示，给数据库访问造成较大的压力。

（5）、功能弱化，难以实现传统模式下的特殊功能要求。

1.2、C/S 模式的优点和缺点

C/S 模式的优点

（1）.由于客户端实现与服务器的直接相连，没有中间环节，因此响应速度快。

（2）.操作界面漂亮、形式多样，可以充分满足客户自身的个性化要求。

（3）.C/S结构的管理信息系统具有较强的事务处理能力，能实现复杂的业务流程。

C/S 模式的缺点

（1）.需要专门的客户端安装程序，分布功能弱，针对点多面广且不具备网络条件的用户群体，不能够实现快速部署安装和配置。

（2）.兼容性差，对于不同的开发工具，具有较大的局限性。若采用不同工具，需要重新改写程序。

（1）3.开发成本较高，需要具有一定专业水准的技术人员才能完成。

### 2.我们已经有了FTP后，为何在邮件服务器之间传输邮件(邮件也是一种文件)时，还需要SMTP协议？以及为何需要HTTP协议？

SMTP（Simple Mail Transfer Protocol）是简单邮件传输协议，是一种提供可靠且有效电子邮件传输的协议。SMTP是建立在FTP文件传输服务上的一种邮件服务，主要用于传输系统之间的邮件信息并提供与来信有关的通知。

超文本传输协议(HTTP，HyperText Transfer Protocol)是互联网上应用最为广泛的一种网络协议。所有的WWW文件都必须遵守这个标准。设计HTTP最初的目的是为了提供一种发布和接收HTML页面的方法。在Internet上的Web服务器上存放的都是超文本信息，客户机需要通过HTTP协议传输所要访问的超文本信息。HTTP包含命令和传输信息，不仅可用于Web访问，也可以用于其他因特网/内联网应用系统之间的通信，从而实现各类应用资源超媒体访问的集成。
