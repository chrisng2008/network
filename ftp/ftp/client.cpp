#pragma warning(disable:4996)  
#include "Winsock.h"
#include "windows.h"
#include "stdio.h"
#include "time.h"
#include <iostream>
using namespace std;
#define RECV_PORT 3312
#define SEND_PORT 4302  
#pragma comment(lib, "wsock32.lib")
SOCKET sockclient;
char filename[20];                       //�ļ���  
sockaddr_in ServerAddr;                 //��������ַ
char rbuff[1024];                        //���ջ�����  
char sbuff[1024];                       //���ͻ�����  
char InputIP[20];                       //�洢����ķ�����IP


void help()//����help
{
    cout << "                         ��ӭ��������FTP�����˵�              " << endl
        << "                  * * * * * * * * * * * * * * * * * * * * *       " << endl
        << "                  1.get....................����(����)�ļ�      " << endl
        << "                     get���÷�: get �ļ���                         " << endl << endl
        << "                  2.put.................�ϴ�(����)�ļ�       " << endl
        << "                     put���÷���put �ļ���                         " << endl
        << "                  3.pwd..........��ʾ��ǰ�ļ��еľ���·��       " << endl
        << "                  4.dir............��ʾԶ����ǰĿ¼���ļ�       " << endl << endl
        << "                  5.cd.............�ı�Զ����ǰĿ¼��·��       " << endl
        << "                   cd���÷�(�����¼�Ŀ¼): cd ·����             " << endl
        << "                   cd���÷�(�����ϼ�Ŀ¼): cd ..                 " << endl << endl
        << "                  6.?����help................��������˵�      " << endl
        << "                  7.quit..........................�˳�FTP       " << endl
        << "                  * * * * * * * * * * * * * * * * * * * * *       " << endl;
}
void list(SOCKET sockfd)
{
    int nRead;
    while (true)
    {
        nRead = recv(sockclient, rbuff, 1024, 0);
        //recv����ͨ��sockclient�׽ӿڽ������ݴ���rbuff�����������ؽ��ܵ����ֽ���      
        if (nRead == SOCKET_ERROR)
        {
            printf("read response error!\n");
            exit(1);
        }
        if (nRead == 0)//���ݶ�ȡ����        
            break;
        //��ʾ����   
        rbuff[nRead] = '\0';
        printf("%s", rbuff);
    }
}

/*********************** put������Զ��һ���ļ�***************************/
int SendFile(SOCKET datatcps, FILE* file)
{
    printf(" sending file data..");
    for (;;)  //���ļ���ѭ����ȡ���ݲ����Ϳͻ���       
    {
        int r = fread(sbuff, 1, 1024, file);//fread������file�ļ���ȡ1��1024���ȵ����ݵ�sbuff�����سɹ���ȡ��Ԫ�ظ���            
        if (send(datatcps, sbuff, r, 0) == SOCKET_ERROR)
        {
            printf("lost the connection to client!\n");
            closesocket(datatcps);
            return 0;
        }
        if (r < 1024)                      //�ļ����ͽ���    
            break;
    }
    closesocket(datatcps);
    printf("done\n");
    return 1;
}


DWORD StartSock()//����winsock
{
    WSADATA WSAData;
    char a[20];
    memset(a, 0, 20);
    if (WSAStartup(MAKEWORD(2, 2), &WSAData) != 0)//����winsock�汾
    {
        printf("sock init fail!\n");
        return (-1);
    }
    if (strncmp(InputIP, a, 20) == 0)
    {
        printf("���������ӵ�����IP��");
        scanf("%s", &InputIP);
    }
    //���õ�ַ�ṹ
    ServerAddr.sin_family = AF_INET;//AF_INET��ʾʹ��IP��ַ��
    ServerAddr.sin_addr.s_addr = inet_addr(InputIP);//ָ��������IP
    ServerAddr.sin_port = htons(RECV_PORT);//���ö˿ں�
    return(1);
}


//�����׽���  
DWORD CreateSocket()
{
    sockclient = socket(AF_INET, SOCK_STREAM, 0);//��socket�����ɹ�����ʱ����һ���µ�SOCKET(Socket Descriptor)
    if (sockclient == SOCKET_ERROR)
    {
        printf("sockclient create fail! \n");
        WSACleanup();
        return(-1);
    }
    return(1);
}
DWORD CallServer() //������������  
{
    CreateSocket();

    if (connect(sockclient, (struct  sockaddr*)&ServerAddr, sizeof(ServerAddr)) == SOCKET_ERROR)
    {//connect����������ָ���ⲿ�˿ڵ�����
        printf("Connect fail \n");
        memset(InputIP, 0, 20);
        return(-1);
    }
    return(1);
}

