#pragma warning(disable:4996)  
#include "Winsock.h"
#include "windows.h"
#include "stdio.h"  
#define RECV_PORT 3312  
#define SEND_PORT 4302  
#pragma   comment(lib, "wsock32.lib")
SOCKET sockclient, sockserver;
struct sockaddr_in ServerAddr;//��������ַ
struct sockaddr_in ClientAddr;//�ͻ��˵�ַ

/***********************ȫ�ֱ���***********************/
int Addrlen;//��ַ����
char filename[20];//�ļ���
char order[10];//����  
char rbuff[1024];//���ջ�����  
char sbuff[1024];//���ͻ�����  


DWORD StartSock()    //��ʼ��winsock   
{
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
    sockclient = socket(AF_INET, SOCK_STREAM, 0);
    if (sockclient == SOCKET_ERROR)
    {
        printf("sockclient create fail ! \n");
        WSACleanup();
        return(-1);
    }
    ServerAddr.sin_family = AF_INET;
    ServerAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    ServerAddr.sin_port = htons(RECV_PORT);
    if (bind(sockclient, (struct  sockaddr  FAR*) & ServerAddr, sizeof(ServerAddr)) == SOCKET_ERROR)
    {                     //bind�������׽��ֺ͵�ַ�ṹ��   
        printf("bind is the error");
        return(-1);
    }
    return (1);
}

int SendFileRecord(SOCKET datatcps, WIN32_FIND_DATA* pfd)     //�������͵�ǰ�ļ���¼
{
    char filerecord[MAX_PATH + 32];
    FILETIME ft;         //�ļ�����ʱ��   
    FileTimeToLocalFileTime(&pfd->ftLastWriteTime, &ft);
    SYSTEMTIME lastwtime;     //SYSTEMTIMEϵͳʱ�����ݽṹ   
    FileTimeToSystemTime(&ft, &lastwtime);
    char* dir = pfd->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ? "<DIR>" : " ";
    sprintf(filerecord, "%04d-%02d-%02d %02d:%02d  %5s %10d   %-20s\n",
        lastwtime.wYear,
        lastwtime.wMonth,
        lastwtime.wDay,
        lastwtime.wHour,
        lastwtime.wMinute,
        dir,
        pfd->nFileSizeLow,
        pfd->cFileName);
    if (send(datatcps, filerecord, strlen(filerecord), 0) == SOCKET_ERROR)
    { //ͨ��datatcps�ӿڷ���filerecord���ݣ��ɹ����ط��͵��ֽ���   
        printf("Error occurs when sending file list!\n");
        return 0;
    }
    return 1;
}

int SendFileList(SOCKET datatcps)
{
    HANDLE hff;//����һ���߳�  
    char   buffer[MAX_PATH] = { 0, };
    WIN32_FIND_DATA fd;   //�����ļ�   
    hff = FindFirstFile(buffer, &fd);
    //����ͨ��FindFirstFile�����������ݵ�ǰ���ļ����·�����Ҹ��ļ����Ѵ������ļ���������Զ�ȡ��WIN32_FIND_DATA�ṹ��ȥ  
    if (hff == INVALID_HANDLE_VALUE)//��������  
    {
        const char* errstr = "can't list files!\n";
        printf("list file error!\n");
        if (send(datatcps, errstr, strlen(errstr), 0) == SOCKET_ERROR)
        {
            printf("error occurs when senging file list!\n");
        }
        closesocket(datatcps);
        return 0;
    }

    BOOL fMoreFiles = TRUE;
    while (fMoreFiles)
    {//���ʹ����ļ���Ϣ   
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
    for (;;)   //���ļ���ѭ����ȡ���ݲ����Ϳͻ���   
    {
        int r = fread(sbuff, 1, 1024, file);//��file��������ݶ���sbuff������   
        if (send(datatcps, sbuff, r, 0) == SOCKET_ERROR)
        {
            printf("lost the connection to client!\n");
            closesocket(datatcps);
            return 0;
        }
        if (r < 1024)//�ļ����ͽ���    
            break;
    }
    closesocket(datatcps);
    printf("done\n");
    return 1;
}

//����  
DWORD ConnectProcess()
{
    Addrlen = sizeof(ClientAddr);
    if (listen(sockclient, 5) < 0)
    {
        printf("Listen error");
        return(-1);
    }
    printf("������������...\n");
    for (;;)
    {
        sockserver = accept(sockclient, (struct sockaddr FAR*) & ClientAddr, &Addrlen);
        //accept����ȡ�����Ӷ��еĵ�һ����������sockclient�Ǵ��ڼ������׽���ClientAddr �Ǽ����Ķ����ַ��         
        //Addrlen�Ƕ����ַ�ĳ���
        for (;;)
        {
            memset(rbuff, 0, 1024);
            memset(sbuff, 0, 1024);
            if (recv(sockserver, rbuff, 1024, 0) <= 0)
            {
                break;
            }
            printf("\n");
            printf("��ȡ��ִ�е�����Ϊ��");
            printf(rbuff);
            if (strncmp(rbuff, "get", 3) == 0)
            {
                strcpy(filename, rbuff + 4); printf(filename);
                FILE* file; //����һ���ļ�����ָ��    
                //���������ļ�����    
                file = fopen(filename, "rb");//�����ص��ļ���ֻ�����д   
                if (file)
                {
                    sprintf(sbuff, "get file %s\n", filename);
                    if (!send(sockserver, sbuff, 1024, 0))
                    {
                        fclose(file);      return 0;
                    }
                    else
                    {//���������������Ӵ�������     
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

            if (strncmp(rbuff, "put", 3) == 0)
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
                if (!send(sockserver, sbuff, 1024, 0))
                {
                    fclose(fd);
                    return 0;
                }
                while ((count = recv(sockserver, rbuff, 1024, 0)) > 0)//recv�������ؽ��ܵ��ֽ�������count          
                    fwrite(rbuff, sizeof(char), count, fd);
                //��count�����ݳ���Ϊsize0f���������ݴ�rbuff���뵽fdָ���Ŀ���ļ�
                printf(" get %s succed!\n", filename);
                fclose(fd);
            }//put

            if (strncmp(rbuff, "pwd", 3) == 0) {
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
                SetCurrentDirectory(filename);//���õ�ǰĿ¼
            }//cd  
            closesocket(sockserver);
        }//for 2
    }//for 1
}


int main()
{
    if (StartSock() == -1)
        return(-1);
    if (CreateSocket() == -1)
        return(-1);
    if (ConnectProcess() == -1)
        return(-1);
    return(1);
}