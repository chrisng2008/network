# FTP服务器



## 客户端

### 初始化WinSock

```cpp
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

### 创建套接字

```cpp
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

### put、get功能实现

put功能的实现原理：使用二进制打开一个文件，然后不断循环读取数据，(每次的读取长度为1024，因为发送缓存区的大小为1024)，并且返回成功读取的个数。如果一次读取，文件还未读完，理论上成功的个数为1024，如果成功读取的个数小于1024，那么意味着已经全部读取完成。

get功能的实现原理：从客户端读取指令，然后将指令发送到服务端，服务端找到相应的文件，然后将文件按照put功能的类似方法将数据传送到客户端，客户端接收，并保存在本地，这就是get功能的实现原理。

## 服务端

服务端的工作流程为：

初始化winsock -> 创建套接字 -> 建立连接 -> 接收命令

服务端的主要作用是接收来自客户端的指令，然后分别处理指令，最后将结果发送给客户端