DWORD TCPSend(char data[])   //��������  
{
    int length;
    length = send(sockclient, data, strlen(data), 0);
    //send����ͨ��sockclient�ӿڷ���data��������ݣ����ͳɹ����ط��͵��ֽ���  
    if (length <= 0)
    {
        printf("send data error ! \n");
        closesocket(sockclient);
        WSACleanup();
        return(-1);
    }
    return(1);
}


int main()
{
    char messge1[10];           //��������Ҫ������ļ���  
    char messge2[20];           //��������Ҫ������ļ���
    char order[30];             //���������    
    order[0] = '\0';
    char buff[80];              //���Դ洢�����ִ���ʽ����order  
    FILE* fd;                   //FileЭ����Ҫ���ڷ��ʱ��ؼ�����е��ļ���fdָ��ָ��Ҫ���ʵ�Ŀ���ļ�  
    FILE* fd2;
    int count;
    int sin_size = sizeof(ServerAddr);
    StartSock();
    if (CallServer() == -1)
        return main();          //������������ʧ�ܣ�����������  
    printf("\n������������룿��help��������˵���:\n");
    memset(buff, 0, 80);            //�������   
    memset(messge2, 0, 20);
    memset(order, 0, 30);
    memset(messge1, 0, 10);
    memset(rbuff, 0, 1024);
    memset(sbuff, 0, 1024);
    scanf("%s", &messge1);//s%�����ַ���
    if (strncmp(messge1, "get", 3) == 0)
        scanf("%s", &messge2);
    if (strncmp(messge1, "put", 3) == 0)
        scanf("%s", &messge2);
    if (strncmp(messge1, "cd", 2) == 0)
        scanf("%s", &messge2);
    strcat(order, messge1);         //��messge1����order��ĩβ   
    strcat(order, " ");             //�����м�Ŀո�    
    strcat(order, messge2);         //��messge2����order��ĩβ     
    sprintf(buff, order);           //�ѵ�����ʽ��order����buff

    //help�ͣ�    
    if (strncmp(messge1, "help", 4) == 0) {
        help();
    }
    if (strncmp(messge1, "?", 1) == 0) {
        help();
    }

    if (strncmp(messge1, "quit", 4) == 0)
    {
        printf("                    ��ӭ�ٴν�������FTP��ллʹ�ã�\n");
        closesocket(sockclient);
        WSACleanup();
        return 0;
    }
    TCPSend(buff);//����buff���������        
    recv(sockclient, rbuff, 1024, 0);
    printf(rbuff);

    if (strncmp(rbuff, "get", 3) == 0)      //get
    {
        fd = fopen(messge2, "wb");//ʹ�ö����Ʒ�ʽ�����ļ���wbֻд�򿪻��� ��һ���������ļ���ֻ����д���ݡ�              
        if (fd == NULL)
        {
            printf("open file %s for weite failed!\n", messge2);
            return 0;
        }
        while ((count = recv(sockclient, rbuff, 1024, 0)) > 0)
        {
            fwrite(rbuff, sizeof(rbuff), count, fd);
        }
        //��count�����ݳ���Ϊsize0f���������ݴ� rbuff���뵽fdָ���Ŀ���ļ�             
        fclose(fd);        //�ر��ļ�    
    }

    if (strncmp(rbuff, "put", 3) == 0)   //put
    {
        strcpy(filename, rbuff + 9);
        fd2 = fopen(filename, "rb");//rb��д��һ���������ļ���ֻ�����д���ݡ�
        if (fd2)
        {
            if (!SendFile(sockclient, fd2)) {
                printf("send failed!");
                return 0;
            }
            fclose(fd2);
        }//�ر��ļ�

        else//���ļ�ʧ��  
        {
            strcpy(sbuff, "can't open file!\n");
            if (send(sockclient, sbuff, 1024, 0))
                return 0;
        }
    }

    if (strncmp(rbuff, "dir", 3) == 0)      //dir
    {
        printf("\n");
        list(sockclient);               //�г����ܵ����б�����
    }
    if (strncmp(rbuff, "pwd", 3) == 0)
    {
        list(sockclient);               //�г����ܵ�������--����·��
    }
    if (strncmp(rbuff, "cd", 2) == 0) {}      //cd

    closesocket(sockclient);            //�ر�����
    WSACleanup();                       //�ͷ�Winsock    
    return main();
}