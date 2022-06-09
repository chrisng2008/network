#ifndef _RAWSOCKET_H
#define _RAWSOCKET_H

//类rawsocket实现初始化，抓包，设置混杂模式等功能
class rawsocket
{
    private:
	int sockfd;
    public:
	rawsocket(const int protocol);
	~rawsocket() ;

	//set the promiscuous mode.
	bool dopromisc(char *nif);		//设置混杂模式
	
	//capture packets.
	int receive(char *recvbuf,int buflen,struct sockaddr_in *from,int *addrlen);		//rawsocket抓包关键
};
#endif
