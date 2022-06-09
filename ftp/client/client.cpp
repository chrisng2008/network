//#pragma warning(disable:4996)  
//#include "Winsock.h"
//#include "windows.h"
//#include "stdio.h"
//#include "time.h"
//#include <iostream>
//using namespace std;
//#define RECV_PORT 3312
//#define SEND_PORT 4302  
//#pragma comment(lib, "wsock32.lib")
//SOCKET sockclient;
//char filename[20];                       //文件名  
//sockaddr_in ServerAddr;                 //服务器地址
//char rbuff[1024];                        //接收缓冲区  
//char sbuff[1024];                       //发送缓冲区  
//char InputIP[20];                       //存储输入的服务器IP
//
//
//void help()//处理help
//{
//    cout << "                         欢迎进入迷你FTP帮助菜单              " << endl
//        << "                  * * * * * * * * * * * * * * * * * * * * *       " << endl
//        << "                  1.get....................下载(接受)文件      " << endl
//        << "                     get的用法: get 文件名                         " << endl << endl
//        << "                  2.put.................上传(发送)文件       " << endl
//        << "                     put的用法：put 文件名                         " << endl
//        << "                  3.pwd..........显示当前文件夹的绝对路径       " << endl
//        << "                  4.dir............显示远方当前目录的文件       " << endl << endl
//        << "                  5.cd.............改变远方当前目录和路径       " << endl
//        << "                   cd的用法(进入下级目录): cd 路径名             " << endl
//        << "                   cd的用法(进入上级目录): cd ..                 " << endl << endl
//        << "                  6.?或者help................进入帮助菜单      " << endl
//        << "                  7.quit..........................退出FTP       " << endl
//        << "                  * * * * * * * * * * * * * * * * * * * * *       " << endl;
//}
//void list(SOCKET sockfd)
//{
//    int nRead;
//    while (true)
//    {
//        nRead = recv(sockclient, rbuff, 1024, 0);
//        //cout << nRead << endl;
//        /*if (nRead <= 0 || strcmp(FileBuffer, "n") == 0) break;*/
//        //recv函数通过sockclient套接口接受数据存入rbuff缓冲区，返回接受到的字节数      
//        if (nRead == SOCKET_ERROR)
//        {
//            printf("read response error!\n");
//            exit(1);
//        }
//        if (nRead == 0)//数据读取结束        
//            break;
//        //显示数据   
//        rbuff[nRead] = '\0';
//        //if (rbuff == "exit")
//        //    break;
//        printf("%s", rbuff);
//    }
//}
//
///*********************** put：传给远方一个文件***************************/
//int SendFile(SOCKET datatcps, FILE* file)
//{
//    printf(" sending file data..");
//    for (;;)  //从文件中循环读取数据并发送客户端       
//    {
//        int r = fread(sbuff, 1, 1024, file);//fread函数从file文件读取1个1024长度的数据到sbuff，返回成功读取的元素个数            
//        if (send(datatcps, sbuff, r, 0) == SOCKET_ERROR)
//        {
//            printf("lost the connection to client!\n");
//            closesocket(datatcps);
//            return 0;
//        }
//        if (r < 1024)                      //文件传送结束    
//            break;
//    }
//    closesocket(datatcps);
//    printf("done\n");
//    return 1;
//}
//
//
//DWORD StartSock()//启动winsock
//{
//    WSADATA WSAData;
//    char a[20];
//    memset(a, 0, 20);
//    if (WSAStartup(MAKEWORD(2, 2), &WSAData) != 0)//加载winsock版本
//    {
//        printf("sock init fail!\n");
//        return (-1);
//    }
//    if (strncmp(InputIP, a, 20) == 0)
//    {
//        printf("请输入连接的主机IP：");
//        scanf("%s", &InputIP);
//    }
//    //设置地址结构
//    ServerAddr.sin_family = AF_INET;//AF_INET表示使用IP地址族
//    ServerAddr.sin_addr.s_addr = inet_addr(InputIP);//指定服务器IP
//    ServerAddr.sin_port = htons(RECV_PORT);//设置端口号
//    return(1);
//}
//
//
////创建套接字  
//DWORD CreateSocket()
//{
//    sockclient = socket(AF_INET, SOCK_STREAM, 0);//当socket函数成功调用时返回一个新的SOCKET(Socket Descriptor)
//    if (sockclient == SOCKET_ERROR)
//    {
//        printf("sockclient create fail! \n");
//        WSACleanup();
//        return(-1);
//    }
//    return(1);
//}
//DWORD CallServer() //发送连接请求  
//{
//    CreateSocket();
//
//    if (connect(sockclient, (struct  sockaddr*)&ServerAddr, sizeof(ServerAddr)) == SOCKET_ERROR)
//    {//connect函数创建与指定外部端口的连接
//        printf("Connect fail \n");
//        memset(InputIP, 0, 20);
//        return(-1);
//    }
//    return(1);
//}
//
//DWORD TCPSend(char data[])   //发送命令  
//{
//    int length;
//    length = send(sockclient, data, strlen(data), 0);
//    //send函数通过sockclient接口发送data里面的数据，发送成功返回发送的字节数  
//    if (length <= 0)
//    {
//        printf("send data error ! \n");
//        closesocket(sockclient);
//        WSACleanup();
//        return(-1);
//    }
//    return(1);
//}
//
//
//int main()
//{
//    char messge1[10];           //定义输入要处理的命令 
//    char messge2[20];           //定义输入要处理的文件名
//    char order[30];             //输入的命令    
//    order[0] = '\0';
//    char buff[80];              //用以存储经过字串格式化的order  
//    FILE* fd;                   //File协议主要用于访问本地计算机中的文件，fd指针指向要访问的目标文件  
//    FILE* fd2;
//    int count;
//    int sin_size = sizeof(ServerAddr);
//    StartSock();
//    if (CallServer() == -1)
//        return main();          //发送连接请求失败，返回主函数  
//    printf("\n请输入命令（输入？或help进入帮助菜单）:\n");
//    memset(buff, 0, 80);            //清空数组   
//    memset(messge2, 0, 20);
//    memset(order, 0, 30);
//    memset(messge1, 0, 10);
//    memset(rbuff, 0, 1024);         //接收缓冲区
//    memset(sbuff, 0, 1024);         //发送缓冲区
//    scanf("%s", &messge1);//s%输入字符串
//    if (strncmp(messge1, "get", 3) == 0)
//        scanf("%s", &messge2);
//    if (strncmp(messge1, "put", 3) == 0)
//        scanf("%s", &messge2);
//    if (strncmp(messge1, "cd", 2) == 0)
//        scanf("%s", &messge2);
//    strcat(order, messge1);         //把messge1加在order的末尾   
//    strcat(order, " ");             //命令中间的空格    
//    strcat(order, messge2);         //把messge2加在order的末尾     
//    sprintf(buff, order);           //把调整格式的order存入buff
//
//    //help和？    
//    if (strncmp(messge1, "help", 4) == 0) {
//        help();
//    }
//    if (strncmp(messge1, "?", 1) == 0) {
//        help();
//    }
//
//    if (strncmp(messge1, "quit", 4) == 0)
//    {
//        printf("                    欢迎再次进入迷你FTP，谢谢使用！\n");
//        closesocket(sockclient);
//        WSACleanup();
//        return 0;
//    }
//    TCPSend(buff);//发送buff里面的数据        
//    recv(sockclient, rbuff, 1024, 0);
//    printf(rbuff);
//
//    if (strncmp(rbuff, "get", 3) == 0)      //get
//    {
//        fd = fopen(messge2, "wb");//使用二进制方式，打开文件，wb只写打开或新 建一个二进制文件；只允许写数据。              
//        if (fd == NULL)
//        {
//            printf("open file %s for weite failed!\n", messge2);
//            return 0;
//        }
//        while ((count = recv(sockclient, rbuff, 1024, 0)) > 0)
//        {
//            fwrite(rbuff, sizeof(rbuff), count, fd);
//        }
//        //把count个数据长度为size0f（）的数据从 rbuff输入到fd指向的目标文件             
//        fclose(fd);        //关闭文件    
//    }
//
//    if (strncmp(rbuff, "put", 3) == 0)   //put
//    {
//        strcpy(filename, rbuff + 9);
//        fd2 = fopen(filename, "rb");//rb读写打开一个二进制文件，只允许读写数据。
//        if (fd2)
//        {
//            if (!SendFile(sockclient, fd2)) {
//                printf("send failed!");
//                return 0;
//            }
//            fclose(fd2);
//        }//关闭文件
//
//        else//打开文件失败  
//        {
//            strcpy(sbuff, "can't open file!\n");
//            if (send(sockclient, sbuff, 1024, 0))
//                return 0;
//        }
//    }
//
//    if (strncmp(rbuff, "dir", 3) == 0)      //dir
//    {
//        printf("\n");
//        list(sockclient);               //列出接受到的列表内容
//    }
//    if (strncmp(rbuff, "pwd", 3) == 0)
//    {
//        list(sockclient);               //列出接受到的内容--绝对路径
//    }
//    if (strncmp(rbuff, "cd", 2) == 0) {}      //cd
//
//    closesocket(sockclient);            //关闭连接
//    WSACleanup();                       //释放Winsock    
//    return main();
//}